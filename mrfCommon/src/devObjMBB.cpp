/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <mbbiRecord.h>
#include <mbboRecord.h>

#include "devObj.h"

using namespace mrf;

/************** mbbi *************/

template<typename T>
static long read_mbbi_from_integer(mbbiRecord* prec)
{
if (!prec->dpvt) return -1;
try {
    addr<T> *priv=(addr<T>*)prec->dpvt;

    {
        scopedLock<mrf::Object> g(*priv->O);
        prec->rval = priv->P->get();
    }

    return 0;
} catch(std::exception& e) {
    epicsPrintf("%s: read error: %s\n", prec->name, e.what());
    return S_db_noMemory;
}
}

// mbbi uint32

OBJECT_DSET(MBBIFromUINT32,
            (&add_record_inp<mbbiRecord,epicsUInt32>),
            &del_record_property,
            &init_record_empty,
            &read_mbbi_from_integer<epicsUInt32>,
            NULL);

// mbbi uint16

OBJECT_DSET(MBBIFromUINT16,
            (&add_record_inp<mbbiRecord,epicsUInt16>),
            &del_record_property,
            &init_record_empty,
            &read_mbbi_from_integer<epicsUInt16>,
            NULL);


// mbbo uint32

template<typename I>
static long write_mbbo_from_integer(mbboRecord* prec)
{
if (!prec->dpvt) return -1;
try {
    addr<I> *priv=(addr<I>*)prec->dpvt;

    {
        scopedLock<mrf::Object> g(*priv->O);
        priv->P->set(prec->rval);

        prec->rbv = priv->P->get();
    }

    return 0;
} catch(std::exception& e) {
    epicsPrintf("%s: read error: %s\n", prec->name, e.what());
    return S_db_noMemory;
}
}


OBJECT_DSET(MBBOFromUINT32,
            (&add_record_out<mbboRecord,epicsUInt32>),
            &del_record_property,
            &init_record_return2,
            &write_mbbo_from_integer<epicsUInt32>,
            NULL);

// mbbo uint16


OBJECT_DSET(MBBOFromUINT16,
            (&add_record_out<mbboRecord,epicsUInt16>),
            &del_record_property,
            &init_record_return2,
            &write_mbbo_from_integer<epicsUInt16>,
            NULL);

#include <epicsExport.h>

OBJECT_DSET_EXPORT(MBBIFromUINT32);
OBJECT_DSET_EXPORT(MBBIFromUINT16);
OBJECT_DSET_EXPORT(MBBOFromUINT32);
OBJECT_DSET_EXPORT(MBBOFromUINT16);
