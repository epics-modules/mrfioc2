
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

/***************** Longin/Longout *****************/

static long add_record(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
try {
  assert(lnk->type==VME_IO);

  EVR* card=getEVR<EVR>(lnk->value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  property<EVR,epicsUInt32> *prop;
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
