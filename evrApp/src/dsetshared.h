
#ifndef DSETSHARED_H_INC
#define DSETSHARED_H_INC

#include "property.h"

/* Device support helpers */

template<typename T, typename R>
static inline
T* getdpvt(R *p)
{
  T* tmp=static_cast<T*>(p->dpvt);
  if (tmp) {
    p->dpvt = NULL;
    return tmp;
  } else
    return new T;
};

template<typename T, typename R>
static inline
void setdpvt(R *p, T* t)
{
  if (p->dpvt)
    throw std::logic_error("dpvt already set");
  p->dpvt = static_cast<void*>(t);
};

/* A common dset structure to cover all Base types */
struct common_dset{
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  readwrite;
  DEVSUPFUN  special_linconv;
};

/* Templateized device support functions */

/* global init(int) which does nothing but install
 * extended device support functions.
 */
template<dsxt* D>
static inline
long init_dset(int i)
{
  if (i==0) devExtend(D);
  return 0;
}

extern "C" {

// does nothing but return 0
long init_record_empty(void *);
// does nothing but return 2
long init_record_return2(void *);
// does nothing and returns 0
long del_record_empty(dbCommon*);

}

/**Extended device support add_record helper
 * which uses the getunit() function pointer
 * to get a device instance using the provided
 * hardware link string.
 * The property string (string reference argument)
 * is used to search the property list for
 * the appropriate property() instance which will be
 * stored in prec->dpvt
 */
template<class C,typename P>
static long add_record_property(
                       dbCommon *prec,
                       DBLINK* lnk,
                       C* (*getunit)(const char*, std::string&),
                       const prop_entry<C,P>* List)
{
  long ret=0;
  property<C,P> *prop=NULL;
try {
  assert(lnk->type==INST_IO);

  prop = getdpvt<property<C,P> >(prec);

  std::string parm;

  C* pul=(*getunit)(lnk->value.instio.string, parm);

  *prop = find_prop(List, parm, pul);

  if (!prop->valid())
    throw std::runtime_error("Invalid parm string in link");

  setdpvt(prec, prop);

  return 0;

} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
  ret=S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  ret=S_db_noMemory;
}
  delete prop;
  return ret;
}

template<class C,typename P>
static long get_ioint_info_property(int dir,dbCommon* prec,IOSCANPVT* io)
{
if (!prec->dpvt) return -1;
try {
  property<C,P> *prop=static_cast<property<C,P>*>(prec->dpvt);

  *io = prop->update();

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

/************** Analog *************/

// When converting between VAL and RVAL the following
// convention is used.  (ROFF omitted when RVAL is double)
// VAL = ((RVAL+ROFF) * ASLO + AOFF) * ESLO + EOFF
// RVAL = ((VAL - EOFF)/ESLO - AOFF)/ASLO + ROFF

template<class C>
static long read_ai_property(aiRecord* prec)
{
if (!prec->dpvt) return -1;
try {
  property<C,double> *priv=static_cast<property<C,double>*>(prec->dpvt);

  prec->val = priv->get(); // Read "raw" value

  if(prec->aslo!=0)
    prec->val*=prec->aslo;
  prec->val+=prec->aoff;

  if(prec->linr==menuConvertLINEAR){
    if(prec->eslo!=0)
      prec->val*=prec->eslo;
    prec->val+=prec->eoff;
  }

  return 2;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

template<class C>
static long write_ao_property(aoRecord* prec)
{
if (!prec->dpvt) return -1;
try {
  property<C,double> *priv=static_cast<property<C,double>*>(prec->dpvt);

  double val=prec->val;

  if(prec->linr==menuConvertLINEAR){
    val-=prec->eoff;
    if(prec->eslo!=0)
      val/=prec->eslo;
  }

  val-=prec->aoff;
  if(prec->aslo!=0)
    val/=prec->aslo;

  priv->set(val);

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

/************** Long *************/

template<class C>
static long read_li_property(longinRecord* prec)
{
if (!prec->dpvt) return -1;
try {
  property<C,epicsUInt32> *priv=static_cast<property<C,epicsUInt32>*>(prec->dpvt);

  prec->val = priv->get();

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

template<class C>
static long write_lo_property(longoutRecord* prec)
{
if (!prec->dpvt) return -1;
try {
  property<C,epicsUInt32> *priv=static_cast<property<C,epicsUInt32>*>(prec->dpvt);

  priv->set(prec->val);

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

/************** Binary *************/

template<class C>
static long read_bi_property(biRecord* prec)
{
if (!prec->dpvt) return -1;
try {
  property<C,bool> *priv=static_cast<property<C,bool>*>(prec->dpvt);

  prec->rval = priv->get();

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

template<class C>
static long write_bo_property(boRecord* prec)
{
if (!prec->dpvt) return -1;
try {
  property<C,bool> *priv=static_cast<property<C,bool>*>(prec->dpvt);

  priv->set(prec->rval);

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
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

template<typename REC, int i=sizeof(((REC*)0)->dpvt)>
struct dsxt_cast_helper {

  static inline
  DSXTFUN
  dsxt_cast(long (*fn)(REC*))
  {
    return (DSXTFUN)fn;
  }
};

template<typename REC>
static inline
DSXTFUN
dsxt_cast(long (*fn)(REC*))
{
  return dsxt_cast_helper<REC>::dsxt_cast(fn);
}

#endif // DSETSHARED_H_INC
