
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
#include "evr/cml.h"
#include "dsetshared.h"

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

  CML* pul=card->cml(lnk->value.vmeio.signal);
  if(!pul)
    throw std::runtime_error("Failed to lookup CML short registers");

  property<CML,epicsUInt32> *prop;
  if (prec->dpvt) {
    prop=static_cast<property<CML,epicsUInt32>* >(prec->dpvt);
    prec->dpvt=NULL;
  } else
    prop=new property<CML,epicsUInt32>;

  std::string parm(lnk->value.vmeio.parm);
  if( parm=="Pattern Low" ){
    prop=new property<CML,epicsUInt32>(
        pul,
        &CML::getPattern<cmlShortLow>,
        &CML::setPattern<cmlShortLow>
    );
  } else if( parm=="Pattern Rise" ){
    prop=new property<CML,epicsUInt32>(
        pul,
        &CML::getPattern<cmlShortRise>,
        &CML::setPattern<cmlShortRise>
    );
  } else if( parm=="Pattern High" ){
    prop=new property<CML,epicsUInt32>(
        pul,
        &CML::getPattern<cmlShortHigh>,
        &CML::setPattern<cmlShortHigh>
    );
  } else if( parm=="Pattern Fall" ){
    prop=new property<CML,epicsUInt32>(
        pul,
        &CML::getPattern<cmlShortFall>,
        &CML::setPattern<cmlShortFall>
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

dsxt dxtLIEVRCML={add_li,del_record_empty};
static
common_dset devLIEVRCML = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtLIEVRCML>),
  (DEVSUPFUN) init_record_empty,
  dset_cast(&get_ioint_info_property<CML,epicsUInt32>),
  dset_cast(&read_li_property<CML>)
};
epicsExportAddress(dset,devLIEVRCML);

dsxt dxtLOEVRCML={add_lo,del_record_empty};
static
common_dset devLOEVRCML = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtLOEVRCML>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  dset_cast(&write_lo_property<CML>)
};
epicsExportAddress(dset,devLOEVRCML);

};
