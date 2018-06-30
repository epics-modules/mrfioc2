/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

#include <epicsExit.h>
#include <epicsThread.h>
#include <epicsStdio.h>
#include <epicsString.h>

#include <iocsh.h>
#include <drvSup.h>
#include <initHooks.h>
#include <errlog.h>

#include "mrf/object.h"
#include "mrf/databuf.h"
#include "mrmpci.h"

#include <devcsr.h>
/* DZ: Does Win32 have a problem with devCSRTestSlot()? */
#ifdef _WIN32
#define devCSRTestSlot(vmeEvgIDs,slot,info) (NULL)

#include <time.h>
#endif

#include <mrfcsr.h>
#include <mrfCommonIO.h>
#include <devLibPCI.h>
#include "plx9030.h"

#include <epicsExport.h>
#include "evgRegMap.h"

#include "evgInit.h"

/* Bit mask used to communicate which VME interrupt levels
 * are used.  Bits are set by mrmEvgSetupVME().  Levels are
 * enabled later during iocInit.
 */
static epicsUInt8 vme_level_mask = 0;

static const
struct VMECSRID vmeEvgIDs[] = {
{MRF_VME_IEEE_OUI,
    MRF_VME_EVG_BID|MRF_SERIES_230,
    VMECSRANY},
VMECSR_END
};

static bool
enableIRQ(mrf::Object* obj, void*) {
    evgMrm *evg=dynamic_cast<evgMrm*>(obj);
    if(!evg)
        return true;

    evg->enableIRQ();

    return true;
}

static bool
disableIRQ(mrf::Object* obj, void*)
{
    evgMrm *evg=dynamic_cast<evgMrm*>(obj);
    if(!evg)
        return true;

    BITCLR32(evg->getRegAddr(), IrqEnable, EVG_IRQ_ENABLE);
    return true;
}

static void
evgShutdown(void*)
{
    mrf::Object::visitObjects(&disableIRQ,0);
}

static void 
inithooks(initHookState state) {
    epicsUInt8 lvl;
    switch(state) {
    case initHookAfterInterruptAccept:
        epicsAtExit(&evgShutdown, NULL);
        mrf::Object::visitObjects(&enableIRQ, 0);
        for(lvl=1; lvl<=7; ++lvl) {
            if (vme_level_mask&(1<<(lvl-1))) {
                if(devEnableInterruptLevelVME(lvl)) {
                    printf("Failed to enable interrupt level %d\n",lvl);
                    return;
                }
            }
        }

        break;

        //Enable interrupts after IOC has been started (this is need for cPCI version)
    case initHookAtIocRun:
        epicsAtExit(&evgShutdown, NULL);
        mrf::Object::visitObjects(&enableIRQ, 0);
        break;

    default:
        break;
    }
}

void checkVersion(volatile epicsUInt8 *base, unsigned int required,
                  unsigned int recommended)
{
    epicsUInt32 type, ver;
    epicsUInt32 v = READ32(base, FPGAVersion);

    if(v & FPGAVersion_ZERO_MASK)
        throw std::runtime_error("Invalid firmware version (HW or bus error)");

    type = v & FPGAVersion_TYPE_MASK;
    type = v >> FPGAVersion_TYPE_SHIFT;

    if(type != 0x2)
        throw std::runtime_error("Address does not correspond to an EVG");

    ver = v & FPGAVersion_VER_MASK;

    if(ver < required) {
        printf("Firmware version >= %u is required\n", required);
        throw std::runtime_error("Firmware version not supported");

    } else if(ver < recommended) {
        printf("Firmware version >= %u is recommended, please consider upgrading\n", required);
    }
}

