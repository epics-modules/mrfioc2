
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
#include "evr/pulser.h"
#include "property.h"

#include <stdexcept>
#include <string>

/***************** BI/BO *****************/

static long add_record(dbCommon *prec, DBLINK* lnk)
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

dsxt dxtBIEVRPulser={add_bi,del_record_empty};
static
common_dset devBIEVRPulser = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtBIEVRPulser>),
  (DEVSUPFUN) init_record_empty,
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
