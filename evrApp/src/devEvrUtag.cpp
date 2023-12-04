/*************************************************************************\
* Copyright (c) 2023 European Spallation Source ERIC (ESS), Lund, Sweden
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Joao Paulo Martins <joaopaulosm@gmail.com>
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

#include <int64outRecord.h>
#include <epicsVersion.h>

#include "devObj.h"
#include "evr/evr.h"

#include "linkoptions.h"

#include <stdexcept>
#include <string>

struct priv {
    EVR* evr;
    char obj[30];
    int event;
    epicsUTag utag;
};

static const
linkOptionDef utagdef[] =
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

    mrf::auto_ptr<priv> p(new priv);
    p->utag=0;
    p->event=0;

    if (linkOptionsStore(utagdef, p.get(), link->value.instio.string, 0))
        throw std::runtime_error("Couldn't parse link string");

    mrf::Object *O=mrf::Object::getObject(p->obj);
    if(!O) {
        errlogPrintf("%s: failed to find object '%s'\n", prec->name, p->obj);
        return S_db_errArg;
    }
    p->evr=dynamic_cast<EVR*>(O);
    if(!p->evr)
        throw std::runtime_error("Failed to lookup device");

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

static long process_int64out(int64outRecord *prec)
{
    priv *p=static_cast<priv*>(prec->dpvt);
    long ret=0;
try {

    // Inject the value as the UTAG reference
    p->utag = static_cast<epicsUTag>(prec->val);
    p->evr->setUtag(p->utag, p->event);
#ifdef DBR_UTAG
    prec->utag = static_cast<epicsUInt64>(p->utag);
#endif

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
long add_int64out(struct dbCommon *precord)
{
    return add_record(precord, &((struct int64outRecord*)precord)->out);
}

dsxt dxtINT64UTAGEVR={add_int64out,del_record};
static common_dset devINT64UTAGEVR = {
  6, NULL,
  dset_cast(&init_dset<&dxtINT64UTAGEVR>),
  (DEVSUPFUN) init_record_empty,
  (DEVSUPFUN) NULL,
  dset_cast(&process_int64out),
  NULL };

extern "C" {

epicsExportAddress(dset,devINT64UTAGEVR);

}
