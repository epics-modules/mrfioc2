/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
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

#include <longoutRecord.h>
#include <eventRecord.h>

#include "devObj.h"
#include "evr/evr.h"

#include "linkoptions.h"

#include <stdexcept>
#include <string>

/***************** Event *****************/

struct priv {
    EVR* evr;
    char obj[30];
    int event;
};
typedef struct priv priv;

static const
linkOptionDef eventdef[] =
{
    linkString  (priv, obj , "OBJ"  , 1, 0),
    linkInt32   (priv, event, "Code", 1, 0),
    linkOptionEnd
};

static
long add_record(struct dbCommon *prec, struct link* link)
{
    long ret=0;
try {
    assert(link->type==INST_IO);

    std::auto_ptr<priv> p(new priv);
    p->event=0;

    if (linkOptionsStore(eventdef, p.get(), link->value.instio.string, 0))
        throw std::runtime_error("Couldn't parse link string");

    mrf::Object *O=mrf::Object::getObject(p->obj);
    if(!O) {
        errlogPrintf("%s: failed to find object '%s'\n", prec->name, p->obj);
        return S_db_errArg;
    }
    p->evr=dynamic_cast<EVR*>(O);
    if(!p->evr)
        throw std::runtime_error("Failed to lookup device");

    if (!p->evr->interestedInEvent(p->event, true))
        throw std::runtime_error("Failed to register interest");

    prec->dpvt=(void*)p.release();

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

static
long del_record(struct dbCommon *prec)
{
    priv *p=static_cast<priv*>(prec->dpvt);
    long ret=0;
    if (!p) return 0;
try {

    p->evr->interestedInEvent(p->event, false);
    delete p;
    prec->dpvt=0;

} catch(std::runtime_error& e) {
    recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
    ret=S_dev_noDevice;
} catch(std::exception& e) {
    recGblRecordError(S_db_noMemory, (void*)prec, e.what());
    ret=S_db_noMemory;
}
    return ret;
}

static
long
get_ioint_info(int, dbCommon* prec,IOSCANPVT* io)
{
    if(!prec->dpvt)
        return S_db_errArg;
    priv *p=static_cast<priv*>(prec->dpvt);
    long ret=0;
try {

    if(!p) return 1;

    *io=p->evr->eventOccurred(p->event);

    return 0;
} catch(std::runtime_error& e) {
    recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
    ret=S_dev_noDevice;
} catch(std::exception& e) {
    recGblRecordError(S_db_noMemory, (void*)prec, e.what());
    ret=S_db_noMemory;
}
    *io=NULL;
    return ret;
}

static long process_longout(longoutRecord *prec)
{
    priv *p=static_cast<priv*>(prec->dpvt);
    long ret=0;
try {

    if (prec->val>=0 && prec->val<=255)
        post_event(prec->val);

    if(prec->tse==epicsTimeEventDeviceTime){
        p->evr->getTimeStamp(&prec->time,p->event);
    }

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

static long process_event(eventRecord *prec)
{
    priv *p=static_cast<priv*>(prec->dpvt);
    long ret=0;
try {
    if(prec->tse==epicsTimeEventDeviceTime){
        p->evr->getTimeStamp(&prec->time,p->event);
    }

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

static
long add_longout(struct dbCommon *precord)
{
    return add_record(precord, &((struct longoutRecord*)precord)->out);
}

static
long add_event(struct dbCommon *precord)
{
    return add_record(precord, &((struct eventRecord*)precord)->inp);
}

dsxt dxtLOEventEVR={add_longout,del_record};
static common_dset devLOEventEVR = {
  6, NULL,
  dset_cast(&init_dset<&dxtLOEventEVR>),
  (DEVSUPFUN) init_record_empty,
  (DEVSUPFUN) &get_ioint_info,
  dset_cast(&process_longout),
  NULL };

dsxt dxtEVEventEVR={add_event,del_record};
static common_dset devEVEventEVR = {
  6, NULL,
  dset_cast(&init_dset<&dxtEVEventEVR>),
  (DEVSUPFUN) init_record_empty,
  (DEVSUPFUN) &get_ioint_info,
  dset_cast(&process_event),
  NULL };

extern "C" {

epicsExportAddress(dset,devLOEventEVR);
epicsExportAddress(dset,devEVEventEVR);

}
