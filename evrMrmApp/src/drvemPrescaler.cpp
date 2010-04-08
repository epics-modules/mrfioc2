
#include "drvemPrescaler.h"

#include <mrfIoOps.h>
#include "evrRegMap.h"

#include <stdexcept>

epicsUInt32
MRMPreScaler::prescaler() const
{
    return nat_ioread32(base);
}

void
MRMPreScaler::setPrescaler(epicsUInt32 v)
{
    if(v>0xffff)
        throw std::out_of_range("prescaler setting is out of range");

    nat_iowrite32(base, v);
}
