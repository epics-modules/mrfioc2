#include "evgInit.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

#include <epicsExport.h>
#include <iocsh.h>
#include <drvSup.h>
#include <initHooks.h>

#include <mrfcsr.h>
#include <mrfCommonIO.h>

#include "evgRegMap.h"


static volatile epicsUInt8* regCpuAddr;
static struct VMECSRDevice info;
static epicsUInt32 fpgaVersion;


static const struct VMECSRDevice vmeEvgIDs[] = {
	{MRF_VME_IEEE_OUI, MRF_VME_EVG_BID|MRF_SERIES_230, VMECSRANY}
   	,VMECSR_END
};

typedef std::map<epicsUInt32, evgMrm*> EvgList_t;
EvgList_t EvgList;

void
AddEvg(epicsUInt32 id, evgMrm* evg) {
	if(!evg)
    	throw std::runtime_error("AddEvg: Can not store NULL.\
      						 (initialization probably failed)");

  	EvgList[id] = evg;
}

evgMrm*
FindEvg(epicsUInt32 id) {
	EvgList_t::const_iterator it = EvgList.find(id);

  	if(it == EvgList.end())
    	return 0;

	return it->second;
}

static
int
enableIRQ(volatile epicsUInt8* pReg) {
  if(!pReg)
    return 0;

  WRITE32(pReg, IrqEnable,
       	EVG_IRQ_ENABLE 		|
		EVG_IRQ_STOP_RAM1 	|
		EVG_IRQ_STOP_RAM0 	|
		EVG_IRQ_START_RAM1 	|
		EVG_IRQ_START_RAM0 	|
		//EVG_IRQ_DBUFF 		|
		//EVG_IRQ_FIFO		|
		EVG_IRQ_RXVIO
  );

  return 0;
}

static void 
inithooks(initHookState state) {
  switch(state)
  {
  case initHookAfterInterruptAccept:
    enableIRQ(regCpuAddr);
    break;
  default:
    break;
  }
}

