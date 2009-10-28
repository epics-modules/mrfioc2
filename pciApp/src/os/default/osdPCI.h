
#ifndef OSDPCI_H_INC
#define OSDPCI_H_INC

#include <ellLib.h>
#include <dbDefs.h>

#include "devLibPCI.h"

struct osdPCIDevice {
  epicsPCIDevice dev; /* "public" data */
  ELLNODE node;
};
typedef struct osdPCIDevice osdPCIDevice;

#define osd2epicsDev(osd) CONTAINER(osd,osdPCIDevice,dev)

#endif /* OSDPCI_H_INC */