extern "C"
epicsStatus
mrmEvgSetupVME (
        const char* id,         // Card Identifier
        epicsInt32  slot,       // VME slot
        epicsUInt32 vmeAddress, // Desired VME address in A24 space
        epicsInt32  irqLevel,   // Desired interrupt level
        epicsInt32  irqVector)  // Desired interrupt vector number
{
    volatile epicsUInt8* regCpuAddr = 0;
    struct VMECSRID info;
    bus_configuration bus;

    info.board = 0; info.revision = 0; info.vendor = 0;

    bus.busType = busType_vme;
    bus.vme.slot = slot;
    bus.vme.address = vmeAddress;
    bus.vme.irqLevel = irqLevel;
    bus.vme.irqVector = irqVector;

    try {
        if(mrf::Object::getObject(id)){
            errlogPrintf("ID %s already in use\n",id);
            return -1;
        }

        /*csrCpuAddr is VME-CSR space CPU address for the board*/
        volatile unsigned char* csrCpuAddr =
                devCSRTestSlot(vmeEvgIDs,slot,&info);

        if(!csrCpuAddr) {
            errlogPrintf("No EVG in slot %d\n",slot);
            return -1;
        }

        printf("##### Setting up MRF EVG in VME Slot %d #####\n",slot);
        printf("Found Vendor: %08x\nBoard: %08x\nRevision: %08x\n",
               info.vendor, info.board, info.revision);
        
        epicsUInt32 xxx = CSRRead32(csrCpuAddr + CSR_FN_ADER(1));
        if(xxx)
            errlogPrintf("Warning: EVG not in power on state! (%08x)\n", xxx);

        /*Setting the base address of Register Map on VME Board (EVG)*/
        CSRSetBase(csrCpuAddr, 1, vmeAddress, VME_AM_STD_SUP_DATA);

        {
            epicsUInt32 temp=CSRRead32((csrCpuAddr) + CSR_FN_ADER(1));

            if(temp != CSRADER((epicsUInt32)vmeAddress,VME_AM_STD_SUP_DATA)) {
                printf("Failed to set CSR Base address in ADER1.  Check VME bus and card firmware version.\n");
                return -1;
            }
        }

        /* Create a static string for the card description (needed by vxWorks) */
        char *Description = allocSNPrintf(40, "EVG-%d '%s' slot %d",
                                          info.board & MRF_BID_SERIES_MASK,
                                          id, slot);

        /*Register VME address and get corresponding CPU address */
        int status = devRegisterAddress (
                    Description,                           // Event Generator card description
                    atVMEA24,                              // A24 Address space
                    vmeAddress,                            // Physical address of register space
                    EVG_REGMAP_SIZE,                       // Size of card's register space
                    (volatile void **)(void *)&regCpuAddr  // Local address of card's register map
                    );

        if(status) {
            errlogPrintf("Failed to map VME address %08x\n", vmeAddress);
            return -1;
        }

        {
            epicsUInt32 junk;
            if(devReadProbe(sizeof(junk), (volatile void*)(regCpuAddr+U32_FPGAVersion), (void*)&junk)) {
                printf("Failed to read from MRM registers (but could read CSR registers)\n");
                return -1;
            }
        }
        printf("FPGA version: %08x\n", READ32(regCpuAddr, FPGAVersion));
        checkVersion(regCpuAddr, 3, 3);

        evgMrm* evg = new evgMrm(id, bus, regCpuAddr, NULL);

        if(irqLevel > 0 && irqVector >= 0) {
            /*Configure the Interrupt level and vector on the EVG board*/
            CSRWrite8(csrCpuAddr + MRF_UCSR_DEFAULT + UCSR_IRQ_LEVEL, irqLevel&0x7);
            CSRWrite8(csrCpuAddr + MRF_UCSR_DEFAULT + UCSR_IRQ_VECTOR, irqVector&0xff);

            printf("IRQ Level: %d\nIRQ Vector: %d\n",
                   CSRRead8(csrCpuAddr + MRF_UCSR_DEFAULT + UCSR_IRQ_LEVEL),
                   CSRRead8(csrCpuAddr + MRF_UCSR_DEFAULT + UCSR_IRQ_VECTOR)
                   );


            printf("csrCpuAddr : %p\nregCpuAddr : %p\n",csrCpuAddr, regCpuAddr);

            /*Disable the interrupts and enable them at the end of iocInit via initHooks*/
            WRITE32(regCpuAddr, IrqFlag, READ32(regCpuAddr, IrqFlag));
            WRITE32(regCpuAddr, IrqEnable, 0);

            // VME IRQ level will be enabled later during iocInit()
            vme_level_mask |= 1 << ((irqLevel&0x7)-1);

            /*Connect Interrupt handler to vector*/
            if(devConnectInterruptVME(irqVector & 0xff, &evgMrm::isr_vme, evg)){
                errlogPrintf("ERROR:Failed to connect VME IRQ vector %d\n"
                             ,irqVector&0xff);
                delete evg;
                return -1;
            }
        }

        errlogFlush();
        return 0;
    } catch(std::exception& e) {
        errlogPrintf("Error: %s\n",e.what());
    }
    errlogFlush();
    return -1;
} //mrmEvgSetupVME

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
        errlogPrintf("Can't open %s in order to read kernel module interface version. Kernel module not loaded or too old.\n", ifaceversion);
        return true;
    }
    if(fscanf(fd, "%d", &version)!=1) {
        fclose(fd);
        errlogPrintf("Failed to read %s in order to get the kernel module interface version.\n", ifaceversion);
        return true;
    }
    fclose(fd);

    // Interface versions are *not* expected to be backwords or forwards compatible.
    if(version<vmin || version>vmax) {
        errlogPrintf("Error: Expect MRF kernel module interface version between [%d, %d], found %d.\n", vmin, vmax, version);
        return true;
    }
    if(actual)
        *actual = version;
    return false;
}
#else
static bool checkUIOVersion(int,int,int*) {return false;}
#endif


