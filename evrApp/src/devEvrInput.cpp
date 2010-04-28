
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*

#include <biRecord.h>
#include <boRecord.h>
#include <mbboRecord.h>

#include "cardmap.h"
#include "evr/evr.h"
#include "evr/input.h"
#include "property.h"

#include <stdexcept>
#include <string>

/***************** binary *****************/

static long add_record_binary(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
  property<Input,bool> *prop=NULL;
try {
  assert(lnk->type==VME_IO);

  EVR* card=getEVR<EVR>(lnk->value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Input* inp=card->input(lnk->value.vmeio.signal);

  if(!inp)
    throw std::runtime_error("Input Type is out of range");

  if (prec->dpvt) {
    prop=static_cast<property<Input,bool>* >(prec->dpvt);
    prec->dpvt=NULL;
  } else
    prop=new property<Input,bool>;

  std::string parm(lnk->value.vmeio.parm);

  if( parm=="Active Level" ){
    *prop=property<Input,bool>(
        inp,
        &Input::levelHigh,
        &Input::levelHighSet
    );
  }else if( parm=="Active Edge" ){
    *prop=property<Input,bool>(
        inp,
        &Input::edgeRise,
        &Input::edgeRiseSet
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

/***************** long *************************/

static long add_record_long(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
  property<Input,epicsUInt32> *prop=NULL;
try {
  assert(lnk->type==VME_IO);

  EVR* card=getEVR<EVR>(lnk->value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Input* inp=card->input(lnk->value.vmeio.signal);

  if(!inp)
    throw std::runtime_error("Input Type is out of range");

  if (prec->dpvt) {
    prop=static_cast<property<Input,epicsUInt32>* >(prec->dpvt);
    prec->dpvt=NULL;
  } else
    prop=new property<Input,epicsUInt32>;

  std::string parm(lnk->value.vmeio.parm);

  if( parm=="External Code" ){
    *prop=property<Input,epicsUInt32>(
        inp,
        &Input::extEvt,
        &Input::extEvtSet
    );
  }else if( parm=="Backwards Code" ){
    *prop=property<Input,epicsUInt32>(
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
  if (!prec->dpvt) delete prop;
  return ret;
}

/***************** multi-binary *****************/

static long add_record_multi_binary(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
  property<Input,epicsUInt16> *prop=NULL;
try {
  assert(lnk->type==VME_IO);

  EVR* card=getEVR<EVR>(lnk->value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Input* inp=card->input(lnk->value.vmeio.signal);
  if(!inp)
    throw std::runtime_error("Failed to lookup device input");

  if (prec->dpvt) {
    prop=static_cast<property<Input,epicsUInt16>* >(prec->dpvt);
    prec->dpvt=NULL;
  } else
    prop=new property<Input,epicsUInt16>;

  std::string parm(lnk->value.vmeio.parm);

  if( parm=="External Mode" ){
    *prop=property<Input,epicsUInt16>(
        inp,
        &Input::extModeraw,
        &Input::extModeSetraw
    );
  }else if( parm=="Backwards Mode" ){
    *prop=property<Input,epicsUInt16>(
        inp,
        &Input::backModeraw,
        &Input::backModeSetraw
    );
  }else if( parm=="DBus Mask" ){
    *prop=property<Input,epicsUInt16>(
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
  if (!prec->dpvt) delete prop;
  return ret;
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
    break;
  default:
    throw std::out_of_range("Invalid Input trigger mode");
  }

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

/*************** boilerplate *****************/

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

dsxt dxtBIInput={add_bi,del_record_empty};
static
common_dset devBIInput = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtBIInput>),
  (DEVSUPFUN) init_record_empty,
  dset_cast(&dsetshared<Input>::get_ioint_info<bool>),
  dset_cast(&dsetshared<Input>::read_bi)
};
epicsExportAddress(dset,devBIInput);

dsxt dxtBOInput={add_bo,del_record_empty};
static
common_dset devBOInput = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtBOInput>),
  (DEVSUPFUN) init_record_return2,
  NULL,
  dset_cast(&dsetshared<EVR>::write_bo)
};
epicsExportAddress(dset,devBOInput);

};

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

dsxt dxtLIInput={add_li,del_record_empty};
static
common_dset devLIInput = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtLIInput>),
  (DEVSUPFUN) init_record_empty,
  dset_cast(&dsetshared<Input>::get_ioint_info<epicsUInt32>),
  dset_cast(&dsetshared<Input>::read_li)
};
epicsExportAddress(dset,devLIInput);

dsxt dxtLOInput={add_lo,del_record_empty};
static
common_dset devLOInput = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtLOInput>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  dset_cast(&dsetshared<Input>::write_lo)
};
epicsExportAddress(dset,devLOInput);

};

/***************** MBBO *****************/

static long add_mbbo(dbCommon *praw)
{
  mbboRecord *prec=(mbboRecord*)praw;
  return add_record_multi_binary((dbCommon*)prec, &prec->out);
}

extern "C" {

dsxt dxtMBBOInput={add_mbbo,del_record_empty};
static
common_dset devMBBOInput = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtMBBOInput>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  (DEVSUPFUN) write_mbbo
};
epicsExportAddress(dset,devMBBOInput);

};

