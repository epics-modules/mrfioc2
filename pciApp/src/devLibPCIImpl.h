
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
  const char *name;

  int (*pDevInit)(void);

  int (*pDevFinal)(void);

  int (*pDevPCIFind)(const epicsPCIID *ids, devPCISearchFn searchfn, void *arg, unsigned int o);

  int (*pDevPCIToLocalAddr)(epicsPCIDevice* dev,unsigned int bar,volatile void **a,unsigned int o);

  epicsUInt32 (*pDevPCIBarLen)(epicsPCIDevice* dev,unsigned int bar);

  int (*pDevPCIConnectInterrupt)(epicsPCIDevice *id,
                                 void (*pFunction)(void *),
                                 void  *parameter,
                                 unsigned int opt);

  int (*pDevPCIDisconnectInterrupt)(epicsPCIDevice *id,
                                    void (*pFunction)(void *),
                                    void  *parameter);

  ELLNODE node;
} devLibPCI;

epicsShareFunc
int
devLibPCIRegisterDriver(devLibPCI*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DEVLIBPCIIMPL_H_INC */
