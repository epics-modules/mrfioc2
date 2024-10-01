/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* Copyright (c) 2022 Cosylab d.d.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <cstdio>
#include <cstring>

#include <stdexcept>
#include <sstream>
#include <map>

#include <epicsString.h>
#include <epicsStdio.h> /* redirects stdout/err */
#include <drvSup.h>
#include <iocsh.h>
#include <initHooks.h>
#include <epicsExit.h>
#include <errlog.h>

#include <devLibPCI.h>
#include <devcsr.h>
#include <epicsInterrupt.h>
#include <epicsThread.h>
#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "drvem.h"
#include "mrfcsr.h"
#include "mrmpci.h"

#include <epicsExport.h>

#include "drvemIocsh.h"

// for htons() et al.
#ifdef _WIN32
 #include <Winsock2.h>
#endif

#include "evrRegMap.h"
#include "plx9030.h"
#include "plx9056.h"
#include "latticeEC30.h"

#ifdef _WIN32
 #define strtok_r(strToken,strDelimit,lasts ) (*(lasts) = strtok((strToken),(strDelimit)))
#endif

/* Bit mask used to communicate which VME interrupt levels
 * are used.  Bits are set by mrmEvrSetupVME().  Levels are
 * enabled later during iocInit.
 */
static epicsUInt8 vme_level_mask = 0;

static const epicsPCIID mrmevrs[] = {
    /* PMC-EVR-230 */
    DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030,    PCI_VENDOR_ID_PLX,
                               PCI_DEVICE_ID_MRF_PMCEVR_230, PCI_VENDOR_ID_MRF)
    /* PXI-EVR-230 */
    ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030,    PCI_VENDOR_ID_PLX,
                                PCI_DEVICE_ID_MRF_PXIEVR_230, PCI_VENDOR_ID_MRF)
    /* cPCI-EVRTG-300 */
    ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9056,    PCI_VENDOR_ID_PLX,
                                PCI_DEVICE_ID_MRF_EVRTG_300, PCI_VENDOR_ID_MRF)
    /* cPCI-EVRTG-300E ?? PCIe-EVR-300 */
    ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_EC_30,    PCI_VENDOR_ID_LATTICE,
                                PCI_DEVICE_ID_MRF_EVRTG_300E, PCI_VENDOR_ID_MRF)
    /* cPCI-EVR-300 */
    ,DEVPCI_DEVICE_VENDOR(PCI_DEVICE_ID_MRF_CPCIEVR300,    PCI_VENDOR_ID_MRF)
    /* mTCA-EVR-300 */
    ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_XILINX_DEV,    PCI_VENDOR_ID_XILINX,
                               PCI_DEVICE_ID_MRF_EVRMTCA300, PCI_VENDOR_ID_MRF)
    /* PCIe-EVR-300DC */
    ,DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_XILINX_DEV,    PCI_VENDOR_ID_XILINX,
                               PCI_SUBDEVICE_ID_PCIE_EVR_300, PCI_VENDOR_ID_MRF)
    ,DEVPCI_END
};

static const struct VMECSRID vmeevrs[] = {
    // VME-EVR-230 and VME-EVRRF-230
    {MRF_VME_IEEE_OUI, MRF_VME_EVR230RF_BID, VMECSRANY}
    // VME-EVR-300
    ,{MRF_VME_IEEE_OUI, MRF_VME_EVR300_BID, VMECSRANY}
    ,VMECSR_END
};

static const EVRMRM::Config cpci_evr_230 = {
    "cPCI-EVR-230",
    10, // pulse generators
    3,  // prescalers
    0,  // FP outputs
    4,  // FPUV outputs
    0,  // RB outputs
    0,  // Backplane outputs
    2,  // FP Delay outputs
    0,  // CML/GTX outputs
    MRMCML::typeCML,
    2,  // FP inputs
};

static const EVRMRM::Config pmc_evr_230 = {
    "PMC-EVR-230",
    10, // pulse generators
    3,  // prescalers
    3,  // FP outputs
    0,  // FPUV outputs
    10, // RB outputs
    0,  // Backplane outputs
    0,  // FP Delay outputs
    0,  // CML/GTX outputs
    MRMCML::typeCML,
    1,  // FP inputs
};

static const EVRMRM::Config pcie_evr_230 = {
    "PCIe-EVR-230",
    10, // pulse generators
    3,  // prescalers
    0,  // FP outputs
    16, // FPUV outputs
    0,  // RB outputs
    0,  // Backplane outputs
    0,  // FP Delay outputs
    0,  // CML/GTX outputs
    MRMCML::typeCML,
    0,  // FP inputs
};

static const EVRMRM::Config vme_evrrf_230 = { // no way to distinguish RF and non-RF variants
    "VME-EVRRF-230",
    16, // pulse generators
    3,  // prescalers
    8,  // FP outputs
    4,  // FPUV outputs
    16, // RB outputs
    0,  // Backplane outputs
    2,  // FP Delay outputs
    3,  // CML/GTX outputs
    MRMCML::typeCML,
    2,  // FP inputs
};

static const EVRMRM::Config cpci_evrtg_300 = {
    "cPCI-EVRTG-300",
    10, // pulse generators
    3,  // prescalers
    0,  // FP outputs
    4,  // FPUV outputs
    0,  // RB outputs
    0,  // Backplane outputs
    0,  // FP Delay outputs
    4,  // CML/GTX outputs
    MRMCML::typeTG300,
    0,  // FP inputs
};

static const EVRMRM::Config cpci_evr_300 = {
    "cPCI-EVR-300",
    14, // pulse generators
    3,  // prescalers
    0,  // FP outputs
    12, // FPUV outputs
    0,  // RB outputs
    0,  // Backplane outputs
    0,  // FP Delay outputs
    4,  // CML/GTX outputs
    MRMCML::typeTG300,
    2,  // FP inputs
};

