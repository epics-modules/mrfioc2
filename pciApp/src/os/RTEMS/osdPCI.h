
#ifndef OSDPCI_H_INC
#define OSDPCI_H_INC

#include <rtems/pci.h>
#include <rtems/endian.h>

#include <ellLib.h>
#include <dbDefs.h>

#include "devLibPCI.h"

/* 0 <= N <= 5 */
#define PCI_BASE_ADDRESS(N) ( PCI_BASE_ADDRESS_0 + 4*(N) )

struct osdPCIDevice {
  epicsPCIDevice dev; /* "public" data */
  ELLNODE node;
};
typedef struct osdPCIDevice osdPCIDevice;

#define osd2epicsDev(osd) CONTAINER(osd,const osdPCIDevice,dev)

#endif /* OSDPCI_H_INC */
