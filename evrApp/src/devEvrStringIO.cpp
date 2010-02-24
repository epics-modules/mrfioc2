
#include <stdlib.h>
#include <string.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>

#include <stringinRecord.h>

#include <mrfCommon.h> // for mrfDisableRecord

#include "cardmap.h"
#include "evr/evr.h"

#include <stdexcept>
#include <string>

/***************** Stringin (Timestamp) *****************/

static
long stringin_init(stringinRecord *prec)
{
  long ret=0;
try {
  assert(prec->inp.type==VME_IO);

  EVR* card=getEVR<EVR>(prec->inp.value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  prec->dpvt=static_cast<void*>(card);

  return 0;

} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
  ret=S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  ret=S_db_noMemory;
}
  mrfDisableRecord((dbCommon*)prec);
  return ret;
}


static long read_si(stringinRecord* prec)
{
try {
  EVR *priv=static_cast<EVR*>(prec->dpvt);

  epicsTimeStamp ts;

  if(!priv->getTimeStamp(&ts,TSModeFree)){
    strncpy(prec->val,"EVR time unavailable",sizeof(prec->val));
    return S_dev_deviceTMO;
  }

  size_t r=epicsTimeToStrftime(prec->val,
                               sizeof(prec->val),
                               "%a, %d %b %Y %H:%M:%S %z",
                               &ts);
  if(r==0||r==sizeof(prec->val)){
    recGblRecordError(S_dev_badArgument, (void*)prec, 
      "Format string resulted in error");
    return S_dev_badArgument;
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
  DEVSUPFUN  read;
} devSIEVR = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) stringin_init,
  NULL,
  (DEVSUPFUN) read_si
};
epicsExportAddress(dset,devSIEVR);


};