//VME EVR
static const EVRMRM::Config vme_evr_300 = {
    "VME-EVR-300",
    24, // pulse generators
    8,  // prescalers
    0,  // FP outputs
    10, // FPUV outputs
    16, // RB outputs
    0,  // Backplane outputs
    8,  // FP Delay outputs
    4,  // CML/GTX outputs
    MRMCML::typeCML,
    2,  // FP inputs
};

static const EVRMRM::Config mtca_evr_300rf = {
    "mTCA-EVR-300RF",
    24, // pulse generators
    8,  // prescalers
    0,  // FP outputs
    2,  // FPUV outputs (only FPUV0/1, mapped to FrontUnivOut0/1, FPUV2/3 mapped in the code to FrontUnivOut18/19)
    10, // RB outputs  (RTM)
    8,  // Backplane outputs
    2,  // FP Delay outputs
    6,  // CML/GTX outputs - CLKA/B, 1x UNIV I/O slot (2 outputs), 1x SFP, 1x CML
    MRMCML::typeCML,
    /**
     * 0 <= N <= 1   : FPInMap
     * 2 <= N <= 15  : UnivInMap
     * 16 <= N <= 25 : BPInMap
     */
    26,  // FP, Univ, BP inputs
};

static const EVRMRM::Config mtca_evr_300u = { // with UNIV slots on FP
    "mTCA-EVR-300U",
    24, // pulse generators
    8,  // prescalers
    4,  // FP outputs
    4,  // FPUV outputs
    10, // RB outputs  (RTM)
    8,  // Backplane outputs
    2,  // FP Delay outputs
    2,  // CML/GTX outputs - CLKA/B
    MRMCML::typeCML,
    /**
     * 0 <= N <= 3   : FPInMap
     * 4 <= N <= 23  : UnivInMap
     * 24 <= N <= 31 : BPInMap
     */
    32,  // FP, Univ, BP inputs
};
// Default MTCA EVR
static const EVRMRM::Config mtca_evr_300 = {
    "mTCA-EVR-300",
    24, // pulse generators
    8,  // prescalers
    4,  // FP outputs
    4,  // Univ outputs
    10, // RB outputs (10 EVRTM)
    8,  // Backplane outputs
    2,  // FP Delay outputs
    2,  // CML/GTX outputs - CLKA/B
    MRMCML::typeCML,
    /**
     * 0 <= N <= 3   : FPInMap
     * 4 <= N <= 23  : UnivInMap
     * 24 <= N <= 31 : BPInMap
     * 48 - 57 : TBInMap (EVRTM)
     */
    58, // FP, Univ, BP, TB inputs
};
// Obsolte model
static const EVRMRM::Config mtca_evr_300ifb = {
    "mTCA-EVR-300IFB",
    24, // pulse generators
    8,  // prescalers
    4,  // FP outputs
    0,  // FPUV outputs
    10, // RB outputs  (via external IFB)
    8,  // Backplane outputs
    0,  // FP Delay outputs
    2,  // CML/GTX outputs
    MRMCML::typeCML,
    /**
     * 0 <= N <= 3   : FPInMap
     * 4 <= N <= 23  : UnivInMap
     * 24 <= N <= 31 : BPInMap
     */
    32, // FP, Univ, BP inputs
};

static const EVRMRM::Config pcie_evr_300 = {
    "PCIe-EVR-300DC",
    24, // pulse generators
    8,  // prescalers
    0,  // FP outputs
    16, // FPUV outputs  (via external IFB)
    0,  // RB outputs
    0,  // Backplane outputs
    0,  // FP Delay outputs
    0,  // CML/GTX outputs
    MRMCML::typeCML,
    /**
     * 0 <= N <= 3  : FPInMap
     * 4 <= N <= 23 : UnivInMap
     */
    24, // FP, Univ inputs
};

static const EVRMRM::Config cpci_evr_unknown = {
    "cPCI-EVR-???",
    10, // pulse generators
    3,  // prescalers
    1,  // FP outputs
    2,  // FPUV outputs
    1,  // RB outputs
    1,  // Backplane outputs
    1,  // FP Delay outputs
    1,  // CML/GTX outputs
    MRMCML::typeCML,
    1,  // FP inputs
};

