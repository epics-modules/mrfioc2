
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*

#include <longinRecord.h>
#include <longoutRecord.h>

#include "cardmap.h"
#include "evr/evr.h"
#include "evr/prescaler.h"

#include <stdexcept>

extern "C" {

static long init_in(longinRecord *pli);
static long read_long(longinRecord *pli);

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  read_long;
} devLIEVRPreScaler = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_in,
  NULL,
  (DEVSUPFUN) read_long
};
epicsExportAddress(dset,devLIEVRPreScaler);

static long init_out(longoutRecord *plo);
static long write_long(longoutRecord *plo);

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  write_long;
} devLOEVRPreScaler = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_out,
  NULL,
  (DEVSUPFUN) write_long
};
epicsExportAddress(dset,devLOEVRPreScaler);

}; // extern "C"

static long init_record(dbCommon *prec, DBLINK* lnk)
{
try {

  EVR* card=getEVR<EVR>(lnk->value.vmeio.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  PreScaler* scaler=card->prescaler(lnk->value.vmeio.signal);
  if(!card)
    throw std::runtime_error("Prescaler id# is out of range");

  prec->dpvt=static_cast<void*>(scaler);

  return 0;

} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
  return S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

static long init_in(longinRecord *pli)
{
  return init_record((dbCommon*)pli, &pli->inp);
}

static long init_out(longoutRecord *plo)
{
  return init_record((dbCommon*)plo, &plo->out);
}

static long read_long(longinRecord *pli)
{
try {

  PreScaler* scaler=static_cast<PreScaler*>(pli->dpvt);

  pli->val=scaler->prescaler();

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)pli, e.what());
  return 1;
}
}

static long write_long(longoutRecord *plo)
{
try {

  PreScaler* scaler=static_cast<PreScaler*>(plo->dpvt);

  scaler->setPrescaler(plo->val);

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)plo, e.what());
  return 1;
}
}
