/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <mbbiDirectRecord.h>
#include <mbboDirectRecord.h>

#include "devObj.h"

using namespace mrf;

/************** mbbiDirect *************/

template<typename T>
static long read_mbbidir_from_integer(mbbiDirectRecord* prec)
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

// mbbidir uint32

OBJECT_DSET(MBBIDirFromUINT32,
            (&add_record_inp<mbbiDirectRecord,epicsUInt32>),
            &del_record_property,
            &init_record_empty,
            &read_mbbidir_from_integer<epicsUInt32>,
            NULL);

// mbbidir uint16

OBJECT_DSET(MBBIDirFromUINT16,
            (&add_record_inp<mbbiDirectRecord,epicsUInt16>),
            &del_record_property,
            &init_record_empty,
            &read_mbbidir_from_integer<epicsUInt16>,
            NULL);


// mbbodir uint32

template<typename I>
static long write_mbbodir_from_integer(mbboDirectRecord* prec)
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


OBJECT_DSET(MBBODirFromUINT32,
            (&add_record_out<mbboDirectRecord,epicsUInt32>),
            &del_record_property,
            &init_record_empty,
            &write_mbbodir_from_integer<epicsUInt32>,
            NULL);

// mbbodir uint16


OBJECT_DSET(MBBODirFromUINT16,
            (&add_record_out<mbboDirectRecord,epicsUInt16>),
            &del_record_property,
            &init_record_empty,
            &write_mbbodir_from_integer<epicsUInt16>,
            NULL);

#include <epicsExport.h>
extern "C" {
 OBJECT_DSET_EXPORT(MBBIDirFromUINT32);
 OBJECT_DSET_EXPORT(MBBIDirFromUINT16);
 OBJECT_DSET_EXPORT(MBBODirFromUINT32);
 OBJECT_DSET_EXPORT(MBBODirFromUINT16);
}