static
const
struct printreg
{
    const char *label;
    epicsUInt32 offset;
    int rsize;
} printreg[] = {
#define REGINFO(label, name, size) {label, U##size##_##name, size}
REGINFO("FWVersion", FWVersion, 32),
REGINFO("Control", Control, 32),
REGINFO("Status",  Status, 32),
REGINFO("IRQ Flag",IRQFlag, 32),
REGINFO("IRQ Ena", IRQEnable, 32),
REGINFO("PCIMIE", PCI_MIE, 32),
REGINFO("IRQPlsmap",IRQPulseMap, 32),
REGINFO("DBufCtrl",DataBufCtrl, 32),
REGINFO("DBufTxCt",DataTxCtrl, 32),
REGINFO("CountPS", CounterPS, 32),
REGINFO("USecDiv", USecDiv, 32),
REGINFO("ClkCtrl", ClkCtrl, 32),
REGINFO("LogSts",  LogStatus, 32),
REGINFO("TSSec",TSSec, 32),
REGINFO("TSEvt",TSEvt, 32),
REGINFO("TSSecLath",TSSecLatch, 32),
REGINFO("TSEvtLath",TSEvtLatch, 32),
REGINFO("FracDiv", FracDiv, 32),
REGINFO("RFInitPhas",RFInitPhas, 32),

REGINFO("GPIODir",GPIODir, 32),
REGINFO("GPIOIn",GPIOIn, 32),
REGINFO("GPIOOut",GPIOOut, 32),

REGINFO("DCTarget",DCTarget, 32),
REGINFO("DCRxVal",DCRxVal, 32),
REGINFO("DCIntVal",DCIntVal, 32),
REGINFO("DCStatus",DCStatus, 32),
REGINFO("TOPID",TOPID, 32),

REGINFO("SeqControl0",SeqControl(0), 32),
REGINFO("SeqControl1",SeqControl(1), 32),

REGINFO("Scaler0",Scaler(0),32),
REGINFO("Scaler1",Scaler(1),32),
REGINFO("Scaler2",Scaler(2),32),
REGINFO("Scaler3",Scaler(3),32),
REGINFO("Scaler4",Scaler(4),32),
REGINFO("Scaler5",Scaler(5),32),
REGINFO("Scaler6",Scaler(6),32),
REGINFO("Scaler7",Scaler(7),32),

REGINFO("DBusPulsTrig0",DBusPulsTrig(0),32),
REGINFO("DBusPulsTrig1",DBusPulsTrig(1),32),
REGINFO("DBusPulsTrig2",DBusPulsTrig(2),32),
REGINFO("DBusPulsTrig3",DBusPulsTrig(3),32),
REGINFO("DBusPulsTrig4",DBusPulsTrig(4),32),
REGINFO("DBusPulsTrig5",DBusPulsTrig(5),32),
REGINFO("DBusPulsTrig6",DBusPulsTrig(6),32),
REGINFO("DBusPulsTrig7",DBusPulsTrig(7),32),

REGINFO("Pul0Ctrl",PulserCtrl(0),32),
REGINFO("Pul0Scal",PulserScal(0),32),
REGINFO("Pul0Dely",PulserDely(0),32),
REGINFO("Pul0Wdth",PulserWdth(0),32),

REGINFO("Pul1Ctrl",PulserCtrl(1),32),
REGINFO("Pul1Scal",PulserScal(1),32),
REGINFO("Pul1Dely",PulserDely(1),32),
REGINFO("Pul1Wdth",PulserWdth(1),32),

REGINFO("Pul2Ctrl",PulserCtrl(2),32),
REGINFO("Pul2Scal",PulserScal(2),32),
REGINFO("Pul2Dely",PulserDely(2),32),
REGINFO("Pul2Wdth",PulserWdth(2),32),

REGINFO("Pul3Ctrl",PulserCtrl(3),32),
REGINFO("Pul3Scal",PulserScal(3),32),
REGINFO("Pul3Dely",PulserDely(3),32),
REGINFO("Pul3Wdth",PulserWdth(3),32),

REGINFO("Pul4Ctrl",PulserCtrl(4),32),
REGINFO("Pul4Scal",PulserScal(4),32),
REGINFO("Pul4Dely",PulserDely(4),32),
REGINFO("Pul4Wdth",PulserWdth(4),32),

REGINFO("Pul5Ctrl",PulserCtrl(5),32),
REGINFO("Pul5Scal",PulserScal(5),32),
REGINFO("Pul5Dely",PulserDely(5),32),
REGINFO("Pul5Wdth",PulserWdth(5),32),

REGINFO("Pul6Ctrl",PulserCtrl(6),32),
REGINFO("Pul6Scal",PulserScal(6),32),
REGINFO("Pul6Dely",PulserDely(6),32),
REGINFO("Pul6Wdth",PulserWdth(6),32),

REGINFO("Pul7Ctrl",PulserCtrl(7),32),
REGINFO("Pul7Scal",PulserScal(7),32),
REGINFO("Pul7Dely",PulserDely(7),32),
REGINFO("Pul7Wdth",PulserWdth(7),32),

REGINFO("OutputMapFP0",OutputMapFP(0),32),
REGINFO("OutputMapFP1",OutputMapFP(1),32),
REGINFO("OutputMapFP2",OutputMapFP(2),32),
REGINFO("OutputMapFP3",OutputMapFP(3),32),
REGINFO("OutputMapFP4",OutputMapFP(4),32),
REGINFO("OutputMapFP5",OutputMapFP(5),32),
REGINFO("OutputMapFP6",OutputMapFP(6),32),
REGINFO("OutputMapFP7",OutputMapFP(7),32),

REGINFO("OutputMapFPUniv0",OutputMapFPUniv(0),32),
REGINFO("OutputMapFPUniv1",OutputMapFPUniv(1),32),
REGINFO("OutputMapFPUniv2",OutputMapFPUniv(2),32),
REGINFO("OutputMapFPUniv3",OutputMapFPUniv(3),32),
REGINFO("OutputMapFPUniv4",OutputMapFPUniv(4),32),
REGINFO("OutputMapFPUniv5",OutputMapFPUniv(5),32),
REGINFO("OutputMapFPUniv6",OutputMapFPUniv(6),32),
REGINFO("OutputMapFPUniv7",OutputMapFPUniv(7),32),
REGINFO("OutputMapFPUniv8",OutputMapFPUniv(8),32),
REGINFO("OutputMapFPUniv9",OutputMapFPUniv(9),32),
REGINFO("OutputMapFPUniv10",OutputMapFPUniv(10),32),
REGINFO("OutputMapFPUniv11",OutputMapFPUniv(11),32),
REGINFO("OutputMapFPUniv12",OutputMapFPUniv(12),32),
REGINFO("OutputMapFPUniv13",OutputMapFPUniv(13),32),
REGINFO("OutputMapFPUniv14",OutputMapFPUniv(14),32),
REGINFO("OutputMapFPUniv15",OutputMapFPUniv(15),32),

REGINFO("OutputMapRB0",OutputMapRB(0),32),
REGINFO("OutputMapRB1",OutputMapRB(1),32),
REGINFO("OutputMapRB2",OutputMapRB(2),32),
REGINFO("OutputMapRB3",OutputMapRB(3),32),
REGINFO("OutputMapRB4",OutputMapRB(4),32),
REGINFO("OutputMapRB5",OutputMapRB(5),32),
REGINFO("OutputMapRB6",OutputMapRB(6),32),
REGINFO("OutputMapRB7",OutputMapRB(7),32),
REGINFO("OutputMapRB8",OutputMapRB(8),32),
REGINFO("OutputMapRB9",OutputMapRB(9),32),
REGINFO("OutputMapRB10",OutputMapRB(10),32),
REGINFO("OutputMapRB11",OutputMapRB(11),32),
REGINFO("OutputMapRB12",OutputMapRB(12),32),
REGINFO("OutputMapRB13",OutputMapRB(13),32),
REGINFO("OutputMapRB14",OutputMapRB(14),32),
REGINFO("OutputMapRB15",OutputMapRB(15),32),

REGINFO("OutputMapBackplane0",OutputMapBackplane(0),32),
REGINFO("OutputMapBackplane1",OutputMapBackplane(1),32),
REGINFO("OutputMapBackplane2",OutputMapBackplane(2),32),
REGINFO("OutputMapBackplane3",OutputMapBackplane(3),32),
REGINFO("OutputMapBackplane4",OutputMapBackplane(4),32),
REGINFO("OutputMapBackplane5",OutputMapBackplane(5),32),
REGINFO("OutputMapBackplane6",OutputMapBackplane(6),32),
REGINFO("OutputMapBackplane7",OutputMapBackplane(7),32),

REGINFO("InputMapFP0",InputMapFP(0),32),
REGINFO("InputMapFP1",InputMapFP(1),32),
REGINFO("InputMapFP2",InputMapFP(2),32),
REGINFO("InputMapFP3",InputMapFP(3),32),
REGINFO("InputMapFP4",InputMapFP(4),32),
REGINFO("InputMapFP5",InputMapFP(5),32),
REGINFO("InputMapFP6",InputMapFP(6),32),
REGINFO("InputMapFP7",InputMapFP(7),32),

REGINFO("CML0Low",OutputCMLLow(0),32),
REGINFO("CML0Rise",OutputCMLRise(0),32),
REGINFO("CML0High",OutputCMLHigh(0),32),
REGINFO("CML0Fall",OutputCMLFall(0),32),
REGINFO("CML0Ena",OutputCMLEna(0),32),
REGINFO("CML0Cnt",OutputCMLCount(0),32),
REGINFO("CML0Len",OutputCMLPatLength(0),32),
REGINFO("CML0Pat0",OutputCMLPat(0,0),32),
REGINFO("CML0Pat1",OutputCMLPat(0,1),32),

REGINFO("CML1Low",OutputCMLLow(1),32),
REGINFO("CML1Rise",OutputCMLRise(1),32),
REGINFO("CML1High",OutputCMLHigh(1),32),
REGINFO("CML1Fall",OutputCMLFall(1),32),
REGINFO("CML1Ena",OutputCMLEna(1),32),
REGINFO("CML1Cnt",OutputCMLCount(1),32),
REGINFO("CML1Len",OutputCMLPatLength(1),32),
REGINFO("CML1Pat0",OutputCMLPat(1,0),32),
REGINFO("CML1Pat1",OutputCMLPat(1,1),32),

REGINFO("TXBuf0-3",DataTx(0),32),
REGINFO("RXBuf4-7",DataRx(1),32),
REGINFO("TXBuf8-11",DataTx(2),32),
REGINFO("RXBuf12-15",DataRx(3),32),
REGINFO("TXBuf16-19",DataTx(4),32),
REGINFO("RXBuf20-23",DataRx(5),32),
REGINFO("RXBuf24-27",DataRx(6),32),
REGINFO("RXBuf28-31",DataRx(7),32),

REGINFO("EventLog0",EventLog(0),32),
REGINFO("EventLog1",EventLog(1),32),
REGINFO("EventLog2",EventLog(2),32),
REGINFO("EventLog3",EventLog(3),32),
REGINFO("EventLog4",EventLog(4),32),
REGINFO("EventLog5",EventLog(5),32),
REGINFO("EventLog6",EventLog(6),32),
REGINFO("EventLog7",EventLog(7),32),

#undef REGINFO
};

