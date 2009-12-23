
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

static long long_init_record(dbCommon *prec, DBLINK* lnk)
{
try {
  assert(lnk->type==AB_IO);

  EVR* card=getEVR<EVR>(lnk->value.abio.link);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Pulser* pul=card->pulser(lnk->value.abio.adapter);
  if(!pul)
    throw std::runtime_error("Failed to lookup pulser");

  property<Pulser,epicsUInt32> *prop;
  std::string parm(lnk->value.abio.parm);

  if( parm=="Delay" ){
    prop=new property<Pulser,epicsUInt32>(
        pul,
        &Pulser::delayRaw,
        &Pulser::setDelayRaw
    );
  }else if( parm=="Width" ){
    prop=new property<Pulser,epicsUInt32>(
        pul,
        &Pulser::widthRaw,
        &Pulser::setWidthRaw
    );
  }else if( parm=="Prescaler" ){
    prop=new property<Pulser,epicsUInt32>(
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
  return S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

static long init_li(longinRecord *pli)
{
  return long_init_record((dbCommon*)pli, &pli->inp);
}

static long read_li(longinRecord* pli)
{
try {
  property<Pulser,epicsUInt32> *priv=static_cast<property<Pulser,epicsUInt32>*>(pli->dpvt);

  pli->val = priv->get();

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)pli, e.what());
  return S_db_noMemory;
}
}

static long init_lo(longoutRecord *plo)
{
  return long_init_record((dbCommon*)plo, &plo->out);
}

static long get_ioint_info_li(int dir,dbCommon* prec,IOSCANPVT* io)
{
  return get_ioint_info<Pulser,epicsUInt32,epicsUInt32>(dir,prec,io);
}

static long write_lo(longoutRecord* plo)
{
try {
  property<Pulser,epicsUInt32> *priv=static_cast<property<Pulser,epicsUInt32>*>(plo->dpvt);

  priv->set(plo->val);

  long rbv=priv->get();

  // Probably an indication that this is a 16-bit field
  if(rbv!=plo->val){
    recGblSetSevr((dbCommon*)plo,SOFT_ALARM,MINOR_ALARM);
  }

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)plo, e.what());
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
  DEVSUPFUN  read;
} devLIEVRPulser = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_li,
  (DEVSUPFUN) get_ioint_info_li,
  (DEVSUPFUN) read_li
};
epicsExportAddress(dset,devLIEVRPulser);

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  write;
} devLOEVRPulser = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_lo,
  NULL,
  (DEVSUPFUN) write_lo
};
epicsExportAddress(dset,devLOEVRPulser);

};
