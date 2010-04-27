
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
#include "property.h"

#include <stdexcept>
#include <string>
#include <cfloat>

/***************** AI/AO *****************/

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

dsxt dxtAIEVRPulser={add_ai,del_record_empty};
static
common_dset devAIEVRPulser = {
  6,
  NULL,
  dset_cast(&init_dset<&dxtAIEVRPulser>),
  (DEVSUPFUN) init_ai_record,
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
  (DEVSUPFUN) init_ao_record,
  NULL,
  dset_cast(&dsetshared<Pulser>::write_ao),
  NULL
};
epicsExportAddress(dset,devAOEVRPulser);

};
