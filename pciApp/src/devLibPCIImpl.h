
#ifndef DEVLIBPCIIMPL_H_INC
#define DEVLIBPCIIMPL_H_INC

#include <ellLib.h>
#include <shareLib.h>
#include <epicsTypes.h>

/*#include "osdPCI.h"
*/
struct osdPCIDevice;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

  /*
   * Find all by Device and Vender only.  Append to the list 'store'.
   */
  int (*pDevPCIFind)(epicsUInt16 dev,epicsUInt16 vend,ELLLIST* store);

  int (*pDevPCIToLocalAddr)(struct osdPCIDevice* dev,unsigned int bar,volatile void **a);

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
