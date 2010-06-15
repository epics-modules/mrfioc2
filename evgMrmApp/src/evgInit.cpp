#include "evgInit.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

#include <epicsExport.h>
#include <iocsh.h>
#include <drvSup.h>

#include <mrfcsr.h>
#include <mrfCommonIO.h>

#include "evgRegMap.h"


static volatile epicsUInt8* CpuAddress;
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


extern "C"
epicsStatus
mrmEvgSetupVME (
	epicsInt32    CardNum,           // Logical card number
    epicsInt32    Slot,              // VME slot
    epicsUInt32   VmeAddress,        // Desired VME address in A24 space
    epicsInt32    IrqVector,         // Desired interrupt vector number
    epicsInt32    IrqLevel)          // Desired interrupt level
{
  	try {
		if(FindEvg(CardNum)) {
			errlogPrintf("EVG Identifier %d already used", CardNum);
			return -1;
		}

    	/*csr is VME-CSR space CPU address for the board*/
    	volatile unsigned char* csrCpuAddr = devCSRTestSlot(vmeEvgIDs,Slot,&info);

    	if(!csrCpuAddr){
      		errlogPrintf("No EVG in slot %d\n",Slot);
      		return -1;
    	}

    	printf("##### Setting up MRF EVG in VME Slot %d #####\n",Slot);
    	printf("Found Vendor: %08x\nBoard: %08x\nRevision: %08x\n",
				info.vendor, info.board, info.revision);

    	/*Setting the base address for Register Map on VME Board (EVR)*/
    	CSRSetBase(csrCpuAddr, 1, VmeAddress, VME_AM_STD_SUP_DATA);

    	/*Register VME address and get corresponding CPU address */
		std::string Description("EVG");
		std::ostringstream oss;
		oss<<CardNum;
		Description += oss.str();

    	int status = devRegisterAddress (
        		Description.c_str(),                    	// Event Generator Card name
          		atVMEA24,                          	// A24 Address space
            	VmeAddress,                       	// Physical address of register space
            	EVG_REGMAP_SIZE,                  	// Size of card's register space
            	(volatile void **)&CpuAddress);  	// Local address of card's register map


    	if(status) {
      		errlogPrintf("Failed to map VME address %08x\n", VmeAddress);
      		return -1;
    	}

		evgMrm* evg = new evgMrm(CardNum, CpuAddress);
		AddEvg(CardNum, evg);

    	/*Read version number of EVR board*/
    	fpgaVersion = READ32(CpuAddress, FPGAVersion);
    	printf("FPGA verion: %08x\n", fpgaVersion);

		BITSET32(CpuAddress, Control, EVG_MASTER_ENA);

    	return 0;
  	} catch(std::exception& e) {
    	errlogPrintf("Error: %s\n",e.what());
 	}
	return -1;

} //EgConfigureVME

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
REGINFO("InterruptFlag", 	InterruptFlag, 		32),
REGINFO("InterruptEnable", 	InterruptEnable,	32),
REGINFO("SwEventControl", 	SwEventControl, 	8),
REGINFO("SwEventCode", 		SwEventCode, 		8),
REGINFO("DBusMap",			DBusMap, 			32),
REGINFO("FPGAVersion", 		FPGAVersion, 		32),
REGINFO("uSecDiv", 			uSecDiv, 			16),
REGINFO("ClockSource", 		ClockSource, 		8),
REGINFO("RfDiv", 			RfDiv, 				8),
REGINFO("ClockStatus", 		ClockStatus, 		16),
REGINFO("FracSynthWord", 	FracSynthWord, 		32),
REGINFO("TrigEventCtrl(0)", TrigEventCtrl(0), 	32),
REGINFO("MuxControl(0)", 	MuxControl(0), 		32),
REGINFO("MuxPrescaler(0)", 	MuxPrescaler(0), 	32),
REGINFO("FPOutMap(0)", 		FPOutMap(0), 		16),
REGINFO("FPInMap(0)", 		FPInMap(0), 		32),
REGINFO("FPInMap(1)", 		FPInMap(1), 		32),
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
  		printregisters(CpuAddress);
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