extern "C"
epicsStatus
mrmEvgSetupVME (
	epicsInt32    cardNum,           // Logical card number
    epicsInt32    slot,              // VME slot
    epicsUInt32   vmeAddress,        // Desired VME address in A24 space
    epicsInt32    irqVector,         // Desired interrupt vector number
    epicsInt32    irqLevel)          // Desired interrupt level
{
  	try {
		if(FindEvg(cardNum)) {
			errlogPrintf("EVG Identifier %d already used", cardNum);
			return -1;
		}

    	/*csrCpuAddr is VME-CSR space CPU address for the board*/
    	volatile unsigned char* csrCpuAddr = devCSRTestSlot(vmeEvgIDs,slot,&info);

    	if(!csrCpuAddr){
      		errlogPrintf("No EVG in slot %d\n",slot);
      		return -1;
    	}

    	printf("##### Setting up MRF EVG in VME Slot %d #####\n",slot);
    	printf("Found Vendor: %08x\nBoard: %08x\nRevision: %08x\n",
				info.vendor, info.board, info.revision);

    	/*Setting the base address for Register Map on VME Board (EVG)*/
    	CSRSetBase(csrCpuAddr, 1, vmeAddress, VME_AM_STD_SUP_DATA);
    	
		std::string Description("EVG");
		std::ostringstream oss;
		oss<<cardNum;
		Description += oss.str();

		/*Register VME address and get corresponding CPU address */
    	int status = devRegisterAddress (
        		Description.c_str(),          	// Event Generator Card name
          		atVMEA24,                      	// A24 Address space
            	vmeAddress,                    	// Physical address of register space
            	EVG_REGMAP_SIZE,              	// Size of card's register space
            	(volatile void **)&regCpuAddr	// Local address of card's register map
		);	

    	if(status) {
      		errlogPrintf("Failed to map VME address %08x\n", vmeAddress);
      		return -1;
    	}

    	fpgaVersion = READ32(regCpuAddr, FPGAVersion);
    	printf("FPGA verion: %08x\n", fpgaVersion);

		evgMrm* evg = new evgMrm(cardNum, regCpuAddr);
		AddEvg(cardNum, evg);
		
		if(irqLevel > 0 && irqVector >= 0) {
			/*Configure the Interrupt level and vector on the EVG board*/
    		CSRWrite8(csrCpuAddr + 0x7fb03 + UCSR_IRQ_LEVEL,  irqLevel&0x7);
    		CSRWrite8(csrCpuAddr + 0x7fb03 + UCSR_IRQ_VECTOR, irqVector&0xff);

    		printf("IRQ Level: %d\nIRQ Vector: %d\n",
      			CSRRead8(csrCpuAddr + 0x7fb03 + UCSR_IRQ_LEVEL),
      			CSRRead8(csrCpuAddr + 0x7fb03 + UCSR_IRQ_VECTOR)
 			);

			printf("csrCpuAddr : %p\nregCpuAddr : %p\n",csrCpuAddr, regCpuAddr);

			/*Disable the interrupts and enable them at the end of iocInit via initHooks*/
			WRITE32(regCpuAddr, IrqEnable, 0);
			WRITE32(regCpuAddr, IrqFlag, READ32(regCpuAddr, IrqFlag));

			/*Enable interrupt from VME to CPU*/
    		if(devEnableInterruptLevelVME(irqLevel&0x7))
    		{
      			errlogPrintf("ERROR:Failed to enable VME interrupt level %d\n",irqLevel&0x7);
      			delete evg;
				return -1;
    		}
	
			/*Connect Interrupt handler to vector*/
    		if(devConnectInterruptVME(irqVector & 0xff, &evgMrm::isr, evg))
    		{
      			errlogPrintf("ERROR:Failed to connect VME IRQ vector %d\n",irqVector&0xff);
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
 *  EPICS Registrar Function for this Module
 */

static const iocshArg mrmEvgSetupVMEArg0 = { "Card number",iocshArgInt};
static const iocshArg mrmEvgSetupVMEArg1 = { "Slot number",iocshArgInt};
static const iocshArg mrmEvgSetupVMEArg2 = { "A24 base address",iocshArgInt};
static const iocshArg mrmEvgSetupVMEArg3 = { "IRQ Level 1-7 (0 - disable)",iocshArgInt};
static const iocshArg mrmEvgSetupVMEArg4 = { "IRQ Vector 0-255",iocshArgInt};
static const iocshArg * const mrmEvgSetupVMEArgs[5] = {	&mrmEvgSetupVMEArg0,
													 	&mrmEvgSetupVMEArg1,
														&mrmEvgSetupVMEArg2,
														&mrmEvgSetupVMEArg3,
														&mrmEvgSetupVMEArg4 };
static const iocshFuncDef mrmEvgSetupVMEFuncDef = {	"mrmEvgSetupVME",
													5,
													mrmEvgSetupVMEArgs };
static void mrmEvgSetupVMECallFunc(const iocshArgBuf *args) {
    mrmEvgSetupVME(args[0].ival,args[1].ival,args[2].ival,args[3].ival,args[4].ival);
}

static
void evgMrmRegistrar()
{
	initHookRegister(&inithooks);
  	iocshRegister(&mrmEvgSetupVMEFuncDef,mrmEvgSetupVMECallFunc);
}

epicsExportRegistrar(evgMrmRegistrar);


/*
 * EPICS Driver Support for this module
 */

static const
struct printreg
{
  char label[17];
  epicsUInt32 offset;
  int rsize;
} printreg[] = {
#define REGINFO(label, name, size) {label, U##size##_##name, size}
REGINFO("Status", 			Status, 			32),
REGINFO("Control", 			Control, 			32),
REGINFO("IrqFlag", 			IrqFlag, 			32),
REGINFO("IrqEnable", 		IrqEnable,			32),
REGINFO("SwEventControl", 	SwEventControl, 	8),
REGINFO("SwEventCode", 		SwEventCode, 		8),
REGINFO("DBusMap",			DBusMap, 			32),
REGINFO("FPGAVersion", 		FPGAVersion, 		32),
REGINFO("uSecDiv", 			uSecDiv, 			16),
REGINFO("ClockSource", 		ClockSource, 		8),
REGINFO("RfDiv", 			RfDiv, 				8),
REGINFO("ClockStatus", 		ClockStatus, 		16),
REGINFO("SeqControl(1)", 	SeqControl(1), 		32),
REGINFO("SeqControl(2)", 	SeqControl(2), 		32),
REGINFO("FracSynthWord", 	FracSynthWord, 		32),
REGINFO("TrigEventCtrl(0)", TrigEventCtrl(0), 	32),
REGINFO("MuxControl(0)", 	MuxControl(0), 		32),
REGINFO("MuxPrescaler(0)", 	MuxPrescaler(0), 	32),
REGINFO("FPOutMap(0)", 		FPOutMap(0), 		16),
REGINFO("FPInMap(0)", 		FPInMap(0), 		32),
REGINFO("FPInMap(1)", 		FPInMap(1), 		32),
REGINFO("SeqRamTS(1,0)", 	SeqRamTS(1,0), 		32),
REGINFO("SeqRamTS(1,1)", 	SeqRamTS(1,1), 		32),
REGINFO("SeqRamTS(1,2)", 	SeqRamTS(1,2), 		32),
REGINFO("SeqRamTS(1,3)", 	SeqRamTS(1,3), 		32),
REGINFO("SeqRamTS(1,4)", 	SeqRamTS(1,4), 		32),
REGINFO("SeqRamEvent(1,0)", SeqRamEvent(1,0), 	8),
REGINFO("SeqRamEvent(1,1)", SeqRamEvent(1,1), 	8),
REGINFO("SeqRamEvent(1,2)", SeqRamEvent(1,2), 	8),
REGINFO("SeqRamEvent(1,3)", SeqRamEvent(1,3), 	8),
REGINFO("SeqRamEvent(1,4)", SeqRamEvent(1,4), 	8),
REGINFO("SeqRamTS(2,0)", 	SeqRamTS(2,0),		32),
REGINFO("SeqRamTS(2,1)", 	SeqRamTS(2,1),		32),
REGINFO("SeqRamTS(2,2)", 	SeqRamTS(2,2),		32),
REGINFO("SeqRamTS(2,3)", 	SeqRamTS(2,3),		32),
REGINFO("SeqRamTS(2,4)", 	SeqRamTS(2,4),		32),
REGINFO("SeqRamEvent(2,0)", SeqRamEvent(2,0), 	8),
REGINFO("SeqRamEvent(2,1)", SeqRamEvent(2,1), 	8),
REGINFO("SeqRamEvent(2,2)", SeqRamEvent(2,2), 	8),
REGINFO("SeqRamEvent(2,3)", SeqRamEvent(2,3), 	8),
REGINFO("SeqRamEvent(2,4)", SeqRamEvent(2,4), 	8),
#undef REGINFO
};

static
void
printregisters(volatile epicsUInt8 *evg)
{
    size_t reg;
	printf("\n--- Register Dump @%p	---\n", evg);

    for(reg=0; reg<NELEMENTS(printreg); reg++){

      switch(printreg[reg].rsize){
      case 8:
        printf("%16s: %02x\n",
                printreg[reg].label,
                ioread8(evg+printreg[reg].offset));
        break;
      case 16:
        printf("%16s: %04x\n",
               printreg[reg].label,
               nat_ioread16(evg+printreg[reg].offset));
        break;
      case 32:
        printf("%16s: %08x\n",
               printreg[reg].label,
               nat_ioread32(evg+printreg[reg].offset));
        break;
      }
    }
}


static
long report(int level)
{
	
	printf("\n===  	MRF EVG	===\n");

	if(level >= 1) {
		printf("Vendor ID: %08x\nBoard ID: %08x\nRevision: %08x\n",
				info.vendor, info.board, info.revision);
		printf("FPGA verion: %08x\n", fpgaVersion);
	}
	
	if(level >= 2) {
  		printregisters(regCpuAddr);
  	}
	printf("\n");
	return 0;
}

static
drvet drvEvgMrm = {
    2,
    (DRVSUPFUN)report,
    NULL
};
epicsExportAddress (drvet, drvEvgMrm);