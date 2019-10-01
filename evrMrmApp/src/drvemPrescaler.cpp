/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */


#include "mrf/databuf.h"
#include <mrfCommonIO.h>
#include "evrRegMap.h"

#include <stdexcept>
#include <evr/prescaler.h>

#include <epicsExport.h>
#include "drvemPrescaler.h"


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

epicsUInt32
MRMPreScaler::prescalerPhasOffs() const
{
    return nat_ioread32(base + ScalerPhasOffs_offset);
}

void
MRMPreScaler::setPrescalerPhasOffs(epicsUInt32 v)
{
    nat_iowrite32(base + ScalerPhasOffs_offset, v);
}

epicsUInt32
MRMPreScaler::prescalerPulsTrig() const
{
    return nat_ioread32(base + ScalerPulsTrig_offset);
}

void
MRMPreScaler::setPrescalerPulsTrig(epicsUInt32 v)
{
    nat_iowrite32(base + ScalerPulsTrig_offset, v);
}
