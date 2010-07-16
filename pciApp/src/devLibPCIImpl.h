
#ifndef DEVLIBPCIIMPL_H_INC
#define DEVLIBPCIIMPL_H_INC

#include <stddef.h>

#include <dbDefs.h>
#include <ellLib.h>
#include <shareLib.h>
#include <epicsTypes.h>

#include "devLibPCI.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {

  int (*pDevInit)(void);

  int (*pDevFinal)(void);

  int (*pDevPCIFind)(const epicsPCIID *ids, devPCISearchFn searchfn, void *arg, unsigned int o);

  int (*pDevPCIToLocalAddr)(epicsPCIDevice* dev,unsigned int bar,volatile void **a,unsigned int o);

  epicsUInt32 (*pDevPCIBarLen)(epicsPCIDevice* dev,unsigned int bar);

  int (*pDevPCIConnectInterrupt)(epicsPCIDevice *id,
                                 void (*pFunction)(void *),
                                 void  *parameter);

  int (*pDevPCIDisconnectInterrupt)(epicsPCIDevice *id,
                                    void (*pFunction)(void *),
                                    void  *parameter);

} devLibPCI;

epicsShareExtern devLibPCI *pdevLibPCI;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DEVLIBPCIIMPL_H_INC */
