
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
#include <menuConvert.h>

#include "cardmap.h"
#include "evr/pulser.h"
#include "dsetshared.h"

#include <stdexcept>
#include <string>
#include <cfloat>

/***************** analog *****************/

static long add_record_analog(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
try {
  assert(lnk->type==AB_IO);

  EVR* card=getEVR<EVR>(lnk->value.abio.link);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Pulser* pul=card->pulser(lnk->value.abio.adapter);
  if(!pul)
    throw std::runtime_error("Failed to lookup pulser");

  property<Pulser,double> *prop;
  if (prec->dpvt) {
    prop=static_cast<property<Pulser,double>* >(prec->dpvt);
    prec->dpvt=NULL;
  } else
    prop=new property<Pulser,double>;

  std::string parm(lnk->value.abio.parm);

  if( parm=="Delay" ){
    *prop=property<Pulser,double>(
        pul,
        &Pulser::delay,
        &Pulser::setDelay
    );
  }else if( parm=="Width" ){
    *prop=property<Pulser,double>(
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
  ret=S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  ret=S_db_noMemory;
}
  return ret;
}

/******************* binary *********************/

static long add_record_binary(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
try {
  assert(lnk->type==AB_IO);

  EVR* card=getEVR<EVR>(lnk->value.abio.link);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Pulser* pul=card->pulser(lnk->value.abio.adapter);
  if(!pul)
    throw std::runtime_error("Failed to lookup pulser");

  property<Pulser,bool> *prop;
  if (prec->dpvt) {
    prop=static_cast<property<Pulser,bool>* >(prec->dpvt);
    prec->dpvt=NULL;
  } else
    prop=new property<Pulser,bool>;

  std::string parm(lnk->value.abio.parm);

  if( parm=="Enable" ){
    *prop=property<Pulser,bool>(
        pul,
        &Pulser::enabled,
        &Pulser::enable
    );
  }else if( parm=="Polarity" ){
    *prop=property<Pulser,bool>(
        pul,
        &Pulser::polarityInvert,
        &Pulser::setPolarityInvert
    );
  }else
    throw std::runtime_error("Invalid parm string in link");

  prec->dpvt=static_cast<void*>(prop);

  return 2;

} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
  ret=S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  ret=S_db_noMemory;
}
  return ret;
}

/******************* long ***********************/

static long add_record_long(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
try {
  assert(lnk->type==AB_IO);

  EVR* card=getEVR<EVR>(lnk->value.abio.link);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Pulser* pul=card->pulser(lnk->value.abio.adapter);
  if(!pul)
    throw std::runtime_error("Failed to lookup pulser");

  property<Pulser,epicsUInt32> *prop;
  if (prec->dpvt) {
    prop=static_cast<property<Pulser,epicsUInt32>* >(prec->dpvt);
    prec->dpvt=NULL;
  } else
    prop=new property<Pulser,epicsUInt32>;

  std::string parm(lnk->value.abio.parm);

  if( parm=="Delay" ){
    *prop=property<Pulser,epicsUInt32>(
        pul,
        &Pulser::delayRaw,
        &Pulser::setDelayRaw
    );
  }else if( parm=="Width" ){
    *prop=property<Pulser,epicsUInt32>(
        pul,
        &Pulser::widthRaw,
        &Pulser::setWidthRaw
    );
  }else if( parm=="Prescaler" ){
    *prop=property<Pulser,epicsUInt32>(
        pul,
        &Pulser::prescaler,
        &Pulser::setPrescaler
    );
  }else
    throw std::runtime_error("Invalid parm string in link");

  prec->dpvt=static_cast<void*>(prop);

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

/*************** boiler plate *******************/

/***************** AI/AO *****************/

static long add_ai(dbCommon *raw)
{
  aiRecord *prec=(aiRecord*)raw;
  return add_record_analog((dbCommon*)prec, &prec->inp);
}

static long add_ao(dbCommon *raw)
{
  aoRecord *prec=(aoRecord*)raw;
  return add_record_analog((dbCommon*)prec, &prec->out);
}

extern "C" {

dsxt dxtAIEVRPulser={add_ai,del_record_empty};
static
common_dset devAIEVRPulser = {
  6,
  NULL,
  dset_cast(&init_dset<&dxtAIEVRPulser>),
  (DEVSUPFUN) init_record_empty,
  dset_cast(&dsetshared<Pulser>::get_ioint_info<double>),
  dset_cast(&dsetshared<Pulser>::read_ai),
  NULL
};
epicsExportAddress(dset,devAIEVRPulser);

dsxt dxtAOEVRPulser={add_ao,del_record_empty};
static
common_dset devAOEVRPulser = {
  6,
  NULL,
  dset_cast(&init_dset<&dxtAOEVRPulser>),
  (DEVSUPFUN) init_record_return2,
  NULL,
  dset_cast(&dsetshared<Pulser>::write_ao),
  NULL
};
epicsExportAddress(dset,devAOEVRPulser);

};

/***************** BI/BO *****************/

static long add_bi(dbCommon *prec)
{
  biRecord *pbi=(biRecord*)prec;
  return add_record_binary((dbCommon*)pbi, &pbi->inp);
}

static long add_bo(dbCommon *prec)
{
  boRecord *pbo=(boRecord*)prec;
  return add_record_binary((dbCommon*)pbo, &pbo->out);
}

extern "C" {

dsxt dxtBIEVRPulser={add_bi,del_record_empty};
static
common_dset devBIEVRPulser = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtBIEVRPulser>),
  (DEVSUPFUN) init_record_return2,
  dset_cast(&dsetshared<Pulser>::get_ioint_info<bool>),
  dset_cast(&dsetshared<Pulser>::read_bi)
};
epicsExportAddress(dset,devBIEVRPulser);

dsxt dxtBOEVRPulser={add_bo,del_record_empty};
static
common_dset devBOEVRPulser = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtBOEVRPulser>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  dset_cast(&dsetshared<Pulser>::write_bo)
};
epicsExportAddress(dset,devBOEVRPulser);

};

/***************** Longin/Longout *****************/

static long add_li(dbCommon *prec)
{
  longinRecord *pli=(longinRecord*)prec;
  return add_record_long((dbCommon*)pli, &pli->inp);
}

static long add_lo(dbCommon *prec)
{
  longoutRecord *plo=(longoutRecord*)prec;
  return add_record_long((dbCommon*)plo, &plo->out);
}

extern "C" {

dsxt dxtLIEVRPulser={add_li,del_record_empty};
static
common_dset devLIEVRPulser = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtLIEVRPulser>),
  (DEVSUPFUN) init_record_empty,
  dset_cast(&dsetshared<Pulser>::get_ioint_info<epicsUInt32>),
  dset_cast(&dsetshared<Pulser>::read_li)
};
epicsExportAddress(dset,devLIEVRPulser);

dsxt dxtLOEVRPulser={add_lo,del_record_empty};
static
common_dset devLOEVRPulser = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtLOEVRPulser>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  dset_cast(&dsetshared<Pulser>::write_lo)
};
epicsExportAddress(dset,devLOEVRPulser);

};