static
void
printregisters(volatile epicsUInt8 *evr,epicsUInt32 len)
{
    size_t reg;

    printf("EVR register dump\n");
    for(reg=0; reg<NELEMENTS(printreg); reg++){

        if(printreg[reg].offset+printreg[reg].rsize/8 > len)
            continue;

        switch(printreg[reg].rsize){
        case 8:
            printf("%9s: %02x\n",
                   printreg[reg].label,
                   ioread8(evr+printreg[reg].offset));
            break;
        case 16:
            printf("%9s: %04x\n",
                   printreg[reg].label,
                   nat_ioread16(evr+printreg[reg].offset));
            break;
        case 32:
            printf("%9s: %08x\n",
                   printreg[reg].label,
                   nat_ioread32(evr+printreg[reg].offset));
            break;
        }
    }
}

static
bool reportCard(mrf::Object* obj, void* raw)
{
    int *level=(int*)raw;
    EVRMRM *evr=dynamic_cast<EVRMRM*>(obj);
    if(!evr)
        return true;

    printf("EVR: %s\n",obj->name().c_str());
    printf("\tFPGA Version: %08x (firmware: %s)\n", evr->fpgaFirmware(), evr->versionStr().c_str());
    printf("\tForm factor: %s\n", evr->formFactorStr().c_str());
    printf("\tClock: %.6f MHz\n",evr->clock()*1e-6);

    bus_configuration *bus = evr->getBusConfiguration();
    if(bus->busType == busType_vme){
        struct VMECSRID vmeDev;
        volatile unsigned char* csrAddr = devCSRTestSlot(vmeevrs, bus->vme.slot, &vmeDev);
        if(csrAddr){
            epicsUInt32 ader = CSRRead32(csrAddr + CSR_FN_ADER(2));
            size_t user_offset=CSRRead24(csrAddr+CR_BEG_UCSR);
            // Currently that value read from the UCSR pointer is
            // actually little endian.
            user_offset= (( user_offset & 0x00ff0000 ) >> 16 ) |
                         (( user_offset & 0x0000ff00 )       ) |
                         (( user_offset & 0x000000ff ) << 16 );

            printf("\tVME configured slot: %d\n", bus->vme.slot);
            printf("\tVME configured A24 address 0x%08x\n", bus->vme.address);
            printf("\tVME ADER: base address=0x%x\taddress modifier=0x%x\n", ader>>8, (ader&0xFF)>>2);
            printf("\tVME IRQ Level %d (configured to %d)\n", CSRRead8(csrAddr + user_offset + UCSR_IRQ_LEVEL), bus->vme.irqLevel);
            printf("\tVME IRQ Vector %d (configured to %d)\n", CSRRead8(csrAddr + user_offset + UCSR_IRQ_VECTOR), bus->vme.irqVector);
            if(*level>1) printf("\tVME card vendor: 0x%08x\n", vmeDev.vendor);
            if(*level>1) printf("\tVME card board: 0x%08x\n", vmeDev.board);
            if(*level>1) printf("\tVME card revision: 0x%08x\n", vmeDev.revision);
            if(*level>1) printf("\tVME CSR address: %p\n", csrAddr);
        }else{
            printf("\tCard not detected in configured slot %d\n", bus->vme.slot);
        }
    }
    else if(bus->busType == busType_pci){
        const epicsPCIDevice *pciDev = bus->pci.dev;
        printf("\tPCI configured bus: 0x%08x\n", pciDev->bus);
        printf("\tPCI configured device: 0x%08x\n", pciDev->device);
        printf("\tPCI configured function: 0x%08x\n", pciDev->function);
        printf("\tPCI in slot: %s\n", pciDev->slot ? pciDev->slot : "<N/A>");
        printf("\tPCI IRQ: %u\n", pciDev->irq);

    }else{
        printf("\tUnknown bus type\n");
    }

    if(*level>=2){
        printregisters(evr->base, evr->baselen);
    }
    if(*level>=1 && evr->sfp.get()){
        evr->sfp->updateNow();
        evr->sfp->report();
    }

    return true;
}

