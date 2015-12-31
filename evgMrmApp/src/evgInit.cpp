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

#include <iocsh.h>
#include <drvSup.h>
#include <initHooks.h>
#include <errlog.h>

#include "mrf/object.h"
#include "mrf/databuf.h"

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

	WRITE32(evg->getRegAddr(), IrqEnable,
             EVG_IRQ_PCIIE          | //PCIe interrupt enable,
             EVG_IRQ_ENABLE         |
             EVG_IRQ_EXT_INP        |
             EVG_IRQ_STOP_RAM(0)    |
             EVG_IRQ_STOP_RAM(1)    |
             EVG_IRQ_START_RAM(0)   |
             EVG_IRQ_START_RAM(1)
    );

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
bool checkUIOVersion(int expect)
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
    if(version!=expect) {
        errlogPrintf("Error: Expect MRF kernel module version %d, found %d.\n", version, expect);
        return true;
    }
    return false;
}
#else
static bool checkUIOVersion(int) {return false;}
#endif


static const epicsPCIID
mrmevgs[] = {
		DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030, PCI_VENDOR_ID_PLX,PCI_DEVICE_ID_MRF_PXIEVG230, PCI_VENDOR_ID_MRF),
		DEVPCI_END };

extern "C"
epicsStatus
mrmEvgSetupPCI (
		const char* id,         // Card Identifier
		int b,       			// Bus number
		int d, 					// Device number
		int f)   				// Function number
{
    bus_configuration bus;

    bus.busType = busType_pci;
    bus.pci.bus = b;
    bus.pci.device = d;
    bus.pci.function = f;

#if EPICS_BYTE_ORDER != EPICS_ENDIAN_LITTLE
    printf("Warning: PCI EVG untested on bigendian hosts!\n");
#endif

	try {
		if (mrf::Object::getObject(id)) {
			errlogPrintf("ID %s already in use\n", id);
			return -1;
		}

        if(checkUIOVersion(1))
            return -1;

		/* Find PCI device from devLib2 */
		const epicsPCIDevice *cur = 0;
		if (devPCIFindBDF(mrmevgs, b, d, f, &cur, 0)) {
			printf("PCI Device not found\n");
			return -1;
		}

		printf("Device %s  %u:%u.%u\n", id, cur->bus, cur->device,
				cur->function);
		printf("Using IRQ %u\n", cur->irq);

		/* MMap BAR0(plx) and BAR2(EVG)*/
		volatile epicsUInt8 *BAR_plx, *BAR_evg;

		if (devPCIToLocalAddr(cur, 0, (volatile void**) (void *) &BAR_plx, 0)
				|| devPCIToLocalAddr(cur, 2, (volatile void**) (void *) &BAR_evg, 0)) {
			errlogPrintf("Failed to map BARs 0 and 2\n");
			return -1;
		}
		if (!BAR_plx || !BAR_evg) {
			errlogPrintf("BARs mapped to zero? (%08lx,%08lx)\n",
					(unsigned long) BAR_plx, (unsigned long) BAR_evg);
			return -1;
		}

#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
        BITSET(LE,32, BAR_plx, LAS0BRD, LAS0BRD_ENDIAN);
#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
        BITCLR(LE,32, BAR_plx, LAS0BRD, LAS0BRD_ENDIAN);
#endif
		printf("FPGA version: %08x\n", READ32(BAR_evg, FPGAVersion));
		checkVersion(BAR_evg, 3, 3);

        evgMrm* evg = new evgMrm(id, bus, BAR_evg, cur);

		evg->getSeqRamMgr()->getSeqRam(0)->disable();
		evg->getSeqRamMgr()->getSeqRam(1)->disable();


		/*Disable the interrupts and enable them at the end of iocInit via initHooks*/
		WRITE32(BAR_evg, IrqFlag, READ32(BAR_evg, IrqFlag));
		WRITE32(BAR_evg, IrqEnable, 0);

#if !defined(__linux__) && !defined(_WIN32)
		/*
		 * Enable active high interrupt1 through the PLX to the PCI bus.
         */
        LE_WRITE16(BAR_plx, INTCSR,	INTCSR_INT1_Enable| INTCSR_INT1_Polarity| INTCSR_PCI_Enable);
#else
        if(devPCIEnableInterrupt(cur)) {
            printf("Failed to enable interrupt\n");
            return -1;
        }
#endif

#ifdef __linux__
        evg->isrLinuxPvt = (void*) cur;
#endif

		/*Connect Interrupt handler to isr thread*/
		if (devPCIConnectInterrupt(cur, &evgMrm::isr_pci, (void*) evg, 0)) {//devConnectInterruptVME(irqVector & 0xff, &evgMrm::isr, evg)){
			errlogPrintf("ERROR:Failed to connect PCI interrupt\n");
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

#if !defined(_WIN32) && !defined(__rtems__)
/*
 * This function spawns additional thread that emulate PPS input. Function is used for
 * testing of timestamping functionality... DO NOT USE IN PRODUCTION!!!!!
 * 
 * Change by: tslejko
 * Reason: testing utilities 
 */
void mrmEvgSoftTime(void* pvt) {
	evgMrm* evg = static_cast<evgMrm*>(pvt);

	if (!evg) {
		errlogPrintf("mrmEvgSoftTimestamp: Could not find EVG!\n");
	}

	while (1) {
		epicsUInt32 data = evg->sendTimestamp();
		if (!data){
			errlogPrintf("mrmEvgSoftTimestamp: Could not retrive timestamp...\n");
			epicsThreadSleep(1);
			continue;
		}

		//Send out event reset
		evg->getSoftEvt()->setEvtCode(MRF_EVENT_TS_COUNTER_RST);

		//Clock out data...
		for (int i = 0; i < 32; data <<= 1, i++) {
			if (data & 0x80000000)
				evg->getSoftEvt()->setEvtCode(MRF_EVENT_TS_SHIFT_1);
			else
				evg->getSoftEvt()->setEvtCode(MRF_EVENT_TS_SHIFT_0);
		}

		struct timespec sleep_until_t;

		clock_gettime(CLOCK_REALTIME,&sleep_until_t); //Get current time
		/* Sleep until next full second */
		sleep_until_t.tv_nsec=0;
		sleep_until_t.tv_sec++;

		clock_nanosleep(CLOCK_REALTIME,TIMER_ABSTIME,&sleep_until_t,0);


//		sleep(1);
	}
}
#else
void mrmEvgSoftTime(void* pvt) {}
#endif

/*
 *    EPICS Registrar Function for this Module
 */
static const iocshArg mrmEvgSoftTimeArg0 = { "Card ID", iocshArgString};
static const iocshArg * const mrmEvgSoftTimeArgs[1] = { &mrmEvgSoftTimeArg0};
static const iocshFuncDef mrmEvgSoftTimeFuncDef = { "mrmEvgSoftTime", 1, mrmEvgSoftTimeArgs };

static void mrmEvgSoftTimeFunc(const iocshArgBuf *args) {
	printf("Starting EVG Software based time provider...\n");

	if(!args[0].sval) return;

	evgMrm* evg = dynamic_cast<evgMrm*>(mrf::Object::getObject(args[0].sval));
	if(!evg){
		errlogPrintf("EVG <%s> does not exist!\n",args[0].sval);
	}

	epicsThreadCreate("EVG_TimestampTestThread",90, epicsThreadStackSmall,mrmEvgSoftTime,static_cast<void*>(evg));
}


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
static const iocshArg mrmEvgSetupPCIArg1 = { "B board", iocshArgInt };
static const iocshArg mrmEvgSetupPCIArg2 = { "D device", iocshArgInt };
static const iocshArg mrmEvgSetupPCIArg3 = { "F function", iocshArgInt };

static const iocshArg * const mrmEvgSetupPCIArgs[4] = { &mrmEvgSetupPCIArg0,
		&mrmEvgSetupPCIArg1, &mrmEvgSetupPCIArg2, &mrmEvgSetupPCIArg3 };

static const iocshFuncDef mrmEvgSetupPCIFuncDef = { "mrmEvgSetupPCI", 4,
		mrmEvgSetupPCIArgs };

static void mrmEvgSetupPCICallFunc(const iocshArgBuf *args) {
	mrmEvgSetupPCI(args[0].sval, args[1].ival, args[2].ival, args[3].ival);

}

extern "C"{
static void evgMrmRegistrar() {
	initHookRegister(&inithooks);
	iocshRegister(&mrmEvgSetupVMEFuncDef, mrmEvgSetupVMECallFunc);
	iocshRegister(&mrmEvgSetupPCIFuncDef, mrmEvgSetupPCICallFunc);
	iocshRegister(&mrmEvgSoftTimeFuncDef, mrmEvgSoftTimeFunc);

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

    printf("EVG: %s     \n", evg->getId().c_str());
    printf("\tFPGA Version: %08x (firmware: %x)\n", evg->getFwVersion(), evg->getFwVersionID());
    printf("\tForm factor: %s\n", evg->getFormFactorStr().c_str());

    bus_configuration *bus = evg->getBusConfiguration();
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
        const epicsPCIDevice *pciDev;
        if(!devPCIFindBDF(mrmevgs, bus->pci.bus, bus->pci.device, bus->pci.function, &pciDev, 0)){
            printf("\tPCI configured bus: 0x%08x\n", bus->pci.bus);
            printf("\tPCI configured device: 0x%08x\n", bus->pci.device);
            printf("\tPCI configured function: 0x%08x\n", bus->pci.function);
            printf("\tPCI IRQ: %u\n", pciDev->irq);
            if(*level>1) printf("\tPCI class: 0x%08x, revision: 0x%x, sub device: 0x%x, sub vendor: 0x%x\n", pciDev->id.pci_class, pciDev->id.revision, pciDev->id.sub_device, pciDev->id.sub_vendor);

        }else{
            printf("\tPCI Device not found\n");
        }
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
