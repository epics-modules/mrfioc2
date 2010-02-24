#include <string>

#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*

#include <longinRecord.h>
#include <longoutRecord.h>

#include <mrfCommon.h> // for mrfDisableRecord

#include "cardmap.h"
#include "evr/evr.h"
#include "evr/input.h"
#include "property.h"

#include <stdexcept>

static long init_record(dbCommon *prec, DBLINK* lnk)
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

  property<Input,epicsUInt32> *prop;
  std::string parm(lnk->value.vmeio.parm);

  if( parm=="External Code" ){
    prop=new property<Input,epicsUInt32>(
        inp,
        &Input::extEvt,
        &Input::extEvtSet
    );
  }else if( parm=="Backwards Code" ){
    prop=new property<Input,epicsUInt32>(
        inp,
        &Input::backEvt,
        &Input::backEvtSet
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

/********** LONGOUT **************/

static long init_longout(longoutRecord *prec)
{
  return init_record((dbCommon*)prec, &prec->out);
}

static long write_longout(longoutRecord *prec)
{
try{
  property<Input,epicsUInt32> *priv=static_cast<property<Input,epicsUInt32>*>(prec->dpvt);

  priv->set(prec->val);

  return 0;

} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}


/************* LONGIN ****************/

static long init_longin(longinRecord *plongin)
{
  return init_record((dbCommon*)plongin, &plongin->inp);
}

static long read_longin(longinRecord *prec)
{
try{
  property<Input,epicsUInt32> *priv=static_cast<property<Input,epicsUInt32>*>(prec->dpvt);

  prec->val=priv->get();

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

/***************** DSET ********************/
extern "C" {

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  read;
} devLIInput = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_longin,
  NULL,
  (DEVSUPFUN) read_longin
};
epicsExportAddress(dset,devLIInput);

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  write;
} devLOInput = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_longout,
  NULL,
  (DEVSUPFUN) write_longout
};
epicsExportAddress(dset,devLOInput);

};
