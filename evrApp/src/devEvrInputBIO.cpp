
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*

#include <biRecord.h>
#include <boRecord.h>

#include "cardmap.h"
#include "evr/evr.h"
#include "evr/input.h"
#include "property.h"

#include <stdexcept>
#include <string>

/***************** BI/BO *****************/

static long add_record(dbCommon *prec, DBLINK* lnk)
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
  return ret;
}

static long add_bi(dbCommon *prec)
{
  biRecord *pbi=(biRecord*)prec;
  return add_record((dbCommon*)pbi, &pbi->inp);
}

static long add_bo(dbCommon *prec)
{
  boRecord *pbo=(boRecord*)prec;
  return add_record((dbCommon*)pbo, &pbo->out);
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
  (DEVSUPFUN) init_record_empty,
  NULL,
  dset_cast(&dsetshared<EVR>::write_bo)
};
epicsExportAddress(dset,devBOInput);

};
