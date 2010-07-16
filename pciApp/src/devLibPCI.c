
#include <stdlib.h>

#include <ellLib.h>
#include <epicsThread.h>

#include "devLibPCIImpl.h"

#define epicsExportSharedSymbols
#include "devLibPCI.h"

static epicsThreadOnceId devPCIInit_once = EPICS_THREAD_ONCE_INIT;
static int devPCIInit_result = 42;

static
void devInit(void* junk)
{
  if(!pdevLibPCI) {
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
  void  *parameter
)
{
  PCIINIT;

  return (*pdevLibPCI->pDevPCIConnectInterrupt)
                (curdev,pFunction,parameter);
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
