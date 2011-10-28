/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */
#ifndef DEVOBJ_H
#define DEVOBJ_H

#include <stdlib.h>
#include <dbAccess.h>
#include <dbScan.h>
#include <link.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>
#include <errlog.h>
#include <menuConvert.h>

#include "mrf/object.h"
#include "linkoptions.h"
#include "mrfCommon.h"

#include <stdexcept>
#include <string>

/* Device support related casting functions */

template<typename REC>
static inline
DEVSUPFUN
dset_cast(long (*fn)(REC*))
{
  return (DEVSUPFUN)fn;
}

DEVSUPFUN
static inline
dset_cast(long (*fn)(int,dbCommon*,IOSCANPVT*))
{
  return (DEVSUPFUN)fn;
}

DEVSUPFUN
static inline
dset_cast(long (*fn)(int))
{
  return (DEVSUPFUN)fn;
}

typedef long (*DSXTFUN)(dbCommon*);

template<typename REC>
static inline
DSXTFUN
dsxt_cast(long (*fn)(REC*))
{
    return (DSXTFUN)fn;
}

struct common_dset{
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  readwrite;
  DEVSUPFUN  special_linconv;
};

struct addrBase {
    char obj[30];
    char prop[30];
    int rbv;
    mrf::Object *O;
};

static const
linkOptionEnumType readbackEnum[] = { {"No",0}, {"Yes",1} };

template<typename T>
struct addr : public addrBase {
    std::auto_ptr<mrf::property<T> > P;
};

static const
linkOptionDef objdef[] =
{
    linkString  (addrBase, obj , "OBJ"  , 1, 0),
    linkString  (addrBase, prop , "PROP"  , 1, 0),
    linkEnum    (addrBase, rbv, "RB"   , 0, 0, readbackEnum),
    linkOptionEnd
};

template<dsxt* D>
static inline
long init_dset(int i)
{
  if (i==0) devExtend(D);
  return 0;
}

static inline
long init_record_empty(void *)
{
  return 0;
}

static inline
long init_record_return2(void *)
{
  return 2;
}

static inline
long del_record_empty(dbCommon*)
{
  return 0;
}

template<typename P>
static long add_record_property(
                       dbCommon *prec,
                       DBLINK* lnk)
{
    using namespace mrf;
try {
    if(lnk->type!=INST_IO)
        return S_db_errArg;

    std::auto_ptr<addr<P> > a;
    if(prec->dpvt) {
        a.reset((addr<P>*)prec->dpvt);
        prec->dpvt=0;
    } else
        a.reset(new addr<P>);

    a->rbv=0;

    if(linkOptionsStore(objdef, (void*)(addrBase*)a.get(),
                        lnk->value.instio.string, 0)) {
        errlogPrintf("%s: Invalid Input link", prec->name);
        return S_db_errArg;
    }

    Object *o = Object::getObject(a->obj);
    if(!o) {
        errlogPrintf("%s: failed to find object '%s'\n", prec->name, a->obj);
        return S_db_errArg;
    }

    std::auto_ptr<property<P> > prop = o->getProperty<P>(a->prop);
    if(!prop.get()) {
        errlogPrintf("%s: '%s' lacks property '%s'\n", prec->name, o->name().c_str(), a->prop);
        return S_db_errArg;
    }

    a->O = o;
    a->P = prop;

    prec->dpvt = (void*)a.release();

    return 0;
} catch (std::exception& e) {
    errlogPrintf("%s: add_record failed: %s\n", prec->name, e.what());
    return S_db_errArg;
}
}

template<typename REC, typename T>
static inline
long add_record_inp(dbCommon *pcom)
{
    REC *prec=(REC*)pcom;
    return add_record_property<T>(pcom, &prec->inp);
}

template<typename REC, typename T>
static inline
long add_record_out(dbCommon *pcom)
{
    REC *prec=(REC*)pcom;
    return add_record_property<T>(pcom, &prec->out);
}

template<typename DT>
static inline
long del_record_delete(dbCommon* prec)
{
    DT *prop=static_cast<DT*>(prec->dpvt);
    prec->dpvt = 0;
    delete prop;
    return 0;
}

static inline
long del_record_property(dbCommon* prec)
{
    using namespace mrf;
    addrBase *prop=static_cast<addrBase*>(prec->dpvt);
    prec->dpvt = 0;
    delete prop;
    return 0;
}

static inline long get_ioint_info_property(int dir,dbCommon* prec,IOSCANPVT* io)
{
    using namespace mrf;
if (!prec->dpvt) return -1;
try {
    addrBase *prop=static_cast<addrBase*>(prec->dpvt);

    std::auto_ptr<property<IOSCANPVT> > up = prop->O->getProperty<IOSCANPVT>(prop->prop);

    if(up.get())
        *io = up->get();

    return 0;
} catch(std::exception& e) {
    recGblRecordError(S_db_noMemory, (void*)prec, e.what());
    return S_db_noMemory;
}
}

#define OBJECT_DSET(NAME, ADD, DEL, INIT, WRITE, LINR) \
dsxt dxt ## NAME={ADD,DEL}; \
static common_dset dev ## NAME = { \
  6, NULL, \
  dset_cast(&init_dset<&dxt ## NAME>), \
  (DEVSUPFUN) INIT, \
  (DEVSUPFUN) &get_ioint_info_property, \
  dset_cast(WRITE), \
  LINR }

#define OBJECT_DSET_EXPORT(NAME) epicsExportAddress(dset,dev ## NAME)

#endif // DEVOBJ_H