static const epicsPCIID
mrmevgs[] = {
    DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030, PCI_VENDOR_ID_PLX,PCI_SUBDEVICE_ID_MRF_PXIEVG_220, PCI_VENDOR_ID_MRF),
    DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030, PCI_VENDOR_ID_PLX,PCI_DEVICE_ID_MRF_PXIEVG230, PCI_VENDOR_ID_MRF),
    DEVPCI_DEVICE_VENDOR(PCI_DEVICE_ID_MRF_CPCIEVG300, PCI_VENDOR_ID_MRF),
    DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_XILINX_DEV, PCI_VENDOR_ID_XILINX, PCI_DEVICE_ID_MRF_MTCA_EVM_300, PCI_VENDOR_ID_MRF),
    DEVPCI_END
};

extern "C"
epicsStatus
mrmEvgSetupPCI (
        const char* id,         // Card Identifier
        const char *spec,   	// ID spec. or Bus number
        int d, 					// Device number
        int f)   				// Function number
{
    if(d!=0 || f!=0) {
        std::istringstream strm(spec);
        unsigned B =0xf;
        strm >> B;
        char buf[40];
        epicsSnprintf(buf, sizeof(buf), "%x:%x.%x", B, d, f);
        buf[sizeof(buf)-1] = '\0';
        spec = epicsStrDup(buf);
        fprintf(stderr, "Deprecated call.  Replace with:\n"
                        "  mrmEvgSetupPCI(\"%s\", \"%s\")\n",
                id, spec);
    }

    try {
        if (mrf::Object::getObject(id)) {
            errlogPrintf("ID %s already in use\n", id);
            return -1;
        }

        /* Linux only
         * kernel driver interface version.
         * 0 - Broken
         * 1 - Use of irqcontrol callback to avoid races for plx pci bridges
         * 2 - Use of new PCI master enable register to avoid races for soft pci bridges
         */
        int kifacever = -1;
        if(checkUIOVersion(1,2,&kifacever))
            return -1;

        /* Find PCI device from devLib2 */
        const epicsPCIDevice *cur = 0;
        if (devPCIFindSpec(mrmevgs, spec, &cur, 0)) {
            printf("PCI Device not found\n");
            return -1;
        }

        bus_configuration bus;

        bus.busType = busType_pci;
        bus.pci.dev = cur;

        printf("Device %s  %u:%u.%u\n", id, cur->bus, cur->device,
               cur->function);
        printf("Using IRQ %u\n", cur->irq);

        /* MMap BAR0(plx) and BAR2(EVG)*/
        volatile epicsUInt8 *BAR_plx, *BAR_evg;

        if (devPCIToLocalAddr(cur, 0, (volatile void**) (void *) &BAR_plx, 0)) {
            errlogPrintf("Failed to map BARs 0\n");
            return -1;
        }
        if (!BAR_plx) {
            errlogPrintf("BAR0 mapped to zero? (%08lx)\n",
                         (unsigned long) BAR_plx);
            return -1;
        }

        switch(cur->id.device) {
        case PCI_DEVICE_ID_PLX_9030: /* cPCI-EVG-220 and cPCI-EVG-230 */
            if (devPCIToLocalAddr(cur, 2, (volatile void**) (void *) &BAR_evg, 0)) {
                errlogPrintf("Failed to map BARs 2\n");
                return -1;
            }
            if (!BAR_evg) {
                errlogPrintf("BAR2 mapped to zero? (%08lx)\n",
                             (unsigned long) BAR_evg);
                return -1;
            }

#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
            BITSET(LE,32, BAR_plx, LAS0BRD, LAS0BRD_ENDIAN);
#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
            BITCLR(LE,32, BAR_plx, LAS0BRD, LAS0BRD_ENDIAN);
#endif
            break;
        case PCI_DEVICE_ID_MRF_CPCIEVG300:
        case PCI_DEVICE_ID_XILINX_DEV:
            BAR_evg = BAR_plx;
            /* the endianness the 300 series devices w/o PLX bridge
             * is a little tricky to setup.  byte order swapping is controlled
             * through the EVR's Control register and access to this register
             * is subject to byte order swapping...
             */

            // Disable EVG and set's byte order to big endian
            NAT_WRITE32(BAR_evg, Control, 0);
            // Disable reception, and enable byte order swapping if necessary
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
            BE_WRITE32(BAR_evg, Control, 0x70000000);
#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
            BE_WRITE32(BAR_evg, Control, 0x72000000);
#endif

            break;
        default:
            errlogPrintf("Unknown/unsupported PCI device 0x%04x\n", (unsigned)cur->device);
            return -1;
        }

        printf("FPGA version: %08x\n", READ32(BAR_evg, FPGAVersion));
        checkVersion(BAR_evg, 3, 8);

        /*Disable the interrupts and enable them at the end of iocInit via initHooks*/
        WRITE32(BAR_evg, IrqFlag, READ32(BAR_evg, IrqFlag));
        WRITE32(BAR_evg, IrqEnable, 0);

        evgMrm* evg = new evgMrm(id, bus, BAR_evg, cur);

#if !defined(__linux__) && !defined(_WIN32)
        if(cur->id.device==PCI_DEVICE_ID_PLX_9030) {
            // Enable active high interrupt1 through the PLX to the PCI bus.
            LE_WRITE16(BAR_plx, INTCSR,	INTCSR_INT1_Enable| INTCSR_INT1_Polarity| INTCSR_PCI_Enable);
        }
        if(evg->getFwVersionID()>=8) {
            // RTOS doesn't need this, so always enable
            WRITE32(BAR_evg, PCI_MIE, EVG_MIE_ENABLE);
        }
#else
        if(evg->getFwVersionID()>=8 && kifacever>=2) {
            // PCI master enable supported by firmware and kernel module.
            // the kernel will set this bit when devPCIEnableInterrupt() is called
        } else if(cur->id.device==PCI_DEVICE_ID_PLX_9030) {
            // PLX based devices don't need special handling
            WRITE32(BAR_evg, PCI_MIE, EVG_MIE_ENABLE);
        } else if(evg->getFwVersionID()<8) {
            // old firmware and (maybe) old kernel module.
            // this will still work, so just complain
            errlogPrintf("Warning: this configuration of FW and SW is known to have race conditions in interrupt handling.\n"
                         "         Please consider upgrading to FW version 8.\n");
            if(kifacever<2)
                errlogPrintf("         Also upgrade the linux kernel module to interface version 2.");
        } else if(evg->getFwVersionID()>=8 && kifacever<2) {
            // New firmware w/ old kernel module, this won't work
            throw std::runtime_error("FW version 8 for this device requires a linux kernel module w/ interface version 2");
        } else {
            throw std::logic_error("logic error in FW/kernel module compatibility check.");
        }
        if(devPCIEnableInterrupt(cur)) {
            printf("Failed to enable interrupt\n");
            return -1;
        }
#endif

#ifdef __linux__
        evg->isrLinuxPvt = (void*) cur;
#endif
        int ret;
        /*Connect Interrupt handler to isr thread*/
        if ((ret=devPCIConnectInterrupt(cur, &evgMrm::isr_pci, (void*) evg, 0))!=0) {//devConnectInterruptVME(irqVector & 0xff, &evgMrm::isr, evg)){
            char buf[80];
            errSymLookup(ret, buf, sizeof(buf));
            errlogPrintf("ERROR:Failed to connect PCI interrupt. err (%d) %s\n", ret, buf);
            delete evg;
            return -1;
        } else {
            printf("PCI interrupt connected!\n");
        }

        return 0;

    } catch (std::exception& e) {
        errlogPrintf("Error: %s\n", e.what());
    }
    return -1;
} //mrmEvgSetupPCI

