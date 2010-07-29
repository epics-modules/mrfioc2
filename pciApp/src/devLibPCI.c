
#include <stdlib.h>
#include <string.h>

#include <ellLib.h>
#include <epicsThread.h>
#include <epicsMutex.h>
#include <iocsh.h>

#include "devLibPCIImpl.h"

#define epicsExportSharedSymbols
#include "devLibPCI.h"

#ifndef CONTAINER
# ifdef __GNUC__
#   define CONTAINER(ptr, structure, member) ({                     \
        const __typeof(((structure*)0)->member) *_ptr = (ptr);      \
        (structure*)((char*)_ptr - offsetof(structure, member));    \
    })
# else
#   define CONTAINER(ptr, structure, member) \
        ((structure*)((char*)(ptr) - offsetof(structure, member)))
# endif
#endif

static ELLLIST pciDrivers = {{NULL,NULL},0};

static devLibPCI *pdevLibPCI=NULL;

static epicsMutexId pciDriversLock = NULL;
static epicsThreadOnceId devPCIReg_once = EPICS_THREAD_ONCE_INIT;

static epicsThreadOnceId devPCIInit_once = EPICS_THREAD_ONCE_INIT;
static int devPCIInit_result = 42;

/******************* Initialization *********************/

static
void regInit(void* junk)
{
    pciDriversLock = epicsMutexMustCreate();
}

epicsShareFunc
int
devLibPCIRegisterDriver(devLibPCI* drv)
{
    int ret=0;
    ELLNODE *cur;
    devLibPCI *other;

    epicsThreadOnce(&devPCIReg_once, &regInit, NULL);

    epicsMutexMustLock(pciDriversLock);

    for(cur=ellFirst(&pciDrivers); cur; cur=ellNext(cur)) {
        other=CONTAINER(cur, devLibPCI, node);
        if (strcmp(drv->name, other->name)==0) {
            fprintf(stderr,"Failed to register PCI bus driver: name already taken\n");
            ret=1;
            break;
        }
    }
    if (!ret)
        ellAdd(&pciDrivers, &drv->node);

    epicsMutexUnlock(pciDriversLock);

    return ret;
}

epicsShareFunc
int
devLibPCIUse(const char* use)
{
    ELLNODE *cur;
    devLibPCI *drv;

    if (!use)
        use="native";

    epicsThreadOnce(&devPCIReg_once, &regInit, NULL);

    epicsMutexMustLock(pciDriversLock);

    if (pdevLibPCI) {
        epicsMutexUnlock(pciDriversLock);
        fprintf(stderr,"PCI bus driver already selected. Can't change selection\n");
        return 1;
    }

    for(cur=ellFirst(&pciDrivers); cur; cur=ellNext(cur)) {
        drv=CONTAINER(cur, devLibPCI, node);
        if (strcmp(drv->name, use)==0) {
            pdevLibPCI = drv;
            epicsMutexUnlock(pciDriversLock);
            return 0;
        }
    }
    epicsMutexUnlock(pciDriversLock);
    fprintf(stderr,"PCI bus driver '%s' not found\n",use);
    return 1;
}

epicsShareFunc
const char* devLibPCIDriverName()
{
    const char* ret=NULL;

    epicsThreadOnce(&devPCIReg_once, &regInit, NULL);

    epicsMutexMustLock(pciDriversLock);
    if (pdevLibPCI)
        ret = pdevLibPCI->name;
    epicsMutexUnlock(pciDriversLock);

    return ret;
}

static
void devInit(void* junk)
{
  if(devLibPCIUse(NULL)) {
    devPCIInit_result = S_dev_internal;
    return;
  }

  if(!!pdevLibPCI->pDevInit)
    devPCIInit_result = (*pdevLibPCI->pDevInit)();
  else
    devPCIInit_result = 0;
}

#define PCIINIT \
do { \
     epicsThreadOnce(&devPCIInit_once, &devInit, NULL); \
     if (devPCIInit_result) return devPCIInit_result; \
} while(0)


/**************** API functions *****************/

epicsShareFunc
int devPCIFindCB(
     const epicsPCIID *idlist,
     devPCISearchFn searchfn,
     void *arg,
     unsigned int opt /* always 0 */
)
{
  if(!idlist || !searchfn)
    return S_dev_badArgument;

  PCIINIT;

  return (*pdevLibPCI->pDevPCIFind)(idlist,searchfn,arg,opt);
}


struct bdfmatch
{
  unsigned int b,d,f;
  epicsPCIDevice* found;
};

static
int bdfsearch(void* ptr, epicsPCIDevice* cur)
{
  struct bdfmatch *mt=ptr;

  if( cur->bus==mt->b && cur->device==mt->d &&
      cur->function==mt->f )
  {
    mt->found=cur;
    return 1;
  }

  return 0;
}

/*
 * The most common PCI search using only id fields and BDF.
 */
