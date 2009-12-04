
#include "evrmrmiocsh.h"

#include <cstdio>

#include <stdexcept>
#include <map>

#include <iocsh.h>
#include <epicsExport.h>

#include <devLibPCI.h>
#include <epicsInterrupt.h>
#include "mrmpci.h"

#include "evrmrm.h"
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
int
setupAutoPCI(int inst)
{

  if(DBG)
    printf("Searching for PCI EVRs\n");

  // 10 is an arbitrary number to prevent an infinite loop
  for(; inst<10; inst++)
  {
    const epicsPCIDevice *cur=0;
    if( devPCIFind(mrmevrs,inst,&cur) ){
      if(DBG) printf("Done\n");
      break;
    }

    printf("Device %u  %u:%u.%u\n",inst,cur->bus,cur->device,cur->function);

    volatile epicsUInt8 *plx, *evr;

    if( devPCIToLocalAddr(cur,0,(volatile void**)&plx) ||
        devPCIToLocalAddr(cur,2,(volatile void**)&evr))
    {
        printf("Failed to map BARs 0 and 2\n");
        continue;
    }
    if(!plx || !evr){
        printf("BARs mapped to zero? (%08lx,%08lx)\n",
                (unsigned long)plx,(unsigned long)evr);
        continue;
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

    // Install ISR

    EVRMRM *receiver=new EVRMRM(inst,evr);

    int ilock=epicsInterruptLock();

    if(devPCIConnectInterrupt(cur,EVRMRM::isr,receiver)){
        epicsInterruptUnlock(ilock);
        printf("Failed to install ISR\n");
    }else{
        // Acknowledge missed interrupts
        NAT_WRITE32(evr, IRQFlag, NAT_READ32(evr, IRQFlag));

        NAT_WRITE32(evr, IRQEnable,
            IRQ_Enable|IRQ_Heartbeat|IRQ_RXErr
        );

        epicsInterruptUnlock(ilock);

        storeEVR(inst,receiver);
    }
  }

  return inst;
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
mrmevrSetupAuto(void)
{
try {
  if(!!getEVR<EVR>(0))
    printf("mrmevrSetupAuto was already run\n");

  unsigned int inst=0;

  inst=setupAutoPCI(inst);

  //inst=setupAutoVME64(inst);

  if(DBG)
    printf("%u EVRs\n",inst);

} catch(std::exception& e) {
  printf("Error: %s\n",e.what());
}
}

static const iocshArg * const mrmevrSetupAutoArgs[0] =
{};
static const iocshFuncDef mrmevrSetupAutoFuncDef =
    {"mrmevrSetupAuto",0,mrmevrSetupAutoArgs};
static void mrmevrSetupAutoCallFunc(const iocshArgBuf *args)
{
    mrmevrSetupAuto();
}

extern "C"
void mrmsetupreg()
{
  iocshRegister(&mrmevrSetupAutoFuncDef,mrmevrSetupAutoCallFunc);
}

epicsExportRegistrar(mrmsetupreg);
