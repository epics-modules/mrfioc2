
#include "drvemIocsh.h"

#include <cstdio>

#include <stdexcept>
#include <map>

#include <iocsh.h>
#include <epicsExport.h>

#include <devLibPCI.h>
#include <epicsInterrupt.h>
#include "mrmpci.h"

#include "drvem.h"
#include "evrRegMap.h"
#include "plx9030.h"

#include <cardmap.h>

#include <mrfCommonIO.h>
#include <mrfBitOps.h>

extern "C" {
int evrmrmVerb=1;
epicsExportAddress(int,evrmrmVerb);
}

#define DBG evrmrmVerb

static const epicsPCIID mrmevrs[] = {
   DEVPCI_SUBDEVICE_SUBVENDOR(PCI_DEVICE_ID_PLX_9030,    PCI_VENDOR_ID_PLX,
                              PCI_DEVICE_ID_MRF_EVR_230, PCI_VENDOR_ID_MRF)
  ,DEVPCI_END
};

static
void
setupPCI(int id,int b,int d,int f)
{
  const epicsPCIDevice *cur=0;
  if( devPCIFindBDF(mrmevrs,b,d,f,&cur) ){
    printf("PCI Device not found\n");
    return;
  }

  printf("Device %u  %u:%u.%u\n",id,cur->bus,cur->device,cur->function);

  volatile epicsUInt8 *plx, *evr;

  if( devPCIToLocalAddr(cur,0,(volatile void**)&plx) ||
      devPCIToLocalAddr(cur,2,(volatile void**)&evr))
  {
      printf("Failed to map BARs 0 and 2\n");
      return;
  }
  if(!plx || !evr){
      printf("BARs mapped to zero? (%08lx,%08lx)\n",
              (unsigned long)plx,(unsigned long)evr);
      return;
  }

  /* Use the PLX device on the EVR to swap access on
   * little endian systems so we don't have no worry about
   * byte order :)
   */
#if EPICS_BYTE_ORDER == EPICS_ENDIAN_BIG
  BITSET(LE,32, plx, LAS0BRD, LAS0BRD_ENDIAN);
#elif EPICS_BYTE_ORDER == EPICS_ENDIAN_LITTLE
  BITCLR(LE,32, plx, LAS0BRD, LAS0BRD_ENDIAN);
#endif

  /* Enable active high interrupt1 through the PLX to the PCI bus.
   */
  LE_WRITE16(plx, INTCSR, INTCSR_INT1_Enable|
                            INTCSR_INT1_Polarity|
                            INTCSR_PCI_Enable);

  // Install ISR

  NAT_WRITE32(evr, IRQEnable, 0); // Disable interrupts

  // Acknowledge missed interrupts
  //TODO: This avoids a spurious FIFO Full
  NAT_WRITE32(evr, IRQFlag, NAT_READ32(evr, IRQFlag));

  EVRMRM *receiver=new EVRMRM(id,evr);

  if(devPCIConnectInterrupt(cur,&EVRMRM::isr,receiver)){
      printf("Failed to install ISR\n");
  }else{
      NAT_WRITE32(evr, IRQEnable,
          IRQ_Enable|IRQ_Heartbeat|IRQ_RXErr|IRQ_HWMapped|IRQ_Event
      );

      storeEVR(id,receiver);
  }
}

static
void
printRamEvt(EVRMRM *evr,int evt,int ram)
{
  if(evt<=0 || evt>255)
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

/**
 * Automatic discovery and configuration of all
 * compatible EVRs in the system.
 *
 * Searchs first the PCI bus and then the VME bus
 * in a reproducable order.  Each device is assigned
 * an incrementing id number starting from 0.
 */
extern "C"
void
mrmEvrSetupPCI(int id,int b,int d,int f)
{
try {
  if(!!getEVR<EVR>(id))
    printf("ID %d already used run\n",id);

  setupPCI(id,b,d,f);

} catch(std::exception& e) {
  printf("Error: %s\n",e.what());
}
}

static const iocshArg mrmEvrSetupPCIArg0 = { "ID number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg1 = { "Bus number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg2 = { "Device number",iocshArgInt};
static const iocshArg mrmEvrSetupPCIArg3 = { "Function number",iocshArgInt};
static const iocshArg * const mrmEvrSetupPCIArgs[4] =
{&mrmEvrSetupPCIArg0,&mrmEvrSetupPCIArg1,&mrmEvrSetupPCIArg2,&mrmEvrSetupPCIArg3};
static const iocshFuncDef mrmEvrSetupPCIFuncDef =
    {"mrmEvrSetupPCI",4,mrmEvrSetupPCIArgs};
static void mrmEvrSetupPCICallFunc(const iocshArgBuf *args)
{
    mrmEvrSetupPCI(args[0].ival,args[1].ival,args[2].ival,args[3].ival);
}

extern "C"
void
mrmEvrDumpMap(int id,int evt,int ram)
{
  EVRMRM *card=getEVR<EVRMRM>(id);
  if(!card){
    printf("Invalid card\n");
    return;
  }
  printf("Print ram #%d\n",ram);
  if(evt>0){
    // Print a single event
    printRamEvt(card,evt,ram);
    return;
  }
  for(evt=1;evt<=255;evt++){
    printRamEvt(card,evt,ram);
  }
}

static const iocshArg mrmEvrDumpMapArg0 = { "ID number",iocshArgInt};
static const iocshArg mrmEvrDumpMapArg1 = { "Event code",iocshArgInt};
static const iocshArg mrmEvrDumpMapArg2 = { "Mapping select 0 or 1",iocshArgInt};
static const iocshArg * const mrmEvrDumpMapArgs[3] =
{&mrmEvrDumpMapArg0,&mrmEvrDumpMapArg1,&mrmEvrDumpMapArg2};
static const iocshFuncDef mrmEvrDumpMapFuncDef =
    {"mrmEvrDumpMap",3,mrmEvrDumpMapArgs};
static void mrmEvrDumpMapCallFunc(const iocshArgBuf *args)
{
    mrmEvrDumpMap(args[0].ival,args[1].ival,args[2].ival);
}

extern "C"
void mrmsetupreg()
{
  iocshRegister(&mrmEvrSetupPCIFuncDef,mrmEvrSetupPCICallFunc);
  iocshRegister(&mrmEvrDumpMapFuncDef,mrmEvrDumpMapCallFunc);
}

epicsExportRegistrar(mrmsetupreg);
