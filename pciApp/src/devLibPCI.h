
#ifndef DEVLIBPCI_H_INC
#define DEVLIBPCI_H_INC 1

#include <dbDefs.h>
#include <epicsTypes.h>
#include <devLib.h>
#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PCI device identifier
 */

typedef struct {
  epicsUInt16 device, vendor;
  epicsUInt32 sub_device, sub_vendor;
  epicsUInt32 pci_class;
  epicsUInt16 revision;
} epicsPCIID;

/* sub*, class, and revision are oversized to allow a
 * distinct wildcard match value.
 */
#define DEVPCI_ANY_SUBDEVICE 0x10000
#define DEVPCI_ANY_SUBVENDOR 0x10000
#define DEVPCI_ANY_CLASS 0x1000000
#define DEVPCI_ANY_REVISION 0x100

/* Use the following macros when defining ID search lists.
 *
 * ie.
 * static const epicsPCIID mydevs[] = {
 *    DEVPCI_SUBDEVICE_SUBVENDOR( 0x1234, 0x1030, 0x0001, 0x4321 ),
 *    DEVPCI_SUBDEVICE_SUBVENDOR( 0x1234, 0x1030, 0x0002, 0x4321 ),
 *    NULL
 * };
 */

#define DEVPCI_DEVICE_VENDOR(dev,vend) \
{ dev, vend, DEVPCI_ANY_SUBDEVICE, DEVPCI_ANY_SUBVENDOR, \
DEVPCI_ANY_CLASS, DEVPCI_ANY_REVISION }

#define DEVPCI_DEVICE_VENDOR_CLASS(dev,vend,class) \
{ dev, vend, DEVPCI_ANY_SUBDEVICE, DEVPCI_ANY_SUBVENDOR, \
class, DEVPCI_ANY_REVISION }

#define DEVPCI_SUBDEVICE_SUBVENDOR(dev,vend,sdev,svend) \
{ dev, vend, sdev, svend, \
DEVPCI_ANY_CLASS, DEVPCI_ANY_REVISION }

#define DEVPCI_SUBDEVICE_SUBVENDOR_CLASS(dev,vend,sdev,svend,revision,class) \
{ dev, vend, sdev, svend, \
class, revision }

struct PCIBar {
  volatile void *base;
  epicsUInt32 len;
  unsigned int ioport:1; /* 0 memory, 1 I/O */
  unsigned int addr64:1; /* 0 32 bit, 1 64 bit */
  unsigned int below1M:1; /* 0 Normal, 1 Must be mapped below 1M */
};

typedef struct {
  epicsPCIID   id;
  unsigned int bus;
  unsigned int device;
  unsigned int function;
  struct PCIBar bar[6];
  volatile void *erom;
  epicsUInt8 irq;
} epicsPCIDevice;

#define PCIBARCOUNT NELEMENTS( ((epicsPCIDevice*)0)->bar )

/*
 * Expects a NULL terminated list of identifiers
 */
epicsShareFunc
int devPCIFind(
     const epicsPCIID *idlist,
         unsigned int  instance,
const epicsPCIDevice **found
);

epicsShareFunc
int
devPCIToLocalAddr(
  const epicsPCIDevice *idlist,
          unsigned int  bar,
        volatile void **ppLocalAddr
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DEVLIBPCI_H_INC */