epicsShareFunc
int devPCIFindBDF(
     const epicsPCIID *idlist,
     unsigned int      b,
     unsigned int      d,
     unsigned int      f,
      epicsPCIDevice **found,
     unsigned int      opt
)
{
  int err;
  struct bdfmatch find;

  if(!found)
    return 2;

  find.b=b;
  find.d=d;
  find.f=f;
  find.found=NULL;

  /* PCIINIT is called by devPCIFindCB()  */

  err=devPCIFindCB(idlist,&bdfsearch,&find, opt);
  if(err!=0){
    /* Search failed? */
    return err;
  }

  if(!find.found){
    /* Not found */
    return S_dev_noDevice;
  }

  *found=find.found;
  return 0;
}

int
devPCIToLocalAddr(
  epicsPCIDevice *curdev,
  unsigned int bar,
  volatile void **ppLocalAddr,
  unsigned int opt
)
{
  PCIINIT;

  if(bar>=PCIBARCOUNT)
    return S_dev_badArgument;

  return (*pdevLibPCI->pDevPCIToLocalAddr)(curdev,bar,ppLocalAddr,opt);
}



epicsShareFunc
epicsUInt32
devPCIBarLen(
  epicsPCIDevice *curdev,
          unsigned int  bar
)
{
  PCIINIT;

  if(bar>=PCIBARCOUNT)
    return S_dev_badArgument;

  return (*pdevLibPCI->pDevPCIBarLen)(curdev,bar);
}

epicsShareFunc
int devPCIConnectInterrupt(
  epicsPCIDevice *curdev,
  void (*pFunction)(void *),
  void  *parameter,
  unsigned int opt
)
{
  PCIINIT;

  return (*pdevLibPCI->pDevPCIConnectInterrupt)
                (curdev,pFunction,parameter,opt);
}

epicsShareFunc
int devPCIDisconnectInterrupt(
  epicsPCIDevice *curdev,
  void (*pFunction)(void *),
  void  *parameter
)
{
  PCIINIT;

  return (*pdevLibPCI->pDevPCIDisconnectInterrupt)
                (curdev,pFunction,parameter);
}

static
int
searchandprint(void* plvl,epicsPCIDevice* dev)
{
    int *lvl=plvl;
    devPCIShowDevice(*lvl,dev);
    return 0;
}

void
devPCIShow(int lvl, int vendor, int device, int exact)
{
    epicsPCIID ids[] = {
        DEVPCI_DEVICE_VENDOR(device,vendor),
        DEVPCI_END
    };

    if (vendor==0 && !exact) ids[0].vendor=DEVPCI_ANY_VENDOR;
    if (device==0 && !exact) ids[0].device=DEVPCI_ANY_DEVICE;

    devPCIFindCB(ids,&searchandprint, &lvl, 0);
}

void
devPCIShowDevice(int lvl, epicsPCIDevice *dev)
{
    int i;
    printf("PCI %u:%u.%u IRQ %u\n"
           "  vendor:device %04x:%04x\n",
           dev->bus, dev->device, dev->function, dev->irq,
           dev->id.vendor, dev->id.device);
    if(lvl>=1)
        printf("  subved:subdev %04x:%04x\n"
               "  class %06x rev %02x\n",
               dev->id.sub_vendor, dev->id.sub_device,
               dev->id.pci_class, dev->id.revision
               );
    if(lvl<2)
        return;
    for(i=0; i<PCIBARCOUNT; i++)
    {
        printf("BAR %u %s-bit %s%s\n",i,
               dev->bar[i].addr64?"64":"32",
               dev->bar[i].ioport?"IO Port":"MMIO",
               dev->bar[i].below1M?" Below 1M":"");
    }
}

static const iocshArg devPCIShowArg0 = { "verbosity level",iocshArgInt};
static const iocshArg devPCIShowArg1 = { "PCI Vendor ID (0=any)",iocshArgInt};
static const iocshArg devPCIShowArg2 = { "PCI Device ID (0=any)",iocshArgInt};
static const iocshArg devPCIShowArg3 = { "exact (1=treat 0 as 0)",iocshArgInt};
static const iocshArg * const devPCIShowArgs[4] =
{&devPCIShowArg0,&devPCIShowArg1,&devPCIShowArg2,&devPCIShowArg3};
static const iocshFuncDef devPCIShowFuncDef =
    {"devPCIShow",4,devPCIShowArgs};
static void devPCIShowCallFunc(const iocshArgBuf *args)
{
    devPCIShow(args[0].ival,args[1].ival,args[2].ival,args[3].ival);
}

static const iocshArg devLibPCIUseArg0 = { "verbosity level",iocshArgString};
static const iocshArg * const devLibPCIUseArgs[1] =
{&devLibPCIUseArg0};
static const iocshFuncDef devLibPCIUseFuncDef =
    {"devLibPCIUse",1,devLibPCIUseArgs};
static void devLibPCIUseCallFunc(const iocshArgBuf *args)
{
    devLibPCIUse(args[0].sval);
}

#include <epicsExport.h>

static
void devLibPCIIOCSH()
{
  iocshRegister(&devPCIShowFuncDef,devPCIShowCallFunc);
  iocshRegister(&devLibPCIUseFuncDef,devLibPCIUseCallFunc);
}

epicsExportRegistrar(devLibPCIIOCSH);
