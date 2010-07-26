
#include <stdlib.h>
#include <epicsAssert.h>

#include <vxWorks.h>
#include <types.h>
#include <sysLib.h>
#include <intLib.h>
#include <iv.h>
#include <drv/pci/pciIntLib.h>
#include <vxLib.h>

#include <dbDefs.h>

#include "devLibPCIImpl.h"
#include "osdPciShared.h"

#if defined(VXPCIINTOFFSET)
// do nothing

#elif defined(INT_NUM_IRQ0)
#define VXPCIINTOFFSET INT_NUM_IRQ0

#elif defined(INT_VEC_IRQ0)
#define VXPCIINTOFFSET INT_VEC_IRQ0

#else
#define VXPCIINTOFFSET 0

#endif

static
int vxworksDevPCIConnectInterrupt(
  epicsPCIDevice *dev,
  void (*pFunction)(void *),
  void  *parameter,
  unsigned int opt
)
{
  int status;
  struct osdPCIDevice *osd=pcidev2osd(dev);

  status=pciIntConnect((void*)INUM_TO_IVEC(VXPCIINTOFFSET + osd->dev.irq),
                       pFunction, (int)parameter);

  if(status<0)
    return S_dev_vecInstlFail;

  return 1;
}

static
int vxworksDevPCIDisconnectInterrupt(
  epicsPCIDevice *dev,
  void (*pFunction)(void *),
  void  *parameter
)
{
  int status;
  struct osdPCIDevice *osd=pcidev2osd(dev);

#ifdef VXWORKS_PCI_OLD

  status=pciIntDisconnect((void*)INUM_TO_IVEC(VXPCIINTOFFSET + osd->dev.irq),
                       pFunction);

#else

  status=pciIntDisconnect2((void*)INUM_TO_IVEC(VXPCIINTOFFSET + osd->dev.irq),
                       pFunction, (int)parameter);

#endif

  if(status<0)
    return S_dev_intDisconnect;

  return 1;
}


devLibPCI pvxworksPCI = {
  "native",
  NULL, NULL,
  sharedDevPCIFindCB,
  sharedDevPCIToLocalAddr,
  sharedDevPCIBarLen,
  vxworksDevPCIConnectInterrupt,
  vxworksDevPCIDisconnectInterrupt
};
#include <epicsExport.h>

void devLibPCIRegisterBaseDefault(void)
{
    devLibPCIRegisterDriver(&pvxworksPCI);
}
epicsExportRegistrar(devLibPCIRegisterBaseDefault);
