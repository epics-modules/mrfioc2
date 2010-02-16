
#include <stdlib.h>
#include <epicsAssert.h>

#include <rtems/pci.h>
#include <rtems/endian.h>
#include <rtems/irq.h>

#include <dbDefs.h>

#include "devLibPCIImpl.h"
#include "osdPciShared.h"


static
int rtemsDevPCIConnectInterrupt(
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

#if BSP_SHARED_HANDLER_SUPPORT > 0
  isr.next_handler=NULL;

  if (!BSP_install_rtems_shared_irq_handler(&isr))
    return 1;
#else
  if (!BSP_install_rtems_irq_handler(&isr))
    return 1;
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

#if BSP_SHARED_HANDLER_SUPPORT > 0
  isr.next_handler=NULL;
#endif

  if(!BSP_remove_rtems_irq_handler(&isr))
    return 1;

  return 0;
}


devLibPCIVirtualOS prtemsPCIVirtualOS = {
  sharedDevPCIFindCB,
  sharedDevPCIToLocalAddr,
  sharedDevPCIBarLen,
  rtemsDevPCIConnectInterrupt,
  rtemsDevPCIDisconnectInterrupt
};

devLibPCIVirtualOS *pdevLibPCIVirtualOS = &prtemsPCIVirtualOS;