long mrmEvrReport(int level)
{
    printf("=== Begin MRF EVR support ===\n");
    mrf::Object::visitObjects(&reportCard, (void*)&level);
    printf("=== End MRF EVR support ===\n");
    return 0;
}

static
void checkVersion(volatile epicsUInt8 *base, unsigned int required, unsigned int recommended)
{
    epicsUInt32 v = READ32(base, FWVersion);

    printf("FWVersion 0x%08x\n", v);

    epicsUInt32 evr=v&FWVersion_type_mask;
    evr>>=FWVersion_type_shift;

    if(evr!=0x1)
        throw std::runtime_error("Firmware does not correspond to an EVR");

    epicsUInt32 ver=(v&FWVersion_ver_mask)>>FWVersion_ver_shift;

    printf("Found version %u\n", ver);

    if(ver<required) {
        printf("Firmware version >=%u is required\n", required);
        throw std::runtime_error("Firmware version not supported");

    } else if(ver<recommended) {
        printf("Firmware version >=%u is recommended, please consider upgrading\n", recommended);
    }
}

#ifdef __linux__
static char ifaceversion[] = "/sys/module/mrf/parameters/interfaceversion";
/* Check the interface version of the kernel module to ensure compatibility */
static
bool checkUIOVersion(int vmin, int vmax, int *actual)
{
    FILE *fd;
    int version = -1;

    fd = fopen(ifaceversion, "r");
    if(!fd) {
        printf("Can't open %s in order to read kernel module interface version. Kernel module not loaded or too old.\n", ifaceversion);
        return true;
    }
    if(fscanf(fd, "%d", &version)!=1) {
        fclose(fd);
        printf("Failed to read %s in order to get the kernel module interface version.\n", ifaceversion);
        return true;
    }
    fclose(fd);

    // Interface versions are *not* expected to be backwords or forwards compatible.
    if(version<vmin || version>vmax) {
        printf("Error: Expect MRF kernel module interface version between [%d, %d], found %d.\n", vmin, vmax, version);
        return true;
    }
    if(actual)
        *actual = version;
    return false;
}
#else
static bool checkUIOVersion(int,int,int*) {return false;}
#endif

