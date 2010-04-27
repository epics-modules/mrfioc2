
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*

#include <aiRecord.h>
#include <aoRecord.h>
#include <menuConvert.h>

#include "cardmap.h"
#include "evr/evr.h"
#include "property.h"

#include <stdexcept>
#include <string>

/***************** AI/AO *****************/

static long add_record(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
try {
  assert(lnk->type==VME_IO);

  EVR* card=getEVR<EVR>(lnk->value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  property<EVR,double> *prop;
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
  return ret;
}

static long add_ai(dbCommon *raw)
{
  aiRecord *prec=(aiRecord*)raw;
  return add_record((dbCommon*)prec, &prec->inp);
}

static long add_ao(dbCommon *raw)
{
  aoRecord *prec=(aoRecord*)raw;
  return add_record((dbCommon*)prec, &prec->out);
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