static const iocshArg mrmEvgSetupVMEArg0 = { "Card ID", iocshArgString };
static const iocshArg mrmEvgSetupVMEArg1 = { "Slot number", iocshArgInt };
static const iocshArg mrmEvgSetupVMEArg2 = { "A24 base address", iocshArgInt };
static const iocshArg mrmEvgSetupVMEArg3 = { "IRQ Level 1-7 (0 - disable)",
                                             iocshArgInt };
static const iocshArg mrmEvgSetupVMEArg4 = { "IRQ Vector 0-255", iocshArgInt };
static const iocshArg * const mrmEvgSetupVMEArgs[5] = { &mrmEvgSetupVMEArg0,
                                                        &mrmEvgSetupVMEArg1,
                                                        &mrmEvgSetupVMEArg2,
                                                        &mrmEvgSetupVMEArg3,
                                                        &mrmEvgSetupVMEArg4 };

static const iocshFuncDef mrmEvgSetupVMEFuncDef = { "mrmEvgSetupVME", 5,
                                                    mrmEvgSetupVMEArgs };

static void 
mrmEvgSetupVMECallFunc(const iocshArgBuf *args) {
    mrmEvgSetupVME(args[0].sval,
            args[1].ival,
            args[2].ival,
            args[3].ival,
            args[4].ival);
}

