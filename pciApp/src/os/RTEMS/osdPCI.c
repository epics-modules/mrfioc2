
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
  int N, ret=0;

  for(N=0; ; N++){

    osdPCIDevice *next=malloc(sizeof(osdPCIDevice));
    if(!next)
      return 1;

    int b, d, f;
    int err=pci_find_device(vend,dev,N, &b, &d, &f);
    if(err){ /* No more */
      free(next);
      break;
    }
    next->dev.bus=b;
    next->dev.device=d;
    next->dev.function=f;

    /* Read config space */
    uint8_t val8;
    uint16_t val16;
    uint32_t val32;

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

    for(b=0;b<PCIBARCOUNT;b++){
      pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS(b), &val32);

      next->dev.bar[b].ioport=val32 & PCI_BASE_ADDRESS_SPACE_IO;
      if(next->dev.bar[b].ioport){
        /* This BAR is I/O ports */
        next->dev.bar[b].below1M=0;
        next->dev.bar[b].addr64=0;

        next->dev.bar[b].base=(volatile void*)( val32 & PCI_BASE_ADDRESS_IO_MASK );

        next->dev.bar[b].len=0; /* TODO: detect region length */

      }else{
        /* This BAR is memory mapped */
        next->dev.bar[b].below1M=val32&PCI_BASE_ADDRESS_MEM_TYPE_1M;
        next->dev.bar[b].addr64=val32&PCI_BASE_ADDRESS_MEM_TYPE_64;

        next->dev.bar[b].base=(volatile void*)( val32 & PCI_BASE_ADDRESS_MEM_MASK );

        next->dev.bar[b].len=0; /* TODO: detect region length */
      }
    }

    pci_read_config_dword(b,d,f,PCI_ROM_ADDRESS, &val32);
    next->dev.erom=(volatile void*)(val32 & PCI_ROM_ADDRESS_MASK);

    pci_read_config_byte(b,d,f,PCI_INTERRUPT_LINE, &val8);
    next->dev.irq=val32;

    ellInsert(store,NULL,&next->node);
  }

  return ret;
}

static
int rtemsDevPCIToLocalAddr(struct osdPCIDevice* dev,unsigned int bar,volatile void **a)
{
  return 1;
}


devLibPCIVirtualOS prtemsPCIVirtualOS = {
  rtemsDevPCIFind,
  rtemsDevPCIToLocalAddr
};

devLibPCIVirtualOS *pdevLibPCIVirtualOS = &prtemsPCIVirtualOS;
