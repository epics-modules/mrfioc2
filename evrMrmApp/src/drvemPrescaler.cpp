/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */


#include "mrf/databuf.h"
//#include <epicsMMIO.h>
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
    if(v>0xffff)
        throw std::out_of_range("prescaler setting is out of range");

    nat_iowrite32(base, v);
}
