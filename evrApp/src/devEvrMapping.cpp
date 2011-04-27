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

#include <mrfCommon.h> // for mrfDisableRecord

#include "devObj.h"
#include "evr/evr.h"
#include "linkoptions.h"

#include <stdexcept>
#include <string>

/**@file devEvrMapping.cpp
 *
 * A special device support to handle arbitrary mappings in the EVR
 * mapping ram.  This is intended to be used with the special codes
 * only (ie heartbeat, or prescaler reset), but can be used to map
 * arbitrary functions to arbitrary event codes.
 *
 * The function code is given as the signal in the VME_IO output link
 * and the event code is given in the VAL field.
 *
 * The meaning of the function code is implimentation specific except
 * that 0 means no event and must disable a mapping.
 */

/***************** Mapping record ******************/

struct map_priv {
    EVR* card;
    epicsUInt32 last_code;
    epicsUInt32 last_func;
    char obj[30];
    int next_func;
};

static const
linkOptionEnumType funcEnum[] = {
    {"FIFO",     127},
    //{"Latch TS",     126}, // Not supported
    {"Blink",    125},
    {"Forward",  124},
    {"Stop Log", 123},
    {"Log",      122},
    {"Heartbeat",101},
    {"Reset PS", 100},
    {"TS reset",  99},
    {"TS tick",   98},
    {"Shift 1",   97},
    {"Shift 0",   96},
    {NULL,0}
};

static const
linkOptionDef eventdef[] = 
{
    linkString  (map_priv, obj , "OBJ"  , 1, 0),
    linkEnum    (map_priv, next_func, "Func"  , 1, 0, funcEnum),
    linkOptionEnd
};

static long add_lo(dbCommon* praw)
{
    longoutRecord *prec=(longoutRecord*)praw;
    long ret=0;
try {
    assert(prec->out.type==INST_IO);

    std::auto_ptr<map_priv> priv(new map_priv);

    if (linkOptionsStore(eventdef, priv.get(), prec->out.value.instio.string, 0))
        throw std::runtime_error("Couldn't parse link string");

    priv->last_code=0;
    priv->last_func=priv->next_func;

    mrf::Object *O=mrf::Object::getObject(priv->obj);
    if(!O) {
        errlogPrintf("%s: failed to find object '%s'\n", praw->name, priv->obj);
        return S_db_errArg;
    }
    priv->card=dynamic_cast<EVR*>(O);
    if(!priv->card) {
        errlogPrintf("%s: object '%s' is not an EVR\n", praw->name, priv->obj);
        return S_db_errArg;
    }

    praw->dpvt = (void*)priv.release();

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
long del_lo(dbCommon* raw)
{
    delete (map_priv*)raw->dpvt;
    return 0;
}

static long write_lo(longoutRecord* prec)
{
    epicsUInt32  dummy;  // Dummy status variable
try {
    map_priv* priv=static_cast<map_priv*>(prec->dpvt);

    if (!priv)
        return -2;

    epicsUInt32 func=priv->next_func;

    epicsUInt32 code=prec->val;

    if( func==priv->last_func && code==priv->last_code )
        return 0;

    priv->card->specialSetMap(priv->last_code,priv->last_func,false);

    bool restore=false;
    try {
        priv->card->specialSetMap(code,func,true);
    } catch (std::runtime_error& e){
        restore=true;
    }

    if (restore && func==priv->last_func) {
        // Can  (try) to recover previous setting unless
        // function (OUT link) changed.
        priv->card->specialSetMap(priv->last_code,priv->last_func,true);

        prec->val = priv->last_code;
        dummy = recGblSetSevr((dbCommon *)prec, WRITE_ALARM, MAJOR_ALARM);

        return -5;
    } else if (restore) {
        // Can't recover
        prec->val = 0;
        dummy = recGblSetSevr((dbCommon *)prec, WRITE_ALARM, INVALID_ALARM);
        return -5;
    }

    priv->last_code=code;
    priv->last_func=func;

    return 0;

} catch(std::exception& e) {
    prec->val=0;
    recGblRecordError(S_db_noMemory, (void*)prec, e.what());
    return S_db_noMemory;
}
}

/********************** DSETs ***********************/

extern "C" {

dsxt dxtLOEVRMap={&add_lo,&del_record_delete<map_priv>};
static
common_dset devLOEVRMap = {
    5,
    NULL,
    dset_cast(&init_dset<&dxtLOEVRMap>),
    (DEVSUPFUN) init_record_empty,
    NULL,
    (DEVSUPFUN) write_lo
};
epicsExportAddress(dset,devLOEVRMap);

};
