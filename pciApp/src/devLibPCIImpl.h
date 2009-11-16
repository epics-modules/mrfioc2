
#ifndef DEVLIBPCIIMPL_H_INC
#define DEVLIBPCIIMPL_H_INC

#include <stddef.h>

#include <dbDefs.h>
#include <ellLib.h>
#include <shareLib.h>
#include <epicsTypes.h>

#include "devLibPCI.h"

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

#ifdef __cplusplus
extern "C" {
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

#define osd2epicsDev(osd) CONTAINER(osd,const osdPCIDevice,dev)

typedef struct {

  /*
   * Find all by Device and Vender only.  Append to the list 'store'.
   */
  int (*pDevPCIFind)(epicsUInt16 dev,epicsUInt16 vend,ELLLIST* store);

  int (*pDevPCIToLocalAddr)(struct osdPCIDevice* dev,unsigned int bar,volatile void **a);

  epicsUInt32 (*pDevPCIBarLen)(struct osdPCIDevice* dev,unsigned int bar);

} devLibPCIVirtualOS;

epicsShareExtern devLibPCIVirtualOS *pdevLibPCIVirtualOS;

/* Functions for OS support implementors */

epicsShareFunc
int
devPCIToLocalAddr_General(
  struct osdPCIDevice* dev,
  unsigned int bar,
  volatile void **ppLocalAddr
);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DEVLIBPCIIMPL_H_INC */
