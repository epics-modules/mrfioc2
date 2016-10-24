/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <stdlib.h>
#include <string.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>
#include <errlog.h>
#include "linkoptions.h"
#include "devObj.h"

#include <stringinRecord.h>

#include "evr/evr.h"

#include <stdexcept>
#include <string>

struct ts_priv {
    EVR* evr;
    char obj[30];
    epicsUInt32 code;
    epicsUInt32 last_bad;
};

static const
linkOptionDef eventdef[] = 
{
    linkString  (ts_priv, obj ,  "OBJ"  , 1, 0),
    linkInt32   (ts_priv, code , "Code" , 0, 0),
    linkOptionEnd
};

/***************** Stringin (Timestamp) *****************/

static
long stringin_add(dbCommon *praw)
{
    stringinRecord *prec=(stringinRecord*)(praw);
    long ret=0;
try {
    assert(prec->inp.type==INST_IO);
    std::auto_ptr<ts_priv> priv(new ts_priv);
    priv->code=0;
    priv->last_bad=0;

    if (linkOptionsStore(eventdef, priv.get(), prec->inp.value.instio.string, 0))
        throw std::runtime_error("Couldn't parse link string");

    mrf::Object *O=mrf::Object::getObject(priv->obj);
    if(!O) {
        errlogPrintf("%s: failed to find object '%s'\n", praw->name, priv->obj);
        return S_db_errArg;
    }
    priv->evr=dynamic_cast<EVR*>(O);
    if(!priv->evr)
        throw std::runtime_error("Failed to lookup device");

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


static long read_si(stringinRecord* prec)
{
    if(!prec->dpvt)
        return S_db_errArg;
try {
    ts_priv *priv=static_cast<ts_priv*>(prec->dpvt);
    if(!priv)
        return -2;

    epicsTimeStamp ts;

    if(!priv->evr->getTimeStamp(&ts,priv->code)){
        strncpy(prec->val,"EVR time unavailable",sizeof(prec->val));
        return S_dev_deviceTMO;
    }

    if (ts.secPastEpoch==priv->last_bad)
        return 0;

    size_t r=epicsTimeToStrftime(prec->val,
                                 sizeof(prec->val),
                                 "%a, %d %b %Y %H:%M:%S %z",
                                 &ts);
    if(r==0||r==sizeof(prec->val)){
        recGblRecordError(S_dev_badArgument, (void*)prec,
                          "Format string resulted in error");
        priv->last_bad=ts.secPastEpoch;
        return S_dev_badArgument;
    }

    if(prec->tse==epicsTimeEventDeviceTime) {
        prec->time = ts;
    }

    return 0;
} catch(std::exception& e) {
    recGblRecordError(S_db_noMemory, (void*)prec, e.what());
    return S_db_noMemory;
}
}

extern "C" {

dsxt dxtSIEVR={&stringin_add,&del_record_delete<ts_priv>};
static
common_dset devSIEVR = {
    5,
    NULL,
    dset_cast(&init_dset<&dxtSIEVR>),
    (DEVSUPFUN) init_record_empty,
    NULL,
    (DEVSUPFUN) read_si
};
epicsExportAddress(dset,devSIEVR);


};
