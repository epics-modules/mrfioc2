
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*

#include <biRecord.h>
#include <boRecord.h>

#include <mrfCommon.h> // for mrfDisableRecord

#include "cardmap.h"
#include "evr/evr.h"
#include "evr/input.h"
#include "property.h"

#include <stdexcept>
#include <string>

/***************** BI/BO *****************/

static long binary_init_record(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
try {
  assert(lnk->type==VME_IO);

  EVR* card=getEVR<EVR>(lnk->value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Input* inp=card->input(lnk->value.vmeio.signal);

  if(!inp)
    throw std::runtime_error("Input Type is out of range");

  property<Input,bool> *prop;
  std::string parm(lnk->value.vmeio.parm);

  if( parm=="Active Level" ){
    prop=new property<Input,bool>(
        inp,
        &Input::levelHigh,
        &Input::levelHighSet
    );
  }else if( parm=="Active Edge" ){
    prop=new property<Input,bool>(
        inp,
        &Input::edgeRise,
        &Input::edgeRiseSet
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
  mrfDisableRecord(prec);
  return ret;
}

static long init_bi(biRecord *pbi)
{
  return binary_init_record((dbCommon*)pbi, &pbi->inp);
}

static long read_bi(biRecord* pbi)
{
try {
  property<Input,bool> *priv=static_cast<property<Input,bool>*>(pbi->dpvt);

  pbi->rval = priv->get();

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)pbi, e.what());
  return S_db_noMemory;
}
}

static long init_bo(boRecord *pbo)
{
  return binary_init_record((dbCommon*)pbo, &pbo->out);
}

static long get_ioint_info_bi(int dir,dbCommon* prec,IOSCANPVT* io)
{
  return get_ioint_info<EVR,bool,bool>(dir,prec,io);
}

static long write_bo(boRecord* pbo)
{
try {
  property<Input,bool> *priv=static_cast<property<Input,bool>*>(pbo->dpvt);

  priv->set(pbo->rval);

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)pbo, e.what());
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
} devBIInput = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_bi,
  (DEVSUPFUN) get_ioint_info_bi,
  (DEVSUPFUN) read_bi
};
epicsExportAddress(dset,devBIInput);

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  write;
} devBOInput = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_bo,
  NULL,
  (DEVSUPFUN) write_bo
};
epicsExportAddress(dset,devBOInput);

};