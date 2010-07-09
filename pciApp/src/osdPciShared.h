
#ifndef OSDPCISHARED_H_INC
#define OSDPCISHARED_H_INC

#include "devLibPCIOSD.h"

/* Subtract member byte offset, returning pointer to parent object
 *
 * Added in Base 3.14.11
 */
#ifndef CONTAINER
# ifdef __GNUC__
#   define CONTAINER(ptr, structure, member) ({                     \
        const __typeof(((structure*)0)->member) *_ptr = (ptr);      \
        (structure*)((char*)_ptr - offsetof(structure, member));    \
    })
# else
#   define CONTAINER(ptr, structure, member) \
        ((structure*)((char*)(ptr) - offsetof(structure, member)))
# endif
#endif

struct osdPCIDevice {
  epicsPCIDevice dev; /* "public" data */

  /* Can be used to cache values */
  volatile void *base[PCIBARCOUNT];
  epicsUInt32    len[PCIBARCOUNT];
  volatile void *erom;

  ELLNODE node;

  void *drvpvt; /* for out of tree drivers */
};
typedef struct osdPCIDevice osdPCIDevice;

#define pcidev2osd(devptr) CONTAINER(devptr,osdPCIDevice,dev)

int
sharedDevPCIFindCB(
     const epicsPCIID *idlist,
     devPCISearchFn searchfn,
     void *arg,
     unsigned int opt /* always 0 */
);

int
sharedDevPCIToLocalAddr(
  epicsPCIDevice* dev,
  unsigned int bar,
  volatile void **ppLocalAddr,
  unsigned int opt
);

epicsUInt32
sharedDevPCIBarLen(
  epicsPCIDevice* dev,
  unsigned int bar
);

#endif /* OSDPCISHARED_H_INC */