void
mrmEvrSetupPCI(const char* id,const char* pcispec, const char* mtca_evr_model)
{
try {
    bus_configuration bus;

    bus.busType = busType_pci;

    if(mrf::Object::getObject(id)){
        printf("Object ID %s already in use\n",id);
        return;
    }

    /* Linux only
     * kernel driver interface version.
     * 0 - Broken
     * 1 - Use of irqcontrol callback to avoid races for plx pci bridges
     * 2 - Use of new PCI master enable register to avoid races for soft pci bridges
     */
    int kifacever = -1;
    if(checkUIOVersion(1,2,&kifacever))
        return;

    const epicsPCIDevice *cur=0;

    if( devPCIFindSpec(mrmevrs, pcispec, &cur,0) ){
        printf("PCI Device not found on %s\n",
               pcispec);
        return;
    }

    printf("Device %s  %x:%x.%x slot=%s\n",id,cur->bus,cur->device,cur->function,cur->slot);
    printf("Using IRQ %u\n",cur->irq);

    bus.pci.dev = cur;

    const EVRMRM::Config *conf = NULL;
    switch(cur->id.sub_device) {
    case PCI_DEVICE_ID_MRF_PMCEVR_230: conf = &pmc_evr_230; break;
    case PCI_DEVICE_ID_MRF_PXIEVR_230: conf = &cpci_evr_230; break;
    case PCI_DEVICE_ID_MRF_EVRTG_300:  conf = &cpci_evrtg_300; break;
    case PCI_DEVICE_ID_MRF_CPCIEVR300: conf = &cpci_evr_300; break;
    case PCI_DEVICE_ID_MRF_EVRMTCA300:
        if (mtca_evr_model == NULL)
        {
            // if no EVR type is provided, we assume mtca_evr_300 generic as this was the default beforehand
            mtca_evr_model = "default";
            conf = &mtca_evr_300;
        } else if (strcmp(mtca_evr_model, "UNIV") == 0) {
            printf("Config for EVR FP UNIV model (mTCA-EVR-300U).\n");
            conf = &mtca_evr_300u;
        } else if (strcmp(mtca_evr_model, "IFB") == 0) {
            printf("Config for EVR FP IFB model (mTCA-EVR-300IFB - obsolete).\n");
            conf = &mtca_evr_300ifb;
        } else if (strcmp(mtca_evr_model, "RF") == 0) {
            printf("Config for EVR FP RF model (mTCA-EVR-300RF).\n");
            conf = &mtca_evr_300rf;
        } else {
            printf("Error: mtca_evr_model arg (%s), needs no param (default) or 'UNIV' or 'RF' or 'IFB'.\n", mtca_evr_model);
            return;
        }
        break;
    case PCI_DEVICE_ID_MRF_EVRTG_300E: // aka PCI_SUBDEVICE_ID_PCIE_EVR_300
        switch (cur->id.device) {
        case PCI_DEVICE_ID_EC_30: conf = &cpci_evrtg_300; break;
        case PCI_DEVICE_ID_XILINX_DEV: conf = &pcie_evr_300; break;
        }
        break;
    }

    if(!conf) {
        printf("Unknown PCI EVR variant, making assumptions...\n");
        conf = &cpci_evr_unknown;
    }

    volatile epicsUInt8 *plx = 0, *evr = 0;
    epicsUInt32 evrlen = 0;

    if(devPCIToLocalAddr(cur,0,(volatile void**)(void *)&evr,DEVLIB_MAP_UIO1TO1))
    {
        printf("PCI error: Failed to map BAR 0\n");
        return;
    }
    if(!evr){
        printf("PCI error: BAR 0 mapped to zero? (%08lx)\n", (unsigned long)evr);
        return;
    }
    if( devPCIBarLen(cur,0,&evrlen) ) {
        printf("PCI error: Can't find BAR #0 length\n");
        return;
    }

    switch(cur->id.device) {
    case PCI_DEVICE_ID_PLX_9030:
    case PCI_DEVICE_ID_PLX_9056:
        plx = evr;

        if(devPCIToLocalAddr(cur,2,(volatile void**)(void *)&evr,DEVLIB_MAP_UIO1TO1))
        {
            printf("PCI error: Failed to map BAR 2\n");
            return;
        }
        if(!evr){
            printf("PCI error: BAR 2 mapped to zero? (%08lx)\n", (unsigned long)evr);
            return;
        }
        if( devPCIBarLen(cur,0,&evrlen) ) {
            printf("PCI error: Can't find BAR #0 length\n");
            return;
        }
    }

    // handle various PCI to local bus bridges
    switch(cur->id.device) {
    case PCI_DEVICE_ID_PLX_9030:
        printf("Setup PLX PCI 9030\n");
        /* Use the PLX device on the EVR to swap access on
         * little endian systems so we don't have no worry about
         * byte order :)
         */
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
        BITSET(LE,32, plx, LAS0BRD, LAS0BRD_ENDIAN);
#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
        BITCLR(LE,32, plx, LAS0BRD, LAS0BRD_ENDIAN);
#endif

        // Disable interrupts on device

        NAT_WRITE32(evr, IRQEnable, 0);

#ifndef __linux__
        /* Enable active high interrupt1 through the PLX to the PCI bus.
         */
        LE_WRITE16(plx, INTCSR, INTCSR_INT1_Enable|
                   INTCSR_INT1_Polarity|
                   INTCSR_PCI_Enable);
#endif
        break;

    case PCI_DEVICE_ID_PLX_9056:
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
        BITSET(LE,8, plx, BIGEND9056, BIGEND9056_BIG);
#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
        BITCLR(LE,8, plx, BIGEND9056, BIGEND9056_BIG);
#endif

        // Disable interrupts on device

        NAT_WRITE32(evr, IRQEnable, 0);

#ifndef __linux__
        BITSET(LE,32,plx, INTCSR9056, INTCSR9056_PCI_Enable|INTCSR9056_LCL_Enable);
#endif
        break;

    case PCI_DEVICE_ID_EC_30:
    case PCI_DEVICE_ID_MRF_CPCIEVR300:
    case PCI_DEVICE_ID_XILINX_DEV:
        /* the endianness the 300 series devices w/o PLX bridge
         * is a little tricky to setup.  byte order swapping is controlled
         * through the EVR's Control register and access to this register
         * is subject to byte order swapping...
         */

        // Disable EVR and set byte order to big endian
        NAT_WRITE32(evr, Control, 0);
        // Enable byte order swapping if necessary
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
        BE_WRITE32(evr, Control, 0x02000000);
#endif

        // Disable interrupts on device
        NAT_WRITE32(evr, IRQEnable, 0);

#ifndef __linux__
        BITSET32(evr, IRQEnable, IRQ_PCIee);
#endif
        break;
    default:
        printf("Unknown PCI bridge %04x\n", cur->id.device);
        return;
    }

    checkVersion(evr, 3, 6);

    // Acknowledge missed interrupts
    //TODO: This avoids a spurious FIFO Full
    NAT_WRITE32(evr, IRQFlag, NAT_READ32(evr, IRQFlag));

    // Install ISR

    EVRMRM *receiver=new EVRMRM(id,bus,conf,evr,evrlen);

    void *arg=receiver;
#ifdef __linux__
    receiver->isrLinuxPvt = (void*)cur;
#endif

    if(devPCIConnectInterrupt(cur, &EVRMRM::isr_pci, arg, 0)){
        printf("Failed to install ISR\n");
        delete receiver;
        return;
    }else{
        // Interrupts will be enabled during iocInit()
    }


#ifndef __linux__
    if(receiver->version()>=MRFVersion(0, 0xa)) {
        // RTOS doesn't need this, so always enable
        WRITE32(evr, PCI_MIE, EVG_MIE_ENABLE);
    }
#else
    if(receiver->version()>=MRFVersion(0, 0xa) && kifacever>=2) {
        // PCI master enable supported by firmware and kernel module.
        // the kernel will set this bit when devPCIEnableInterrupt() is called
    } else if(cur->id.device==PCI_DEVICE_ID_PLX_9030 ||
              cur->id.device==PCI_DEVICE_ID_PLX_9056) {
        // PLX based devices don't need special handling
        WRITE32(evr, PCI_MIE, EVG_MIE_ENABLE);
    } else if(receiver->version()<MRFVersion(0, 0xa)) {
        // old firmware and (maybe) old kernel module.
        // this will still work, so just complain
        printf("Warning: this configuration of FW and SW is known to have race conditions in interrupt handling.\n"
                     "         Please consider upgrading to FW version 0xA.\n");
        if(kifacever<2)
            printf("         Also upgrade the linux kernel module to interface version 2.");
    } else if(receiver->version()>=MRFVersion(0, 0xa) && kifacever<2) {
        // New firmware w/ old kernel module, this won't work
        throw std::runtime_error("FW version 0xA for this device requires a linux kernel module w/ interface version 2");
    } else {
        throw std::logic_error("logic error in FW/kernel module compatibility check.");
    }

    /* ask the kernel module to enable interrupts */
    printf("Enabling interrupts\n");
    if(devPCIEnableInterrupt(cur)) {
        printf("Failed to enable interrupt\n");
        delete receiver;
        return;
    }
#endif

} catch(std::exception& e) {
    printf("Error: %s\n",e.what());
}
    errlogFlush();
}

