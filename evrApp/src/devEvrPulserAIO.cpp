
#include <stdlib.h>
#include <math.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>

#include <aiRecord.h>
#include <aoRecord.h>

#include "cardmap.h"
#include "evr/pulser.h"
#include "property.h"

#include <stdexcept>
#include <string>
#include <cfloat>

/***************** AI/AO *****************/

static long analog_init_record(dbCommon *prec, DBLINK* lnk)
{
try {
  assert(lnk->type==AB_IO);

  EVR* card=getEVR<EVR>(lnk->value.abio.link);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Pulser* pul=card->pulser(lnk->value.abio.adapter);
  if(!pul)
    throw std::runtime_error("Failed to lookup pulser");

  property<Pulser,double> *prop;
  std::string parm(lnk->value.abio.parm);

  if( parm=="Delay" ){
    prop=new property<Pulser,double>(
        pul,
        &Pulser::delay,
        &Pulser::setDelay
    );
  }else if( parm=="Width" ){
    prop=new property<Pulser,double>(
        pul,
        &Pulser::width,
        &Pulser::setWidth
    );
  }else
    throw std::runtime_error("Invalid parm string in link");

  prec->dpvt=static_cast<void*>(prop);

  return 0;

} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
  return S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

static long init_ai(aiRecord *prec)
{
  return analog_init_record((dbCommon*)prec, &prec->inp);
}

static long read_ai(aiRecord* prec)
{
try {
  property<Pulser,double> *priv=static_cast<property<Pulser,double>*>(prec->dpvt);

  prec->val = priv->get();

  return 2;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

static long init_ao(aoRecord *prec)
{
  long r=analog_init_record((dbCommon*)prec, &prec->out);
  if(r==0)
    return 2;
  return r;
}

static long get_ioint_info_ai(int dir,dbCommon* prec,IOSCANPVT* io)
{
  return get_ioint_info<Pulser,double,double>(dir,prec,io);
}

static long write_ao(aoRecord* prec)
{
try {
  property<Pulser,double> *priv=static_cast<property<Pulser,double>*>(prec->dpvt);

  priv->set(prec->val);

  double rbv=priv->get();

  // NOTE: this test is perhaps too conservative.
  if(fabs(rbv-prec->val)>DBL_EPSILON){
    recGblSetSevr((dbCommon*)prec,SOFT_ALARM,MINOR_ALARM);
  }

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

extern "C" {

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  read;
  DEVSUPFUN  special_linconv;
} devAIEVRPulser = {
  6,
  NULL,
  NULL,
  (DEVSUPFUN) init_ai,
  (DEVSUPFUN) get_ioint_info_ai,
  (DEVSUPFUN) read_ai,
  NULL
};
epicsExportAddress(dset,devAIEVRPulser);

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  write;
  DEVSUPFUN  special_linconv;
} devAOEVRPulser = {
  6,
  NULL,
  NULL,
  (DEVSUPFUN) init_ao,
  NULL,
  (DEVSUPFUN) write_ao,
  NULL
};
epicsExportAddress(dset,devAOEVRPulser);

};
