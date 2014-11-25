/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include "drvemPrescaler.h"

#include <epicsMMIO.h>
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