static
void
printRamEvt(EVRMRM *evr,int evt,int ram)
{
    if(evt<0 || evt>255)
        return;
    if(ram<0 || ram>1)
        return;

    epicsUInt32 map[4];

    map[0]=NAT_READ32(evr->base, MappingRam(ram,evt,Internal));
    map[1]=NAT_READ32(evr->base, MappingRam(ram,evt,Trigger));
    map[2]=NAT_READ32(evr->base, MappingRam(ram,evt,Set));
    map[3]=NAT_READ32(evr->base, MappingRam(ram,evt,Reset));

    printf("Event 0x%02x %3d ",evt,evt);
    printf("%08x %08x %08x %08x\n",map[0],map[1],map[2],map[3]);
}

static
bool
enableIRQ(mrf::Object* obj, void*)
{
    EVRMRM *mrm=dynamic_cast<EVRMRM*>(obj);
    if(!mrm)
        return true;

    mrm->enableIRQ();

    return true;
}

static
bool
disableIRQ(mrf::Object* obj, void*)
{
    EVRMRM *mrm=dynamic_cast<EVRMRM*>(obj);
    if(!mrm)
        return true;

    WRITE32(mrm->base, IRQEnable, 0);
    return true;
}

static
void
evrShutdown(void*)
{
    mrf::Object::visitObjects(&disableIRQ,0);
}

void mrmEvrInithooks(initHookState state)
{
    epicsUInt8 lvl;
    switch(state)
    {
    case initHookAfterInterruptAccept:
        // Register hook to disable interrupts on IOC shutdown
        epicsAtExit(&evrShutdown, NULL);
        // First enable interrupts for each EVR
        mrf::Object::visitObjects(&enableIRQ,0);
        // Then enable all used levels
        for(lvl=1; lvl<=7; ++lvl)
        {
            if (vme_level_mask&(1<<(lvl-1))) {
                if(devEnableInterruptLevelVME(lvl))
                {
                    printf("Failed to enable interrupt level %d\n",lvl);
                    return;
                }
            }

        }

        break;
  default:
        break;
    }
}


void
mrmEvrSetupVME(const char* id,int slot,int base,int level, int vector)
{
try {
    bus_configuration bus;

    bus.busType = busType_vme;
    bus.vme.slot = slot;
    bus.vme.address = base;
    bus.vme.irqLevel = level;
    bus.vme.irqVector = vector;

    if(mrf::Object::getObject(id)){
        printf("ID %s already in use\n",id);
        return;
    }

    struct VMECSRID info;

    volatile unsigned char* csr=devCSRTestSlot(vmeevrs,slot,&info);
    if(!csr){
        printf("No EVR in slot %d\n",slot);
        return;
    }

    printf("Setting up EVR in VME Slot %d\n",slot);

    printf("Found vendor: %08x board: %08x rev.: %08x\n",
           info.vendor, info.board, info.revision);

    //Create the conf struct
    const EVRMRM::Config *conf = NULL;

    switch(info.board) {
        case MRF_VME_EVR230RF_BID: conf = &vme_evrrf_230; break;
        case MRF_VME_EVR300_BID: conf = &vme_evr_300; break;
    }
    // Set base address

  /* Use function 0 for 16-bit addressing (length 0x00800 bytes)
   * and function 1 for 24-bit addressing (length 0x10000 bytes)
   * and function 2 for 32-bit addressing (length 0x40000 bytes)
   *
   * All expose the same registers, but not all registers are
   * visible through functions 0 or 1.
   */

    CSRSetBase(csr, 2, base, VME_AM_EXT_SUP_DATA);

    {
        epicsUInt32 temp=CSRRead32((csr) + CSR_FN_ADER(2));

        if(temp != CSRADER((epicsUInt32)base,VME_AM_EXT_SUP_DATA)) {
            printf("Failed to set CSR Base address in ADER2.  Check VME bus and card firmware version.\n");
            return;
        }
    }

    volatile unsigned char* evr;
    char *Description = allocSNPrintf(40, "EVR-%d '%s' slot %d",
                                      info.board & MRF_BID_SERIES_MASK,
                                      id, slot);

    if(devRegisterAddress(Description, atVMEA32, base, EVR_REGMAP_SIZE, (volatile void**)(void *)&evr))
    {
        printf("Failed to map address %08x\n",base);
        return;
    }

    epicsUInt32 junk;
    if(devReadProbe(sizeof(junk), (volatile void*)(evr+U32_FWVersion), (void*)&junk)) {
        printf("Failed to read from MRM registers (but could read CSR registers)\n");
        return;
    }

    checkVersion(evr, 4, 5);

    // Read offset from start of CSR to start of user (card specific) CSR.
    size_t user_offset=CSRRead24(csr+CR_BEG_UCSR);
    // Currently that value read from the UCSR pointer is
    // actually little endian.
    user_offset= (( user_offset & 0x00ff0000 ) >> 16 ) |
                 (( user_offset & 0x0000ff00 )       ) |
                 (( user_offset & 0x000000ff ) << 16 );
    volatile unsigned char* user_csr=csr+user_offset;

    NAT_WRITE32(evr, IRQEnable, 0); // Disable interrupts

    EVRMRM *receiver=new EVRMRM(id, bus, conf, evr, EVR_REGMAP_SIZE);

    if(level>0 && vector>=0) {
        CSRWrite8(user_csr+UCSR_IRQ_LEVEL,  level&0x7);
        CSRWrite8(user_csr+UCSR_IRQ_VECTOR, vector&0xff);

        printf("Using IRQ %d:%2d\n",
               CSRRead8(user_csr+UCSR_IRQ_LEVEL),
               CSRRead8(user_csr+UCSR_IRQ_VECTOR)
               );

        // Acknowledge missed interrupts
        //TODO: This avoids a spurious FIFO Full
        NAT_WRITE32(evr, IRQFlag, NAT_READ32(evr, IRQFlag));

        level&=0x7;
        // VME IRQ level will be enabled later during iocInit()
        vme_level_mask|=1<<(level-1);

        if(devConnectInterruptVME(vector&0xff, &EVRMRM::isr_vme, receiver))
        {
            printf("Failed to connection VME IRQ %d\n",vector&0xff);
            delete receiver;
            return;
        }

        // Interrupts will be enabled during iocInit()
    }

} catch(std::exception& e) {
    printf("Error: %s\n",e.what());
}
    errlogFlush();
}

