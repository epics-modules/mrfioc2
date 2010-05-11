
#ifndef PROPERTY_H_INC
#define PROPERTY_H_INC

#include <stdexcept>

#include <devSup.h>
#include <dbScan.h>
#include <dbAccess.h>
#include <dbCommon.h>
#include <recGbl.h>

#include <aoRecord.h>
#include <aiRecord.h>
#include <boRecord.h>
#include <biRecord.h>
#include <longoutRecord.h>
#include <longinRecord.h>

#include <menuConvert.h>

/**@brief Getter/Setter access to a pair of member functions
 *
 * Acts as an adapter for a pair of class member functions
 * of the form P a=x->get() and x->set(S) where S==P by default.
 *
 * It also presents a function IOSCANPVT p=x->update() which can
 * be used for notification if the value which would be returned
 * by x->get() has changed.
 *
 */
template<class C,typename P,typename S=P>
class property
{
public:
  typedef C class_type;
  typedef P reading_type;
  typedef S setting_type;

  typedef void (C::*setter_t)(S);
  typedef P (C::*getter_t)() const;
  typedef IOSCANPVT (C::*updater_t)();

  property() :
    inst(0), setter(0), getter(0),updater(0) {};

  property(class_type* o,getter_t g, setter_t s=0, updater_t u=0) :
    inst(o), setter(s), getter(g),updater(u) {};

  property(const property& o) :
    inst(o.inst), setter(o.setter), getter(o.getter), updater(o.updater) {};

  property(const property& o, class_type* i) :
    inst(i), setter(o.setter), getter(o.getter), updater(o.updater) {};

  property& operator=(const property& o)
  {
    inst=o.inst; setter=o.setter; getter=o.getter; updater=o.updater;
    return *this;
  }

  void set_instance(class_type* i) { inst=i; }

  bool valid() const { return !!inst; }

  P get() const
  {
    return (inst->*getter)();
  };

  void set(S v)
  {
    if(!setter) return;
    (inst->*setter)(v);
  };

  IOSCANPVT update()
  {
    if(!updater) return 0;
    return (inst->*updater)();
  };

private:
  class_type* inst;
  setter_t setter;
  getter_t getter;
  updater_t updater;
};

//! An entry in a list of properties
template<class C,typename P,typename S=P>
struct prop_entry {
  const char* name;
  property<C,P,S> prop;
};

//! Search a list of properties (terminated by {NULL, property()})
template<class C,typename P,typename S>
property<C,P,S>
find_prop(const prop_entry<C,P,S>* list, std::string& name, C* c)
{
  for(; list->name; list++) {
    if (name==list->name)
      return property<C,P,S>(list->prop, c);
  }
  return property<C,P,S>();
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


template<dsxt* D>
static inline
long init_dset(int i)
{
  if (i==0) devExtend(D);
  return 0;
}

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

extern "C" {

long init_record_empty(void *);
long init_record_return2(void *);
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

#endif // PROPERTY_H_INC
