
#ifndef DSETSHARED_H_INC
#define DSETSHARED_H_INC

#include "property.h"
#include <devLib.h>

/* Device support helpers */

/** Returns an instance or T from the record dpvt field
 *  if available, or a new instance if dpvt is empty.
 *
 *  When this function returns dpvt will be NULL
 */
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

/** Store the instance of T in dpvt.
 * checks that dpvt==NULL first
 */
template<typename T, typename R>
static inline
void setdpvt(R *p, T* t)
{
  if (p->dpvt)
    throw std::logic_error("dpvt already set");
  p->dpvt = static_cast<void*>(t);
};

/** A common dset structure to cover all Base types */
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

/** global init(int) which does nothing but install
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

//! does nothing but return 0
long init_record_empty(void *);
//! does nothing but return 2
long init_record_return2(void *);
//! does nothing and returns 0
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
  // At this point prec->dpvt is null
  // so 'prop' must be free'd on error
  // to avoid a memory leak

  std::string parm;

  C* pul=(*getunit)(lnk->value.instio.string, parm);

  *prop = find_prop(List, parm, pul);

  if (!prop->valid())
    throw std::runtime_error("Invalid parm string in link");

  // prec->dpvt is set again to indicate
  // This also serves to indicate sucessful
  // initialization to other dset functions
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

/********** property dset table ************/

/** Can't really simplify this any more w/ templating
 * so the remainder can be done w/ a cpp macro
 */

/**@brief Create an add_record function for a combination of recordtype and class
 *
 *@param rectype record type (ie ai, bo, longin)
 *@param proptype property data type (ie double, bool, epicsUInt32)
 *@param linkfld name of the hardware link field (ie inp or out)
 *@param classname Class to be wrapped
 *@param getunit function to return an instance of the class (Class* (*fnptr)(const char*,std::string&)
 *@param proplist a list of name and property<>() pairs (struct prop_entry[])
 *
 * The function will be called add_classname_rectype (ie add_Pulser_ai)
 */
#define PROPERTY_DSET_ADDFN(rectype, proptype, linkfld, classname, getunit, proplist) \
static long add_ ## classname ## _ ## rectype(dbCommon *raw) \
{ \
  rectype ## Record *prec=(rectype ## Record*)raw; \
  return add_record_property<classname,proptype>((dbCommon*)prec, \
                         &prec->linkfld, getunit, proplist); \
}

/**@brief Create dset and dsxt tables for a combination of recordtype and class
 *
 *@param rectype record type (ie ai, bo, longin)
 *@param proptype property data type (ie double, bool, epicsUInt32)
 *@param classname Class to be wrapped
 *@param addfunc an add_record function (see above)
 *@param initfunc an init_record function (ie init_record_empty or init_record_return2)
 *@param procfn the read or write fn
 *
 * The dset structure will be called devRECCLASS (ie devaiPulser).
 */
#define PROPERTY_DSET_TABLE(rectype, proptype, classname, addfunc, initfunc, procfn) \
dsxt dxt ## rectype ## classname={addfunc,del_record_empty}; \
static common_dset dev ## rectype ## classname = { \
  6, NULL, \
  dset_cast(&init_dset<&dxt ## rectype ## classname>), \
  (DEVSUPFUN) initfunc, \
  dset_cast(&get_ioint_info_property<classname,proptype>), \
  dset_cast(&procfn), \
  NULL };\
epicsExportAddress(dset,dev ## rectype ## classname);

/********** Analog ************/

#define PROPERTY_DSET_AI(classname, getunit, proplist) \
PROPERTY_DSET_ADDFN(ai, double, inp, classname, getunit, proplist); \
PROPERTY_DSET_TABLE(ai, double, classname, \
                    add_ ## classname ## _ai, \
                    init_record_empty, \
                    read_ai_property<classname> )

#define PROPERTY_DSET_AO(classname, getunit, proplist) \
PROPERTY_DSET_ADDFN(ao, double, out, classname, getunit, proplist); \
PROPERTY_DSET_TABLE(ao, double, classname, \
                    add_ ## classname ## _ao, \
                    init_record_return2, \
                    write_ao_property<classname> )

/*********** Long integer *********************/

#define PROPERTY_DSET_LONGIN(classname, getunit, proplist) \
PROPERTY_DSET_ADDFN(longin, epicsUInt32, inp, classname, getunit, proplist); \
PROPERTY_DSET_TABLE(longin, epicsUInt32, classname, \
                    add_ ## classname ## _longin, \
                    init_record_empty, \
                    read_li_property<classname> )

#define PROPERTY_DSET_LONGOUT(classname, getunit, proplist) \
PROPERTY_DSET_ADDFN(longout, epicsUInt32, out, classname, getunit, proplist); \
PROPERTY_DSET_TABLE(longout, epicsUInt32, classname, \
                    add_ ## classname ## _longout, \
                    init_record_empty, \
                    write_lo_property<classname> )

/************ binary ******************/

#define PROPERTY_DSET_BI(classname, getunit, proplist) \
PROPERTY_DSET_ADDFN(bi, bool, inp, classname, getunit, proplist); \
PROPERTY_DSET_TABLE(bi, bool, classname, \
                    add_ ## classname ## _bi, \
                    init_record_empty, \
                    read_bi_property<classname> )

#define PROPERTY_DSET_BO(classname, getunit, proplist) \
PROPERTY_DSET_ADDFN(bo, bool, out, classname, getunit, proplist); \
PROPERTY_DSET_TABLE(bo, bool, classname, \
                    add_ ## classname ## _bo, \
                    init_record_return2, \
                    write_bo_property<classname> )

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

//~~~~template<typename REC, int i=sizeof(((REC*)0)->dpvt)>
template<typename REC>
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
