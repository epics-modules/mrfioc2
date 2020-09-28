/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
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

#define CATCH(RET) catch(alarm_exception& e) {\
    (void)recGblSetSevr(prec, e.status(), e.severity());\
    return (RET);\
    } catch(std::exception& e) {\
    (void)recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM);\
    epicsPrintf("%s: error: %s\n", prec->name, e.what());\
    return (RET);\
    }

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
    char klass[30];
    char parent[30];
    int rbv;
    mrf::Object *O;
};

MRFCOMMON_API extern const
linkOptionEnumType readbackEnum[];

template<typename T>
struct addr : public addrBase {
    mrf::auto_ptr<mrf::property<T> > P;
};

MRFCOMMON_API extern const
linkOptionDef objdef[];

struct MRFCOMMON_API CurrentRecord {
    template<typename Rec>
    explicit CurrentRecord(Rec* prec)
    {
        (void)prec->dpvt;
        set((dbCommon*)prec);
    }
    ~CurrentRecord();
    static
    dbCommon* get();
    void set(dbCommon* prec);
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

    mrf::auto_ptr<addr<P> > a;
    if(prec->dpvt) {
        a.reset((addr<P>*)prec->dpvt);
        prec->dpvt=0;
    } else
        a.reset(new addr<P>);

    a->rbv=0;
    a->obj[0] = a->prop[0] = a->klass[0] = a->parent[0] = '\0';

    if(linkOptionsStore(objdef, (void*)(addrBase*)a.get(),
                        lnk->value.instio.string, 0)) {
        errlogPrintf("%s: Invalid Input link", prec->name);
        return S_db_errArg;
    }

    Object *o;
    try {
        Object::create_args_t args;
        args["PARENT"] = a->parent;
        o = Object::getCreateObject(a->obj, a->klass, args);

    } catch(std::exception& e) {
        errlogPrintf("%s: failed to find/create object '%s' : %s\n", prec->name, a->obj, e.what());
        return S_db_errArg;
    }

    mrf::auto_ptr<property<P> > prop = o->getProperty<P>(a->prop);
    if(!prop.get()) {
        errlogPrintf("%s: '%s' lacks property '%s' of required type %s\n",
                     prec->name, o->name().c_str(), a->prop, typeid(P).name());
        return S_db_errArg;
    }

    a->O = o;
    a->P = PTRMOVE(prop);

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
    if (!prop)
        return 0;
    prec->dpvt = 0;
    delete prop;
    return 0;
}

static inline
long del_record_property(dbCommon* prec)
{
    using namespace mrf;
    addrBase *prop=static_cast<addrBase*>(prec->dpvt);
    if (!prop)
        return 0;
    prec->dpvt = 0;
    delete prop;
    return 0;
}

static inline long get_ioint_info_property(int, dbCommon* prec, IOSCANPVT* io)
{
    using namespace mrf;
if (!prec->dpvt) {(void)recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM); return -1; }
try {
    addrBase *prop=static_cast<addrBase*>(prec->dpvt);

    mrf::auto_ptr<property<IOSCANPVT> > up = prop->O->getProperty<IOSCANPVT>(prop->prop);

    if(up.get()) {
        *io = up->get();
    } else {
        errlogPrintf("%s Warning: I/O Intr not supported by PROP=%s\n", prec->name, prop->prop);
    }

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