static const iocshArg mrmEvgSetupPCIArg0 = { "Card ID", iocshArgString };
static const iocshArg mrmEvgSetupPCIArg1 = { "spec or B board", iocshArgString };
static const iocshArg mrmEvgSetupPCIArg2 = { "D device", iocshArgInt };
static const iocshArg mrmEvgSetupPCIArg3 = { "F function", iocshArgInt };

static const iocshArg * const mrmEvgSetupPCIArgs[4] = { &mrmEvgSetupPCIArg0,
                                                        &mrmEvgSetupPCIArg1, &mrmEvgSetupPCIArg2, &mrmEvgSetupPCIArg3 };

static const iocshFuncDef mrmEvgSetupPCIFuncDef = { "mrmEvgSetupPCI", 4,
                                                    mrmEvgSetupPCIArgs };

static void mrmEvgSetupPCICallFunc(const iocshArgBuf *args) {
    mrmEvgSetupPCI(args[0].sval, args[1].sval, args[2].ival, args[3].ival);

}

extern "C"{
static void evgMrmRegistrar() {
    initHookRegister(&inithooks);
    iocshRegister(&mrmEvgSetupVMEFuncDef, mrmEvgSetupVMECallFunc);
    iocshRegister(&mrmEvgSetupPCIFuncDef, mrmEvgSetupPCICallFunc);
}

epicsExportRegistrar(evgMrmRegistrar);
}

/*
 * EPICS Driver Support for this module
 */
