/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <cstdlib>
#include <cstring>

#include <epicsExport.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <errlog.h>

#include <longoutRecord.h>
#include <stringinRecord.h>

#include "devObj.h"
#include "evr/pulser.h"
#include "linkoptions.h"

#include <stdexcept>
#include <string>

/**@file devEvrPulserMapping.cpp
 *
 * A special device support to handle arbitrary mappings in the EVR
 * mapping ram to one of the pulser units.
 *
 * The pulser id is given as the signal in the AB_IO output link
 * and the event code is given in the VAL field.
 *
 * '#Lcard Apulserid Cmapram Sfunction @'
 */

/***************** Mapping record ******************/

struct map_priv {
    char obj[30];
    Pulser* pulser;
    epicsUInt32 last_code;
    MapType::type func;
};

static const
linkOptionEnumType funcEnum[] = {
    {"Trig", MapType::Trigger},
    {"Set",  MapType::Set},
    {"Reset",MapType::Reset},
    {NULL,0}
};

static const
linkOptionDef eventdef[] = 
{
    linkString  (map_priv, obj , "OBJ"  , 1, 0),
    linkEnum    (map_priv, func, "Func"  , 1, 0, funcEnum),
    linkOptionEnd
};

static long add_lo(dbCommon* praw)
{
    long ret=0;
    longoutRecord *prec = (longoutRecord*)praw;
try {
    assert(prec->out.type==INST_IO);

    std::auto_ptr<map_priv> priv(new map_priv);
    priv->last_code=prec->val;

    if (linkOptionsStore(eventdef, priv.get(), prec->out.value.instio.string, 0))
        throw std::runtime_error("Couldn't parse link string");

    mrf::Object *O=mrf::Object::getObject(priv->obj);
    if(!O) {
        errlogPrintf("%s: failed to find object '%s'\n", praw->name, priv->obj);
        return S_db_errArg;
    }
    priv->pulser=dynamic_cast<Pulser*>(O);
    if(!priv->pulser)
        throw std::runtime_error("Failed to lookup device");

    if(priv->last_code>0 || priv->last_code<=255)
        priv->pulser->sourceSetMap(priv->last_code,priv->func);

    prec->dpvt=(void*)priv.release();

    return 0;

} catch(std::runtime_error& e) {
    recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
    ret=S_dev_noDevice;
} catch(std::exception& e) {
    recGblRecordError(S_db_noMemory, (void*)prec, e.what());
    ret=S_db_noMemory;
}
    return ret;
}

static inline
long del_lo(dbCommon* praw)
{
    if(!praw->dpvt)
        return 0;
    try {
        std::auto_ptr<map_priv> priv((map_priv*)praw->dpvt);

        if(priv->last_code>0 && priv->last_code<=255)
            priv->pulser->sourceSetMap(priv->last_code,MapType::None);

        return 0;
    } catch(std::exception& e) {
        recGblRecordError(S_db_noMemory, (void*)praw, e.what());
    }
    recGblSetSevr(praw, WRITE_ALARM, INVALID_ALARM);
    return S_db_noMemory;
}

static long write_lo(longoutRecord* plo)
{
    map_priv* priv=static_cast<map_priv*>(plo->dpvt);
    try {

        if (!priv)
            return -2;

        epicsUInt32 code=plo->val;

        /**
         * Removing 'code < 0' from if condition since variable 'code' is
         * unsigned and can never be less than 0 - this comparison is always
         * true and therefore superfluous.
         *
         * Changed by: jkrasna
         */
        if(code > 255) {
            recGblSetSevr((dbCommon *)plo, WRITE_ALARM, INVALID_ALARM);
            return 0;
        }

        if( code==priv->last_code )
            return 0;

        //TODO: sanity check to catch overloaded mappings

        priv->pulser->sourceSetMap(priv->last_code,MapType::None);

        if(code!=0)
            priv->pulser->sourceSetMap(code,priv->func);

        priv->last_code=code;

        return 0;

    } catch(std::exception& e) {
        plo->val=0;
        priv->last_code=0;
        recGblSetSevr((dbCommon *)plo, WRITE_ALARM, INVALID_ALARM);
        recGblRecordError(S_db_noMemory, (void*)plo, e.what());
        return S_db_noMemory;
    }
}

/********************** DSETs ***********************/

extern "C" {

dsxt dxtLOEVRPulserMap={add_lo, &del_lo};
static
common_dset devLOEVRPulserMap = {
    5,
    NULL,
    dset_cast(&init_dset<&dxtLOEVRPulserMap>),
    (DEVSUPFUN) &init_record_empty,
    NULL,
    (DEVSUPFUN) &write_lo
};
epicsExportAddress(dset,devLOEVRPulserMap);

};
