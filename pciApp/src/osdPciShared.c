
#include <stdlib.h>

#include <ellLib.h>

#include "devLibPCIImpl.h"

#include "devLibPCI.h"

#define epicsExportSharedSymbols
#include "osdPciShared.h"

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

/*
 * Machinery for searching for PCI devices.
 *
 * This is a general function to support all possible
 * search filtering conditions.
 */
int
sharedDevPCIFindCB(
     const epicsPCIID *idlist,
     devPCISearchFn searchfn,
     void *arg,
     unsigned int opt /* always 0 */
)
{
  int err=0;
  ELLNODE *cur;
  osdPCIDevice *curdev=NULL;
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
    if(search->device==DEVPCI_ANY_DEVICE ||
       search->vendor==DEVPCI_ANY_VENDOR)
    {
      epicsPrintf("devPCI: Wildcards are not supported in Device and Vendor fields\n");
      return 3;
    }
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
   * control did not hit one of the break statements.
   */

  cur=ellFirst(&devices);
  for(; cur; cur=ellNext(cur)){
    curdev=CONTAINER(cur,osdPCIDevice,node);

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

int
sharedDevPCIToLocalAddr(
  epicsPCIDevice* dev,
  unsigned int bar,
  volatile void **ppLocalAddr,
  unsigned int opt
)
{
  struct osdPCIDevice *osd=pcidev2osd(dev);

  if(!osd->base[bar])
    return 1;

  *ppLocalAddr=osd->base[bar];
  return 0;
}

epicsUInt32
sharedDevPCIBarLen(
  epicsPCIDevice* dev,
  unsigned int bar
)
{
  struct osdPCIDevice *osd=pcidev2osd(dev);
  int b=dev->bus, d=dev->device, f=dev->function;
  UINT32 start, max, mask;

  if(!osd->base[bar])
    return 1;

  if(osd->len[bar])
    return osd->len[bar];

  /* Note: the following assumes the bar is 32-bit */

  if(dev->bar[bar].ioport)
    mask=PCI_BASE_ADDRESS_IO_MASK;
  else
    mask=PCI_BASE_ADDRESS_MEM_MASK;

  /*
   * The idea here is to find the least significant bit which
   * is set by writing 1 to all the address bits.
   *
   * For example the mask for 32-bit IO Memory is 0xfffffff0
   * If a base address is currently set to 0x00043000
   * and when the mask is written the address becomes
   * 0xffffff80 then the length is 0x80 (128) bytes
   */
  pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS(bar), &start);

  /* If the BIOS didn't set this BAR then don't mess with it */
  if((start&mask)==0)
    return 0;

  pci_write_config_dword(b,d,f,PCI_BASE_ADDRESS(bar), mask);
  pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS(bar), &max);
  pci_write_config_dword(b,d,f,PCI_BASE_ADDRESS(bar), start);

  /* mask out bits which aren't address bits */
  max&=mask;

  /* Find lsb */
  osd->len[bar] = max & ~(max-1);

  return osd->len[bar];
}



/**************** local functions *****************/

static
int sharedDevPCIFind(epicsUInt16 dev,epicsUInt16 vend,ELLLIST* store)
{
  int N, ret=0, err;
  int b, d, f, region;

  /* Read config space */
  uint8_t val8;
  uint16_t val16;
  UINT32 val32;

  for(N=0; ; N++){

    osdPCIDevice *next=calloc(1,sizeof(osdPCIDevice));
    if(!next)
      return 1;

    err=pci_find_device(vend,dev,N, &b, &d, &f);
    if(err){ /* No more */
      free(next);
      break;
    }
    next->dev.bus=b;
    next->dev.device=d;
    next->dev.function=f;

    pci_read_config_word(b,d,f,PCI_DEVICE_ID, &val16);
    next->dev.id.device=val16;

    pci_read_config_word(b,d,f,PCI_VENDOR_ID, &val16);
    next->dev.id.vendor=val16;

    pci_read_config_word(b,d,f,PCI_SUBSYSTEM_ID, &val16);
    next->dev.id.sub_device=val16;

    pci_read_config_word(b,d,f,PCI_SUBSYSTEM_VENDOR_ID, &val16);
    next->dev.id.sub_vendor=val16;

    pci_read_config_dword(b,d,f,PCI_CLASS_REVISION, &val32);
    next->dev.id.revision=val32&0xff;
    next->dev.id.pci_class=val32>>8;

    for(region=0;region<PCIBARCOUNT;region++){
      pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS(region), &val32);

      next->dev.bar[region].ioport=(val32 & PCI_BASE_ADDRESS_SPACE)==PCI_BASE_ADDRESS_SPACE_IO;
      if(next->dev.bar[region].ioport){
        /* This BAR is I/O ports */
        next->dev.bar[region].below1M=0;
        next->dev.bar[region].addr64=0;

        next->base[region]=(volatile void*)( val32 & PCI_BASE_ADDRESS_IO_MASK );

        next->len[region]=0;

      }else{
        /* This BAR is memory mapped */
        next->dev.bar[region].below1M=!!(val32&PCI_BASE_ADDRESS_MEM_TYPE_1M);
        next->dev.bar[region].addr64=!!(val32&PCI_BASE_ADDRESS_MEM_TYPE_64);

        next->base[region]=(volatile void*)( val32 & PCI_BASE_ADDRESS_MEM_MASK );

        next->len[region]=0;
      }
    }

    pci_read_config_dword(b,d,f,PCI_ROM_ADDRESS, &val32);
    next->erom=(volatile void*)(val32 & PCI_ROM_ADDRESS_MASK);

    pci_read_config_byte(b,d,f,PCI_INTERRUPT_LINE, &val8);
    next->dev.irq=val8;

    ellInsert(store,NULL,&next->node);
  }

  return ret;
}

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

  if( sharedDevPCIFind(dev,vend,&devices) ){
    free(next);
    return 12;
  }

  /* Prepend */
  ellInsert(&dev_vend_cache, NULL, &next->node);

  return 0;
}
