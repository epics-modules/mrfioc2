
#include <stdlib.h>

#include <ellLib.h>

#include "devLibPCIImpl.h"

#define epicsExportSharedSymbols
#include "devLibPCI.h"

/* List of osdPCIDevice */
static ELLLIST devices;

typedef struct {
  ELLNODE node;
  epicsUInt16 device,vendor;
} dev_vend_entry;

/* List of dev_vend_entry */
static ELLLIST dev_vend_cache;

static
int fill_cache(epicsUInt16 dev,epicsUInt16 vend);

/**************** API functions *****************/

/*
 * Machinery for searching for PCI devices.
 *
 * This is a general function to support all possible
 * search filtering conditions.
 */
epicsShareFunc
int
devPCIFindCB(
     const epicsPCIID *idlist,
     int (*searchfn)(void*,const epicsPCIDevice*),
     void *arg
)
{
  int err=0;
  ELLNODE *cur;
  const osdPCIDevice *curdev=NULL;
  const epicsPCIID *search;

  if(!pdevLibPCIVirtualOS)
    return 5;

  if(!searchfn)
    return 2;

  /*
   * Ensure all entries for the requested device/vendor pairs
   * are in the 'devices' list.
   */
  for(search=idlist; search && !!search->device; search++){
    if( (err=fill_cache(search->device, search->vendor)) )
      return err;
  }

  /*
   * The search proceeds once through the list of devices.
   *
   * Each device is compared to the list of ids.
   * If it matches then control goes to the end of the outer
   *  loop to determine if this is the requested instance.
   * If not then the next device is searched.
   *
   * After the loops, 'curdev' can be non-NULL only if
   * control hit one of the break statements.
   */

  cur=ellFirst(&devices);
  for(; cur; cur=ellNext(cur)){
    curdev=CONTAINER(cur,const osdPCIDevice,node);

    for(search=idlist; search && !!search->device; search++){

      if(search->device!=curdev->dev.id.device)
        break;
      else
      if(search->vendor!=curdev->dev.id.vendor)
        break;
      else
      if( search->sub_device!=DEVPCI_ANY_SUBDEVICE &&
          search->sub_device!=curdev->dev.id.sub_device
        )
        break;
      else
      if( search->sub_vendor!=DEVPCI_ANY_SUBVENDOR &&
          search->sub_vendor!=curdev->dev.id.sub_vendor
        )
        break;
      else
      if( search->pci_class!=DEVPCI_ANY_CLASS &&
          search->pci_class!=curdev->dev.id.pci_class
        )
        break;
      else
      if( search->revision!=DEVPCI_ANY_REVISION &&
          search->revision!=curdev->dev.id.revision
        )
        break;

      /* Match found */

      err=searchfn(arg,&curdev->dev);
      switch(err){
      case 0: /* Continue search */
        break;
      case 1: /* Abort search OK */
        return 0;
      default:/* Abort search Err */
        return err;
      }

    }

  }

  return 0;
}

struct bdfmatch
{
  unsigned int b,d,f;
  const epicsPCIDevice* found;
};

static
int bdfsearch(void* ptr,const epicsPCIDevice*cur)
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
const epicsPCIDevice **found
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

  err=devPCIFindCB(idlist,&bdfsearch,&find);
  if(err!=0){
    /* Search failed? */
    return err;
  }

  if(!find.found){
    /* Not found */
    return 1;
  }

  *found=find.found;
  return 0;
}

int
devPCIToLocalAddr(
  const epicsPCIDevice *idlist,
  unsigned int bar,
  volatile void **ppLocalAddr
)
{
  osdPCIDevice *curdev=CONTAINER(idlist,osdPCIDevice,dev);

  if(!pdevLibPCIVirtualOS)
    return 5;

  if(bar>=PCIBARCOUNT)
    return 2;

  return (*pdevLibPCIVirtualOS->pDevPCIToLocalAddr)(curdev,bar,ppLocalAddr);
}



epicsShareFunc
epicsUInt32
devPCIBarLen(
  const epicsPCIDevice *idlist,
          unsigned int  bar
)
{
  osdPCIDevice *curdev=CONTAINER(idlist,osdPCIDevice,dev);

  if(!pdevLibPCIVirtualOS)
    return 5;

  if(bar>=PCIBARCOUNT)
    return 2;

  return (*pdevLibPCIVirtualOS->pDevPCIBarLen)(curdev,bar);
}

epicsShareFunc
int devPCIConnectInterrupt(
  const epicsPCIDevice *id,
  void (*pFunction)(void *),
  void  *parameter
)
{
  osdPCIDevice *curdev=CONTAINER(id,osdPCIDevice,dev);

  if(!pdevLibPCIVirtualOS)
    return 5;

  return (*pdevLibPCIVirtualOS->pDevPCIConnectInterrupt)
                (curdev,pFunction,parameter);
}

epicsShareFunc
int devPCIDisconnectInterrupt(
  const epicsPCIDevice *id,
  void (*pFunction)(void *)
)
{
  osdPCIDevice *curdev=CONTAINER(id,osdPCIDevice,dev);

  if(!pdevLibPCIVirtualOS)
    return 5;

  return (*pdevLibPCIVirtualOS->pDevPCIDisconnectInterrupt)
                (curdev,pFunction);
}

/**************** local functions *****************/



static
int fill_cache(epicsUInt16 dev,epicsUInt16 vend)
{
  ELLNODE *cur;
  const dev_vend_entry *curent;
  dev_vend_entry *next;

  for(cur=ellFirst(&dev_vend_cache); cur; cur=ellNext(cur)){
    curent=CONTAINER(cur,const dev_vend_entry,node);

    /* If one device is found then all must be in cache */
    if( curent->device==dev && curent->vendor==vend )
      return 0;
  }

  next=malloc(sizeof(dev_vend_entry));
  if(!next)
    return 11;
  next->device=dev;
  next->vendor=vend;

  if( (pdevLibPCIVirtualOS->pDevPCIFind)(dev,vend,&devices) ){
    free(next);
    return 12;
  }

  /* Prepend */
  ellInsert(&dev_vend_cache, NULL, &next->node);

  return 0;
}
