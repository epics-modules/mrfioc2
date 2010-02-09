
#include <cstdlib>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>

#include <aoRecord.h>
#include <longinRecord.h>
#include <longoutRecord.h>

#include <mrfCommon.h> // for mrfDisableRecord

#include "cardmap.h"
#include "evr/evr.h"
#include "evr/prescaler.h"

#include <stdexcept>

/**@file devPreScaler.cpp
 *
 * Device support for accessing PreScaler instances.
 *
 * Uses VME_IO links. '#Cxx Syy @'
 * xx - card number
 * yy - Prescaler number
 */

static long init_record(dbCommon *prec, DBLINK* lnk)
{
  long ret=0;
try {
  assert(lnk->type==VME_IO);

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
  ret=S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  ret=S_db_noMemory;
}
  mrfDisableRecord(prec);
  return ret;
}

static long init_li(longinRecord *prec)
{
  return init_record((dbCommon*)prec, &prec->inp);
}

static long init_lo(longoutRecord *prec)
{
  return init_record((dbCommon*)prec, &prec->out);
}

static long init_ao(aoRecord *prec)
{
  return init_record((dbCommon*)prec, &prec->out);
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

static long write_analog(aoRecord *prec)
{
try {

  PreScaler* scaler=static_cast<PreScaler*>(prec->dpvt);

  double val=scaler->owner.clock();

  val /= prec->val;

  scaler->setPrescaler(val);

  prec->rval=val;
  prec->rbv=scaler->prescaler();

  // Check that input frequency can be exactly represented.
  // The cut off here is arbitrary since there is no good way
  // to pass it in.
  if( val - ((double)(int)val) > 0.5 )
    recGblSetSevr(prec,SOFT_ALARM,MINOR_ALARM);

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return 1;
}
}

extern "C" {

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
  (DEVSUPFUN) init_li,
  NULL,
  (DEVSUPFUN) read_long
};
epicsExportAddress(dset,devLIEVRPreScaler);

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
  (DEVSUPFUN) init_lo,
  NULL,
  (DEVSUPFUN) write_long
};
epicsExportAddress(dset,devLOEVRPreScaler);

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  write_long;
  DEVSUPFUN  special_linconv;
} devAOEVRPreScaler = {
  6,
  NULL,
  NULL,
  (DEVSUPFUN) init_ao,
  NULL,
  (DEVSUPFUN) write_analog,
  NULL
};
epicsExportAddress(dset,devAOEVRPreScaler);

}; // extern "C"
