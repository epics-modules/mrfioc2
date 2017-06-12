/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <biRecord.h>
#include <boRecord.h>

#include "devObj.h"

using namespace mrf;

/************** bi *************/

template<typename T>
static long read_bi_from_integer(biRecord* prec)
{
if (!prec->dpvt) return -1;
try {
    addr<T> *priv=(addr<T>*)prec->dpvt;

    {
        scopedLock<mrf::Object> g(*priv->O);
        prec->rval = priv->P->get();
        if(prec->mask) prec->rval &= prec->mask;
    }

    return 0;
}CATCH(S_dev_badArgument)
}

// bi uint32

OBJECT_DSET(BIFromUINT32,
            (&add_record_inp<biRecord,epicsUInt32>),
            &del_record_property,
            &init_record_empty,
            &read_bi_from_integer<epicsUInt32>,
            NULL);

// bi uint16

OBJECT_DSET(BIFromUINT16,
            (&add_record_inp<biRecord,epicsUInt16>),
            &del_record_property,
            &init_record_empty,
            &read_bi_from_integer<epicsUInt16>,
            NULL);

// bi bool

OBJECT_DSET(BIFromBool,
            (&add_record_inp<biRecord,bool>),
            &del_record_property,
            &init_record_empty,
            &read_bi_from_integer<bool>,
            NULL);


// bo uint32

template<typename I>
static long write_bo_from_integer(boRecord* prec)
{
if (!prec->dpvt) return -1;
try {
    addr<I> *priv=(addr<I>*)prec->dpvt;

    {
        scopedLock<mrf::Object> g(*priv->O);
        priv->P->set((prec->rval != 0));

        prec->rbv = priv->P->get();
    }
    if(priv->rbv) {
        prec->rval = prec->rbv;
        if (prec->mask) {
            if(prec->rval & prec->mask)
                prec->val=1;
            else
                prec->val=0;
        } else
            prec->val = !!prec->rval;
    }

    return 0;
}CATCH(S_dev_badArgument)
}


OBJECT_DSET(BOFromUINT32,
            (&add_record_out<boRecord,epicsUInt32>),
            &del_record_property,
            &init_record_return2,
            &write_bo_from_integer<epicsUInt32>,
            NULL);

// bo uint16


OBJECT_DSET(BOFromUINT16,
            (&add_record_out<boRecord,epicsUInt16>),
            &del_record_property,
            &init_record_return2,
            &write_bo_from_integer<epicsUInt16>,
            NULL);

// bo bool


OBJECT_DSET(BOFromBool,
            (&add_record_out<boRecord,bool>),
            &del_record_property,
            &init_record_return2,
            &write_bo_from_integer<bool>,
            NULL);

#include <epicsExport.h>
extern "C" {
 OBJECT_DSET_EXPORT(BIFromUINT32);
 OBJECT_DSET_EXPORT(BIFromUINT16);
 OBJECT_DSET_EXPORT(BIFromBool);
 OBJECT_DSET_EXPORT(BOFromUINT32);
 OBJECT_DSET_EXPORT(BOFromUINT16);
 OBJECT_DSET_EXPORT(BOFromBool);
}
