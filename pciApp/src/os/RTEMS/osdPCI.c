
#include <stdlib.h>
#include <epicsAssert.h>

#include <rtems/pci.h>
#include <rtems/endian.h>

#include <ellLib.h>
#include <dbDefs.h>

#include "devLibPCI.h"

#define epicsExportSharedSymbols
#include "devLibPCIImpl.h"

/* 0 <= N <= 5 */
#define PCI_BASE_ADDRESS(N) ( PCI_BASE_ADDRESS_0 + 4*(N) )

static
int rtemsDevPCIFind(epicsUInt16 dev,epicsUInt16 vend,ELLLIST* store)
{
  int N, ret=0, err;
  int b, d, f, region;

  /* Read config space */
  uint8_t val8;
  uint16_t val16;
  uint32_t val32;

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
    next->dev.irq=val32;

    ellInsert(store,NULL,&next->node);
  }

  return ret;
}

static
int
rtemsDevPCIToLocalAddr(
  osdPCIDevice* dev,
  unsigned int bar,
  volatile void **ppLocalAddr
)
{
  *ppLocalAddr=dev->base[bar];
  return 0;
}

static
epicsUInt32
rtemsDevPCIBarLen(
  osdPCIDevice* dev,
  unsigned int bar
)
{
  int b=dev->dev.bus, d=dev->dev.device, f=dev->dev.function;
  uint32_t start, max, mask;

  if(dev->len[bar])
    return dev->len[bar];

  /* Note: the following assumes the bar is 32-bit */

  if(dev->dev.bar[bar].ioport)
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
  pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS(b), &start);

  /* If the BIOS didn't set this BAR then don't mess with it */
  if((start&mask)==0)
    return 0;

  pci_write_config_dword(b,d,f,PCI_BASE_ADDRESS(b), mask);
  pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS(b), &max);
  pci_write_config_dword(b,d,f,PCI_BASE_ADDRESS(b), start);

  /* mask out bits which aren't address bits */
  max&=mask;

  /* Find lsb */
  dev->len[bar] = max & ~(max-1);

  return dev->len[bar];
}


devLibPCIVirtualOS prtemsPCIVirtualOS = {
  rtemsDevPCIFind,
  rtemsDevPCIToLocalAddr,
  rtemsDevPCIBarLen
};

devLibPCIVirtualOS *pdevLibPCIVirtualOS = &prtemsPCIVirtualOS;
