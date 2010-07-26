
#include <stdlib.h>
#include <epicsAssert.h>

#include <rtems/pci.h>
#include <rtems/endian.h>
#include <bsp/irq.h>

#include <dbDefs.h>

#include "devLibPCIImpl.h"
#include "osdPciShared.h"


static
int rtemsDevPCIConnectInterrupt(
  epicsPCIDevice *dev,
  void (*pFunction)(void *),
  void  *parameter,
  unsigned int opt
)
{
  struct osdPCIDevice *id=pcidev2osd(dev);

  rtems_irq_connect_data isr;

  isr.name = id->dev.irq;
  isr.hdl = pFunction;
  isr.handle = parameter;

  isr.on = NULL;
  isr.off= NULL;
  isr.isOn=NULL;

#ifdef BSP_SHARED_HANDLER_SUPPORT
  isr.next_handler=NULL;

  if (!BSP_install_rtems_shared_irq_handler(&isr))
    return S_dev_vecInstlFail;
#else
  if (!BSP_install_rtems_irq_handler(&isr))
    return S_dev_vecInstlFail;
#endif

  return 0;
}

static
int rtemsDevPCIDisconnectInterrupt(
  epicsPCIDevice *dev,
  void (*pFunction)(void *),
  void  *parameter
)
{
  struct osdPCIDevice *id=pcidev2osd(dev);

  rtems_irq_connect_data isr;

  isr.name = id->dev.irq;
  isr.hdl = pFunction;
  isr.handle = parameter;

  isr.on = NULL;
  isr.off= NULL;
  isr.isOn=NULL;

#ifdef BSP_SHARED_HANDLER_SUPPORT
  isr.next_handler=NULL;
#endif

  if(!BSP_remove_rtems_irq_handler(&isr))
    return S_dev_intDisconnect;

  return 0;
}


devLibPCI prtemsPCIVirtualOS = {
  NULL, NULL,
  sharedDevPCIFindCB,
  sharedDevPCIToLocalAddr,
  sharedDevPCIBarLen,
  rtemsDevPCIConnectInterrupt,
  rtemsDevPCIDisconnectInterrupt
};

devLibPCI *pdevLibPCI = &prtemsPCIVirtualOS;
