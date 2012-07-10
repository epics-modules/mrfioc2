/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <aiRecord.h>
#include <aoRecord.h>

#include "devObj.h"

using namespace mrf;

/************** AI *************/

// When converting between VAL and RVAL the following
// convention is used.  (ROFF omitted when RVAL is double)
// VAL = ((RVAL+ROFF) * ASLO + AOFF) * ESLO + EOFF
// RVAL = ((VAL - EOFF)/ESLO - AOFF)/ASLO + ROFF

template<typename T>
static long read_ai_from_real(aiRecord* prec)
{
if (!prec->dpvt) return -1;
try {
    addr<T> *priv=(addr<T>*)prec->dpvt;

    {
        scopedLock<mrf::Object> g(*priv->O);
        prec->val = priv->P->get();
    }

    if(prec->aslo!=0)
        prec->val*=prec->aslo;
    prec->val+=prec->aoff;

    if(prec->linr==menuConvertLINEAR){
        if(prec->eslo!=0)
            prec->val*=prec->eslo;
        prec->val+=prec->eoff;
    }

    prec->udf = 0;
    return 2;
} catch(std::exception& e) {
    epicsPrintf("%s: read error: %s\n", prec->name, e.what());
    return S_db_noMemory;
}
}

// ai double

OBJECT_DSET(AIFromDouble,
            (&add_record_inp<aiRecord,double>),
            &del_record_property,
            &init_record_empty,
            &read_ai_from_real<double>,
            NULL);

template<typename T>
static long read_ai_from_integer(aiRecord* prec)
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

// ai uint32

OBJECT_DSET(AIFromUINT32,
            (&add_record_inp<aiRecord,epicsUInt32>),
            &del_record_property,
            &init_record_empty,
            &read_ai_from_integer<epicsUInt32>,
            NULL);

// ai uint16

OBJECT_DSET(AIFromUINT16,
            (&add_record_inp<aiRecord,epicsUInt16>),
            &del_record_property,
            &init_record_empty,
            &read_ai_from_integer<epicsUInt16>,
            NULL);



/************** AO *************/

template<typename T>
static long write_ao_from_real(aoRecord* prec)
{
if (!prec->dpvt) return -1;
try {
    addr<T> *priv=(addr<T>*)prec->dpvt;

    double val=prec->val;

    if(prec->linr==menuConvertLINEAR){
        val-=prec->eoff;
        if(prec->eslo!=0)
            val/=prec->eslo;
    }

    val-=prec->aoff;
    if(prec->aslo!=0)
        val/=prec->aslo;

    {
        scopedLock<mrf::Object> g(*priv->O);
        priv->P->set(val);

        if (!priv->rbv)
            return 0;

        prec->val = priv->P->get();
    }

    if(prec->aslo!=0)
        prec->val*=prec->aslo;
    prec->val+=prec->aoff;

    if(prec->linr==menuConvertLINEAR){
        if(prec->eslo!=0)
            prec->val*=prec->eslo;
        prec->val+=prec->eoff;
    }

    return 0;
} catch(std::exception& e) {
    epicsPrintf("%s: read error: %s\n", prec->name, e.what());
    return S_db_noMemory;
}
}

// ao double

OBJECT_DSET(AOFromDouble,
            (&add_record_out<aoRecord,double>),
            &del_record_property,
            &init_record_return2,
            &write_ao_from_real<double>,
            NULL);

template<typename T>
static long write_ao_from_integer(aoRecord* prec)
{
if (!prec->dpvt) return -1;
try {
    addr<T> *priv=(addr<T>*)prec->dpvt;

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

// ao uint32

OBJECT_DSET(AOFromUINT32,
            (&add_record_out<aoRecord,epicsUInt32>),
            &del_record_property,
            &init_record_empty,
            &write_ao_from_integer<epicsUInt32>,
            NULL);

// ao uint16

OBJECT_DSET(AOFromUINT16,
            (&add_record_out<aoRecord,epicsUInt16>),
            &del_record_property,
            &init_record_empty,
            &write_ao_from_integer<epicsUInt16>,
            NULL);


#include <epicsExport.h>

OBJECT_DSET_EXPORT(AIFromDouble);
OBJECT_DSET_EXPORT(AIFromUINT32);
OBJECT_DSET_EXPORT(AIFromUINT16);
OBJECT_DSET_EXPORT(AOFromDouble);
OBJECT_DSET_EXPORT(AOFromUINT32);
OBJECT_DSET_EXPORT(AOFromUINT16);