static const
struct printreg {
    char label[18];
    epicsUInt32 offset;
    int rsize;
} printreg[] = {
#define REGINFO(label, name, size) {label, U##size##_##name, size}
    REGINFO("Status",           Status,           32),
    REGINFO("Control",          Control,          32),
    REGINFO("IrqFlag",          IrqFlag,          32),
    REGINFO("IrqEnable",        IrqEnable,        32),
    REGINFO("AcTrigControl",    AcTrigControl,    32),
    REGINFO("AcTrigMap",        AcTrigMap,        32),
    REGINFO("SwEvent",          SwEvent,          32),
    REGINFO("DataBufferControl",DataBufferControl,32),
    REGINFO("DBusSrc",          DBusSrc,          32),
    REGINFO("FPGAVersion",      FPGAVersion,      32),
    REGINFO("ClockControl",     ClockControl,     32),
    REGINFO("SeqControl(0)",    SeqControl(0),    32),
    REGINFO("SeqControl(1)",    SeqControl(1),    32),
    REGINFO("FracSynthWord",    FracSynthWord,    32),
    REGINFO("TrigEventCtrl(0)", TrigEventCtrl(0), 32),
    REGINFO("TrigEventCtrl(1)", TrigEventCtrl(1), 32),
    REGINFO("TrigEventCtrl(2)", TrigEventCtrl(2), 32),
    REGINFO("TrigEventCtrl(3)", TrigEventCtrl(3), 32),
    REGINFO("TrigEventCtrl(4)", TrigEventCtrl(4), 32),
    REGINFO("TrigEventCtrl(5)", TrigEventCtrl(5), 32),
    REGINFO("TrigEventCtrl(6)", TrigEventCtrl(6), 32),
    REGINFO("TrigEventCtrl(7)", TrigEventCtrl(7), 32),
    REGINFO("MuxControl(0)",    MuxControl(0),    32),
    REGINFO("MuxPrescaler(0)",  MuxPrescaler(0),  32),
    REGINFO("MuxControl(1)",    MuxControl(1),    32),
    REGINFO("MuxPrescaler(1)",  MuxPrescaler(1),  32),
    REGINFO("MuxControl(2)",    MuxControl(2),    32),
    REGINFO("MuxPrescaler(2)",  MuxPrescaler(2),  32),
    REGINFO("MuxControl(3)",    MuxControl(3),    32),
    REGINFO("MuxPrescaler(3)",  MuxPrescaler(3),  32),
    REGINFO("MuxControl(4)",    MuxControl(4),    32),
    REGINFO("MuxPrescaler(4)",  MuxPrescaler(4),  32),
    REGINFO("MuxControl(5)",    MuxControl(5),    32),
    REGINFO("MuxPrescaler(5)",  MuxPrescaler(5),  32),
    REGINFO("MuxControl(6)",    MuxControl(6),    32),
    REGINFO("MuxPrescaler(6)",  MuxPrescaler(6),  32),
    REGINFO("MuxControl(7)",    MuxControl(7),    32),
    REGINFO("MuxPrescaler(7)",  MuxPrescaler(7),  32),
    REGINFO("FrontOutMap(0)",   FrontOutMap(0),   16),
    REGINFO("FrontInMap(0)",    FrontInMap(0),    32),
    REGINFO("FrontInMap(1)",    FrontInMap(1),    32),
    REGINFO("UnivInMap(0)",     UnivInMap(0),     32),
    REGINFO("UnivInMap(1)",     UnivInMap(1),     32),
    REGINFO("RearInMap(12)",    RearInMap(12),    32),
    REGINFO("RearInMap(13)",    RearInMap(13),    32),
    REGINFO("RearInMap(14)",    RearInMap(14),    32),
    REGINFO("RearInMap(15)",    RearInMap(15),    32),
    REGINFO("DataBuffer(0)",    DataBuffer(0),     8),
    REGINFO("DataBuffer(1)",    DataBuffer(1),     8),
    REGINFO("DataBuffer(2)",    DataBuffer(2),     8),
    REGINFO("DataBuffer(3)",    DataBuffer(3),     8),
    REGINFO("DataBuffer(4)",    DataBuffer(4),     8),
    REGINFO("DataBuffer(5)",    DataBuffer(5),     8),
    REGINFO("SeqRamTS(0,0)",    SeqRamTS(0,0),    32),
    REGINFO("SeqRamTS(0,1)",    SeqRamTS(0,1),    32),
    REGINFO("SeqRamTS(0,2)",    SeqRamTS(0,2),    32),
    REGINFO("SeqRamTS(0,3)",    SeqRamTS(0,3),    32),
    REGINFO("SeqRamTS(0,4)",    SeqRamTS(0,4),    32),
    REGINFO("SeqRamEvent(0,0)", SeqRamEvent(0,0), 32),
    REGINFO("SeqRamEvent(0,1)", SeqRamEvent(0,1), 32),
    REGINFO("SeqRamEvent(0,2)", SeqRamEvent(0,2), 32),
    REGINFO("SeqRamEvent(0,3)", SeqRamEvent(0,3), 32),
    REGINFO("SeqRamEvent(0,4)", SeqRamEvent(0,4), 32),
    REGINFO("SeqRamTS(1,0)",    SeqRamTS(1,0),    32),
    REGINFO("SeqRamTS(1,1)",    SeqRamTS(1,1),    32),
    REGINFO("SeqRamTS(1,2)",    SeqRamTS(1,2),    32),
    REGINFO("SeqRamTS(1,3)",    SeqRamTS(1,3),    32),
    REGINFO("SeqRamTS(1,4)",    SeqRamTS(1,4),    32),
    REGINFO("SeqRamEvent(1,0)", SeqRamEvent(1,0), 32),
    REGINFO("SeqRamEvent(1,1)", SeqRamEvent(1,1), 32),
    REGINFO("SeqRamEvent(1,2)", SeqRamEvent(1,2), 32),
    REGINFO("SeqRamEvent(1,3)", SeqRamEvent(1,3), 32),
    REGINFO("SeqRamEvent(1,4)", SeqRamEvent(1,4), 32),
#undef REGINFO
};


