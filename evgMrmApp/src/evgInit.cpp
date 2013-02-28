#include "evgInit.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

#include <epicsExit.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <drvSup.h>
#include <initHooks.h>
#include <errlog.h>

#include <mrfcsr.h>
#include <mrfCommonIO.h>

#include "evgRegMap.h"

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

    WRITE32(evg->getRegAddr(), IrqEnable, EVG_IRQ_ENABLE|EVG_IRQ_EXT_INP|
                                     EVG_IRQ_STOP_RAM(0)|EVG_IRQ_STOP_RAM(1));
//     WRITE32(pReg, IrqEnable,
//         EVG_IRQ_ENABLE        |
//         EVG_IRQ_STOP_RAM1     |
//         EVG_IRQ_STOP_RAM0     |
//         EVG_IRQ_START_RAM1    |
//         EVG_IRQ_START_RAM0    |
//         EVG_IRQ_EXT_INP       |
//         EVG_IRQ_DBUFF         |
//         EVG_IRQ_FIFO          |
//         EVG_IRQ_RXVIO             
//    );

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
        default:
            break;
    }
}

void
checkVersion(volatile epicsUInt8 *base, unsigned int required, unsigned int recommended) {
    epicsUInt32 junk;
    if(devReadProbe(sizeof(junk), (volatile void*)(base+U32_FPGAVersion), (void*)&junk)) {
        printf("Failed to read from MRM registers (but could read CSR registers)\n");
        return;
    }
    epicsUInt32 type, ver;
    epicsUInt32 v = READ32(base, FPGAVersion);
    printf("FPGAVersion 0x%08x\n", v);

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
        
        epicsUInt32 xxx;
        if((xxx = CSRRead32(csrCpuAddr + CSR_FN_ADER(1)))) 
            errlogPrintf("Warning: EVG did not reboot properly %08x\n", xxx);
        else {
            /*Setting the base address of Register Map on VME Board (EVG)*/
            CSRSetBase(csrCpuAddr, 1, vmeAddress, VME_AM_STD_SUP_DATA);
        }

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

        printf("FPGA version: %08x\n", READ32(regCpuAddr, FPGAVersion));
        checkVersion(regCpuAddr, 3, 3);

        evgMrm* evg = new evgMrm(id, regCpuAddr);

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
            if(devConnectInterruptVME(irqVector & 0xff, &evgMrm::isr, evg)){
                errlogPrintf("ERROR:Failed to connect VME IRQ vector %d\n"
                                                         ,irqVector&0xff);
                delete evg;
                return -1;
            }    
        }

        return 0;
    } catch(std::exception& e) {
        errlogPrintf("Error: %s\n",e.what());
    }
    return -1;
} //mrmEvgSetupVME

/*
 *    EPICS Registrar Function for this Module
 */

static const iocshArg mrmEvgSetupVMEArg0 = { "Card ID",iocshArgString};
static const iocshArg mrmEvgSetupVMEArg1 = { "Slot number",iocshArgInt};
static const iocshArg mrmEvgSetupVMEArg2 = { "A24 base address",iocshArgInt};
static const iocshArg mrmEvgSetupVMEArg3 = { "IRQ Level 1-7 (0 - disable)"
                                              ,iocshArgInt};
static const iocshArg mrmEvgSetupVMEArg4 = { "IRQ Vector 0-255",iocshArgInt};
static const iocshArg * const mrmEvgSetupVMEArgs[5] = { &mrmEvgSetupVMEArg0,
                                                        &mrmEvgSetupVMEArg1,
                                                        &mrmEvgSetupVMEArg2,
                                                        &mrmEvgSetupVMEArg3,
                                                        &mrmEvgSetupVMEArg4 };

static const iocshFuncDef mrmEvgSetupVMEFuncDef = { "mrmEvgSetupVME",
                                                    5,
                                                    mrmEvgSetupVMEArgs };

static void 
mrmEvgSetupVMECallFunc(const iocshArgBuf *args) {
    mrmEvgSetupVME(args[0].sval,
                   args[1].ival,
                   args[2].ival,
                   args[3].ival,
                   args[4].ival);
}

static void 
evgMrmRegistrar() {
    initHookRegister(&inithooks);
    iocshRegister(&mrmEvgSetupVMEFuncDef,mrmEvgSetupVMECallFunc);
}

epicsExportRegistrar(evgMrmRegistrar);


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
REGINFO("AcTrigEvtMap",     AcTrigEvtMap,      8),
REGINFO("SwEventControl",   SwEventControl,    8),
REGINFO("SwEventCode",      SwEventCode,       8),
REGINFO("DataBufferControl",DataBufferControl,32),
REGINFO("DBusSrc",          DBusSrc,          32),
REGINFO("FPGAVersion",      FPGAVersion,      32),
REGINFO("uSecDiv",          uSecDiv,          16),
REGINFO("ClockSource",      ClockSource,       8),
REGINFO("RfDiv",            RfDiv,             8),
REGINFO("ClockStatus",      ClockStatus,      16),
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
REGINFO("SeqRamEvent(0,0)", SeqRamEvent(0,0),  8),
REGINFO("SeqRamEvent(0,1)", SeqRamEvent(0,1),  8),
REGINFO("SeqRamEvent(0,2)", SeqRamEvent(0,2),  8),
REGINFO("SeqRamEvent(0,3)", SeqRamEvent(0,3),  8),
REGINFO("SeqRamEvent(0,4)", SeqRamEvent(0,4),  8),
REGINFO("SeqRamTS(1,0)",    SeqRamTS(1,0),    32),
REGINFO("SeqRamTS(1,1)",    SeqRamTS(1,1),    32),
REGINFO("SeqRamTS(1,2)",    SeqRamTS(1,2),    32),
REGINFO("SeqRamTS(1,3)",    SeqRamTS(1,3),    32),
REGINFO("SeqRamTS(1,4)",    SeqRamTS(1,4),    32),
REGINFO("SeqRamEvent(1,0)", SeqRamEvent(1,0),  8),
REGINFO("SeqRamEvent(1,1)", SeqRamEvent(1,1),  8),
REGINFO("SeqRamEvent(1,2)", SeqRamEvent(1,2),  8),
REGINFO("SeqRamEvent(1,3)", SeqRamEvent(1,3),  8),
REGINFO("SeqRamEvent(1,4)", SeqRamEvent(1,4),  8),
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

    printf("    ID: %s     \n", evg->getId().c_str());
    
    printf("    FPGA version: %08x\n", evg->getFwVersion());

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

static
drvet drvEvgMrm = {
    2,
    (DRVSUPFUN)report,
    NULL
};
epicsExportAddress (drvet, drvEvgMrm);