void
mrmEvrDumpMap(const char* id,int evt,int ram)
{
try {
    mrf::Object *obj=mrf::Object::getObject(id);
    if(!obj)
        throw std::runtime_error("Object not found");
    EVRMRM *card=dynamic_cast<EVRMRM*>(obj);
    if(!card)
        throw std::runtime_error("Not a MRM EVR");

    printf("Print ram #%d\n",ram);
    if(evt>=0){
        // Print a single event
        printRamEvt(card,evt,ram);
        return;
    }
    for(evt=0;evt<=255;evt++){
        printRamEvt(card,evt,ram);
    }
} catch(std::exception& e) {
    printf("Error: %s\n",e.what());
}
}

/** @brief Setup Event forwarding to downstream link
 *
 * Control which events will be forwarded to the downstream event link
 * when they are received on the upstream link.
 * Useful when daisy chaining EVRs.
 *
 * When invoked with the second argument as NULL or "" the current
 * forward mapping is printed.
 *
 * The second argument to this function is a comma seperated list of
 * event numbers and/or the special token 'all'.  If a token is prefixed
 * with '-' then the mapping is cleared, otherwise it is set.
 *
 * After a cold boot, no events are forwarded until/unless mrmrEvrForward
 * is called.
 *
 @code
   > mrmEvrForward("EVR1") # Prints current mappings
   Events forwarded: ...
   > mrmEvrForward("EVR1", "-all") # Clear all forward mappings
   # Clear all forward mappings except timestamp transmission
   > mrmEvrForward("EVR1", "-all, 0x70, 0x71, 0x7A, 0x7D")
   # Forward all except 42
   > mrmEvrForward("EVR1", "all, -42")
 @endcode
 *
 @param id EVR identifier
 @param events A string with a comma seperated list of event specifiers
 */
void
mrmEvrForward(const char* id, const char* events_iocsh)
{
    char *events=events_iocsh ? epicsStrDup(events_iocsh) : 0;
try {
    mrf::Object *obj=mrf::Object::getObject(id);
    if(!obj)
        throw std::runtime_error("Object not found");
    EVRMRM *card=dynamic_cast<EVRMRM*>(obj);
    if(!card)
        throw std::runtime_error("Not a MRM EVR");

    if(!events || strlen(events)==0) {
        // Just print current mappings
        printf("Events forwarded: ");
        for(unsigned int i=1; i<256; i++) {
            if(card->specialMapped(i, ActionEvtFwd)) {
                printf("%d ", i);
            }
        }
        printf("\n");
        free(events);
        return;
    }

    // update mappings

    const char sep[]=", ";
    char *save=0;

    for(char *tok=strtok_r(events, sep, &save);
        tok!=NULL;
        tok = strtok_r(0, sep, &save)
        )
    {
        if(strcmp(tok, "-all")==0) {
            for(unsigned int i=1; i<256; i++)
                card->specialSetMap(i, ActionEvtFwd, false);

        } else if(strcmp(tok, "all")==0) {
            for(unsigned int i=1; i<256; i++)
                card->specialSetMap(i, ActionEvtFwd, true);

        } else {
            char *end=0;
            long e=strtol(tok, &end, 0);
            if(*end || e==LONG_MAX || e==LONG_MIN) {
                printf("Unable to parse event spec '%s'\n", tok);
            } else if(e>255 || e<-255 || e==0) {
                printf("Invalid event %ld\n", e);
            } else if(e>0) {
                card->specialSetMap(e, ActionEvtFwd, true);
            } else if(e<0) {
                card->specialSetMap(-e, ActionEvtFwd, false);
            }

        }
    }


    free(events);
} catch(std::exception& e) {
    printf("Error: %s\n",e.what());
    free(events);
}
}


void
mrmEvrLoopback(const char* id, int rxLoopback, int txLoopback)
{
try {
    mrf::Object *obj=mrf::Object::getObject(id);
    if(!obj)
        throw std::runtime_error("Object not found");
    EVRMRM *card=dynamic_cast<EVRMRM*>(obj);
    if(!card){
        throw std::runtime_error("Not a MRM EVR");
    }

    epicsUInt32 control = NAT_READ32(card->base,Control);
    control &= ~(Control_txloop|Control_rxloop);
    if (rxLoopback) control |= Control_rxloop;
    if (txLoopback) control |= Control_txloop;
    NAT_WRITE32(card->base,Control, control);

} catch(std::exception& e) {
    printf("Error: %s\n",e.what());
}
}
