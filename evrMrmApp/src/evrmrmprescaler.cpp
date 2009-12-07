
#include "evrmrmprescaler.h"

#include <mrfIoOps.h>
#include "evrRegMap.h"

epicsUInt32
MRMPreScaler::prescaler() const
{
    return nat_ioread32(base);
}

void
MRMPreScaler::setPrescaler(epicsUInt32 v)
{
    nat_iowrite32(base, v);
}
