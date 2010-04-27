
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
#include "evr/pulser.h"
#include "property.h"

#include <stdexcept>
#include <string>

/***************** Longin/Longout *****************/

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

static long add_li(dbCommon *prec)
{
  longinRecord *pli=(longinRecord*)prec;
  return add_record((dbCommon*)pli, &pli->inp);
}

static long add_lo(dbCommon *prec)
{
  longoutRecord *plo=(longoutRecord*)prec;
  return add_record((dbCommon*)plo, &plo->out);
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
