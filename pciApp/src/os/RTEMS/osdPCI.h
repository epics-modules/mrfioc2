
#ifndef OSDPCI_H_INC
#define OSDPCI_H_INC

#include <rtems/pci.h>
#include <rtems/endian.h>

#include <ellLib.h>
#include <dbDefs.h>

#include "devLibPCI.h"


/* Host order <-> Little endian (PCI)
 */

#if CPU_BIG_ENDIAN
#  define htops(x) ( ((x<<8)&0xff00) | ((x>>8)&00ff) )

#  define htopl(x) ( ((x<<24)&0xff000000) | ((x<<8)&0x00ff0000) | \
                     ((x>>8)&0x0000ff00) | ((x>>24)&0x000000ff) )

#elif CPU_LITTLE_ENDIAN
#  define htops(x) ( x )
#  define htopl(x) ( x )
#else
#  error "RTEMS endianness macros undefined"
#endif

/* 0 <= N <= 5 */
#define PCI_BASE_ADDRESS(N) ( PCI_BASE_ADDRESS_0 + 4*(N) )

struct osdPCIDevice {
  epicsPCIDevice dev; /* "public" data */
  ELLNODE node;
};
typedef struct osdPCIDevice osdPCIDevice;

#define osd2epicsDev(osd) CONTAINER(osd,osdPCIDevice,dev)

#endif /* OSDPCI_H_INC */
