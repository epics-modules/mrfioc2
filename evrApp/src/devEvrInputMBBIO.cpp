
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>

#include <mbboRecord.h>

#include "cardmap.h"
#include "evr/evr.h"
#include "evr/input.h"
#include "property.h"

#include <stdexcept>
#include <string>

/***************** mbbo *****************/

static long long_init_record(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
try {
  assert(lnk->type==VME_IO);

  EVR* card=getEVR<EVR>(lnk->value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Input* inp=card->input(lnk->value.vmeio.signal);
  if(!inp)
    throw std::runtime_error("Failed to lookup device input");

  property<Input,epicsUInt16> *prop;
  std::string parm(lnk->value.vmeio.parm);

  if( parm=="External Mode" ){
    prop=new property<Input,epicsUInt16>(
        inp,
        &Input::extModeraw,
        &Input::extModeSetraw
    );
  }else if( parm=="Backwards Mode" ){
    prop=new property<Input,epicsUInt16>(
        inp,
        &Input::backModeraw,
        &Input::backModeSetraw
    );
  }else if( parm=="DBus Mask" ){
    prop=new property<Input,epicsUInt16>(
        inp,
        &Input::dbus,
        &Input::dbusSet
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
  prec->pact=TRUE;
  return ret;
}

static long init_mbbo(mbboRecord *prec)
{
  return long_init_record((dbCommon*)prec, &prec->out);
}

static long write_mbbo(mbboRecord* prec)
{
try {
  property<Input,epicsUInt16> *priv=static_cast<property<Input,epicsUInt16>*>(prec->dpvt);

  switch(prec->rval){
  case TrigNone:
  case TrigLevel:
  case TrigEdge:
    priv->set((epicsUInt16)prec->rval);

    prec->rbv=priv->get();
  default:
    throw std::range_error("Invalid Input trigger mode");
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
  DEVSUPFUN  write;
} devMBBOInput = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_mbbo,
  NULL,
  (DEVSUPFUN) write_mbbo
};
epicsExportAddress(dset,devMBBOInput);

};
