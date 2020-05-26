/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <longinRecord.h>
#include <longoutRecord.h>

#include "devObj.h"

using namespace mrf;

/************** longin *************/

template<typename T>
static long read_li_from_integer(longinRecord* prec)
{
if (!prec->dpvt) {(void)recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM); return -1; }
CurrentRecord cur(prec);
try {
    addr<T> *priv=(addr<T>*)prec->dpvt;

    {
        scopedLock<mrf::Object> g(*priv->O);
        prec->val = priv->P->get();
    }

    return 0;
}CATCH(S_dev_badArgument)
}

// li uint32

OBJECT_DSET(LIFromUINT32,
            (&add_record_inp<longinRecord,epicsUInt32>),
            &del_record_property,
            &init_record_empty,
            &read_li_from_integer<epicsUInt32>,
            NULL);

// li uint16

OBJECT_DSET(LIFromUINT16,
            (&add_record_inp<longinRecord,epicsUInt16>),
            &del_record_property,
            &init_record_empty,
            &read_li_from_integer<epicsUInt16>,
            NULL);

// li bool

OBJECT_DSET(LIFromBool,
            (&add_record_inp<longinRecord,bool>),
            &del_record_property,
            &init_record_empty,
            &read_li_from_integer<bool>,
            NULL);


// lo uint32

template<typename I>
static long write_lo_from_integer(longoutRecord* prec)
{
if (!prec->dpvt) {(void)recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM); return -1; }
CurrentRecord cur(prec);
try {
    addr<I> *priv=(addr<I>*)prec->dpvt;

    {
        scopedLock<mrf::Object> g(*priv->O);
        priv->P->set(prec->val);

        if(priv->rbv)
            prec->val = priv->P->get();
    }

    return 0;
}CATCH(S_dev_badArgument)
}


OBJECT_DSET(LOFromUINT32,
            (&add_record_out<longoutRecord,epicsUInt32>),
            &del_record_property,
            &init_record_empty,
            &write_lo_from_integer<epicsUInt32>,
            NULL);

// lo uint16


OBJECT_DSET(LOFromUINT16,
            (&add_record_out<longoutRecord,epicsUInt16>),
            &del_record_property,
            &init_record_empty,
            &write_lo_from_integer<epicsUInt16>,
            NULL);

// lo bool


OBJECT_DSET(LOFromBool,
            (&add_record_out<longoutRecord,bool>),
            &del_record_property,
            &init_record_empty,
            &write_lo_from_integer<bool>,
            NULL);

#include <epicsExport.h>
extern "C" {
 OBJECT_DSET_EXPORT(LIFromUINT32);
 OBJECT_DSET_EXPORT(LIFromUINT16);
 OBJECT_DSET_EXPORT(LIFromBool);
 OBJECT_DSET_EXPORT(LOFromUINT32);
 OBJECT_DSET_EXPORT(LOFromUINT16);
 OBJECT_DSET_EXPORT(LOFromBool);
}
