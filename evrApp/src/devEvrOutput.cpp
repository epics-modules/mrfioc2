
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*

#include <longinRecord.h>
#include <longoutRecord.h>

#include <mrfCommon.h> // for mrfDisableRecord

#include "cardmap.h"
#include "evr/evr.h"
#include "evr/output.h"
#include "dsetshared.h"

#include <stdexcept>

/**@file devEvrOutput.cpp
 *
 * Device support for accessing Output sub-device instances.
 *
 * Uses AB_IO links. '#Lxx Ayy Czz S0 @'
 * xx - card number
 * yy - Output class (FP, RB, etc.)
 * zz - Output number
 */

static long init_record(dbCommon *prec, DBLINK* lnk)
{
try {
  assert(lnk->type==AB_IO);

  EVR* card=getEVR<EVR>(lnk->value.abio.link);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Output* output=0;

  output=card->output((enum OutputType)lnk->value.abio.adapter,
                      lnk->value.abio.card);

  if(!output)
    throw std::runtime_error("Output Type is out of range");

  prec->dpvt=static_cast<void*>(output);

  return 0;

} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)prec, e.what());
  mrfDisableRecord(prec);
  return S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  mrfDisableRecord(prec);
  return S_db_noMemory;
}
}

/********** LONGOUT **************/

static long init_longout(longoutRecord *prec)
{
  return init_record((dbCommon*)prec, &prec->out);
}

static long write_longout(longoutRecord *prec)
{
try{
  Output* out=static_cast<Output*>(prec->dpvt);

  out->setSource(prec->val);

  return 0;

} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}


/************* LONGIN ****************/

static long init_longin(longinRecord *plongin)
{
  return init_record((dbCommon*)plongin, &plongin->inp);
}

static long read_longin(longinRecord *prec)
{
try{
  Output* out=static_cast<Output*>(prec->dpvt);

  prec->val=out->source();

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

/***************** DSET ********************/
extern "C" {

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  read;
} devLIOutput = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_longin,
  NULL,
  (DEVSUPFUN) read_longin
};
epicsExportAddress(dset,devLIOutput);

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  write;
} devLOOutput = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_longout,
  NULL,
  (DEVSUPFUN) write_longout
};
epicsExportAddress(dset,devLOOutput);

};
