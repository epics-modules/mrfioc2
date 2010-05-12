
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

/**@brief Templetized device support functions
 */
template<class C>
struct dsetshared {

  typedef C unit_type;

  template<typename P>
  static long get_ioint_info(int dir,dbCommon* prec,IOSCANPVT* io)
  {
  if (!prec->dpvt) return -1;
  try {
    property<unit_type,P> *prop=static_cast<property<unit_type,P>*>(prec->dpvt);

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

  static long read_ai(aiRecord* prec)
  {
  if (!prec->dpvt) return -1;
  try {
    property<unit_type,double> *priv=static_cast<property<unit_type,double>*>(prec->dpvt);

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

  static long write_ao(aoRecord* prec)
  {
  if (!prec->dpvt) return -1;
  try {
    property<unit_type,double> *priv=static_cast<property<unit_type,double>*>(prec->dpvt);

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

  static long read_li(longinRecord* prec)
  {
  if (!prec->dpvt) return -1;
  try {
    property<unit_type,epicsUInt32> *priv=static_cast<property<unit_type,epicsUInt32>*>(prec->dpvt);

    prec->val = priv->get();

    return 0;
  } catch(std::exception& e) {
    recGblRecordError(S_db_noMemory, (void*)prec, e.what());
    return S_db_noMemory;
  }
  }

  static long write_lo(longoutRecord* prec)
  {
  if (!prec->dpvt) return -1;
  try {
    property<unit_type,epicsUInt32> *priv=static_cast<property<unit_type,epicsUInt32>*>(prec->dpvt);

    priv->set(prec->val);

    return 0;
  } catch(std::exception& e) {
    recGblRecordError(S_db_noMemory, (void*)prec, e.what());
    return S_db_noMemory;
  }
  }

  /************** Binary *************/

  static long read_bi(biRecord* prec)
  {
  if (!prec->dpvt) return -1;
  try {
    property<unit_type,bool> *priv=static_cast<property<unit_type,bool>*>(prec->dpvt);

    prec->rval = priv->get();

    return 0;
  } catch(std::exception& e) {
    recGblRecordError(S_db_noMemory, (void*)prec, e.what());
    return S_db_noMemory;
  }
  }

  static long write_bo(boRecord* prec)
  {
  if (!prec->dpvt) return -1;
  try {
    property<unit_type,bool> *priv=static_cast<property<unit_type,bool>*>(prec->dpvt);

    priv->set(prec->rval);

    return 0;
  } catch(std::exception& e) {
    recGblRecordError(S_db_noMemory, (void*)prec, e.what());
    return S_db_noMemory;
  }
  }

}; // struct dsetshared

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
