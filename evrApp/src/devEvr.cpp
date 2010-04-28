
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>

#include <longinRecord.h>
#include <longoutRecord.h>

#include "cardmap.h"
#include "evr/evr.h"
#include "property.h"

#include <stdexcept>
#include <string>

/***************** long *****************/

static long add_record_long(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
  property<EVR,epicsUInt32> *prop=NULL;
try {
  assert(lnk->type==VME_IO);

  EVR* card=getEVR<EVR>(lnk->value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  if (prec->dpvt) {
    prop=static_cast<property<EVR,epicsUInt32>* >(prec->dpvt);
    prec->dpvt=NULL;
  } else
    prop=new property<EVR,epicsUInt32>;

  std::string parm(lnk->value.vmeio.parm);

  if( parm=="Model" ){
    *prop=property<EVR,epicsUInt32>(
        card,
        &EVR::model
    );
  }else if( parm=="Version" ){
    *prop=property<EVR,epicsUInt32>(
        card,
        &EVR::version
    );
  }else if( parm=="Event Clock TS Div" ){
    *prop=property<EVR,epicsUInt32>(
        card,
        &EVR::uSecDiv
    );
  }else if( parm=="Receive Error Count" ){
    *prop=property<EVR,epicsUInt32>(
        card,
        &EVR::recvErrorCount,
        0,
        &EVR::linkChanged
    );
  }else if( parm=="Timestamp Prescaler" ){
    *prop=property<EVR,epicsUInt32>(
        card,
        &EVR::tsDiv
    );
  }else if( parm=="Timestamp Source" ){
    *prop=property<EVR,epicsUInt32>(
        card,
        &EVR::SourceTSraw,
        &EVR::setSourceTSraw
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
  if (!prec->dpvt) delete prop;
  return ret;
}

/***************** binary *****************/

static long add_record_binary(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
  property<EVR,bool> *prop=NULL;
try {
  assert(lnk->type==VME_IO);

  EVR* card=getEVR<EVR>(lnk->value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  if (prec->dpvt) {
    prop=static_cast<property<EVR,bool>* >(prec->dpvt);
    prec->dpvt=NULL;
  } else
    prop=new property<EVR,bool>;

  std::string parm(lnk->value.vmeio.parm);

  if( parm=="Enable" ){
    *prop=property<EVR,bool>(
        card,
        &EVR::enabled,
        &EVR::enable
    );
  }else if( parm=="PLL Lock Status" ){
    *prop=property<EVR,bool>(
        card,
        &EVR::pllLocked,
        0
    );
  }else if( parm=="Link Status" ){
    *prop=property<EVR,bool>(
        card,
        &EVR::linkStatus,
        0,
        &EVR::linkChanged
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
  if (!prec->dpvt) delete prop;
  return ret;
}

/***************** AI/AO *****************/

static long add_record_analog(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
  property<EVR,double> *prop=NULL;
try {
  assert(lnk->type==VME_IO);

  EVR* card=getEVR<EVR>(lnk->value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  if (prec->dpvt) {
    prop=static_cast<property<EVR,double>* >(prec->dpvt);
    prec->dpvt=NULL;
  } else
    prop=new property<EVR,double>;

  std::string parm(lnk->value.vmeio.parm);

  if( parm=="Clock" ){
    *prop=property<EVR,double>(
        card,
        &EVR::clock,
        &EVR::clockSet
    );
  }else if( parm=="Timestamp Clock" ){
    *prop=property<EVR,double>(
        card,
        &EVR::clockTS,
        &EVR::clockTSSet
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
  if (!prec->dpvt) delete prop;
  return ret;
}

/************* Boiler plate *****************/

/*************** longout/longin *********************/

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

dsxt dxtLIEVR={add_li,del_record_empty};
static
common_dset devLIEVR = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtLIEVR>),
  (DEVSUPFUN) init_record_empty,
  dset_cast(&dsetshared<EVR>::get_ioint_info<epicsUInt32>),
  dset_cast(&dsetshared<EVR>::read_li)
};
epicsExportAddress(dset,devLIEVR);

dsxt dxtLOEVR={add_lo,del_record_empty};
static
common_dset devLOEVR = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtLOEVR>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  dset_cast(&dsetshared<EVR>::write_lo)
};
epicsExportAddress(dset,devLOEVR);

};

/******************* Binary *********************/

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

dsxt dxtBIEVR={add_bi,del_record_empty};
static
common_dset devBIEVR = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtBIEVR>),
  (DEVSUPFUN) init_record_empty,
  dset_cast(&dsetshared<EVR>::get_ioint_info<bool>),
  dset_cast(&dsetshared<EVR>::read_bi)
};
epicsExportAddress(dset,devBIEVR);

dsxt dxtBOEVR={add_bo,del_record_empty};
static
common_dset devBOEVR = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtBOEVR>),
  (DEVSUPFUN) init_record_return2,
  NULL,
  dset_cast(&dsetshared<EVR>::write_bo)
};
epicsExportAddress(dset,devBOEVR);

};

/******************* Analog ********************/

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


static long init_ai_record(dbCommon *)
{
  return 0;
}

static long init_ao_record(dbCommon *)
{
  return 2;
}

extern "C" {

dsxt dxtAIEVR={add_ai,del_record_empty};
static
common_dset devAIEVR = {
  6,
  NULL,
  dset_cast(&init_dset<&dxtAIEVR>),
  (DEVSUPFUN) init_ai_record,
  dset_cast(&dsetshared<EVR>::get_ioint_info<double>),
  dset_cast(&dsetshared<EVR>::read_ai),
  NULL
};
epicsExportAddress(dset,devAIEVR);

dsxt dxtAOEVR={add_ao,del_record_empty};
static
common_dset devAOEVR = {
  6,
  NULL,
  dset_cast(&init_dset<&dxtAOEVR>),
  (DEVSUPFUN) init_ao_record,
  NULL,
  dset_cast(&dsetshared<EVR>::write_ao),
  NULL
};
epicsExportAddress(dset,devAOEVR);

};