static void
printregisters(volatile epicsUInt8 *evg) {
    size_t reg;
    printf("\n--- Register Dump @%p ---\n", evg);

    for(reg=0; reg<NELEMENTS(printreg); reg++){
        switch(printreg[reg].rsize){
        case 8:
            printf("%16s: %02x\n", printreg[reg].label,
                   ioread8(evg+printreg[reg].offset));
            break;
        case 16:
            printf("%16s: %04x\n", printreg[reg].label,
                   nat_ioread16(evg+printreg[reg].offset));
            break;
        case 32:
            printf("%16s: %08x\n", printreg[reg].label,
                   nat_ioread32(evg+printreg[reg].offset));
            break;
        }
    }
}

static bool
reportCard(mrf::Object* obj, void* arg) {
    int *level=(int*)arg;
    evgMrm *evg=dynamic_cast<evgMrm*>(obj);
    if(!evg)
        return true;

    printf("EVG: %s     \n", evg->getId().c_str());
    printf("\tFPGA Version: %08x (firmware: %x)\n", evg->getFwVersion(), evg->getFwVersionID());
    printf("\tForm factor: %s\n", evg->getFormFactorStr().c_str());

    const bus_configuration *bus = evg->getBusConfiguration();
    if(bus->busType == busType_vme){
        struct VMECSRID vmeDev;
        vmeDev.board = 0; vmeDev.revision = 0; vmeDev.vendor = 0;
        volatile unsigned char* csrAddr = devCSRTestSlot(vmeEvgIDs, bus->vme.slot, &vmeDev);
        if(csrAddr){
            epicsUInt32 ader = CSRRead32(csrAddr + CSR_FN_ADER(1));
            printf("\tVME configured slot: %d\n", bus->vme.slot);
            printf("\tVME configured A24 address 0x%08x\n", bus->vme.address);
            printf("\tVME ADER: base address=0x%x\taddress modifier=0x%x\n", ader>>8, (ader&0xFF)>>2);
            printf("\tVME IRQ Level %d (configured to %d)\n", CSRRead8(csrAddr + MRF_UCSR_DEFAULT + UCSR_IRQ_LEVEL), bus->vme.irqLevel);
            printf("\tVME IRQ Vector %d (configured to %d)\n", CSRRead8(csrAddr + MRF_UCSR_DEFAULT + UCSR_IRQ_VECTOR), bus->vme.irqVector);
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

    evg->show(*level);
    
    if(*level >= 2)
        printregisters(evg->getRegAddr());

    printf("\n");
    return true;
}

static long 
report(int level) {
    printf("===  Begin MRF EVG support   ===\n");
    mrf::Object::visitObjects(&reportCard, (void*)&level);
    printf("===   End MRF EVG support    ===\n");
    return 0;
}
extern "C"{
static
drvet drvEvgMrm = {
    2,
    (DRVSUPFUN)report,
    NULL
};
epicsExportAddress (drvet, drvEvgMrm);
}
