
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*

#include <mbbiRecord.h>
#include <mbboRecord.h>

#include "cardmap.h"
#include "evr/evr.h"
#include "evr/output.h"

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
  return S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

/********** MBBO **************/

static long init_mbbo(mbboRecord *prec)
{
  return init_record((dbCommon*)prec, &prec->out);
}

static long write_mbbo(mbboRecord *prec)
{
try{
  Output* out=static_cast<Output*>(prec->dpvt);

  out->setSource(prec->rval);

  return 0;

} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}


/************* MBBI ****************/

static long init_mbbi(mbbiRecord *pmbbi)
{
  return init_record((dbCommon*)pmbbi, &pmbbi->inp);
}

static long read_mbbi(mbbiRecord *prec)
{
try{
  Output* out=static_cast<Output*>(prec->dpvt);

  prec->rval=out->source();

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
} devMBBIOutput = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_mbbi,
  NULL,
  (DEVSUPFUN) read_mbbi
};
epicsExportAddress(dset,devMBBIOutput);

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  write;
} devMBBOOutput = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_mbbo,
  NULL,
  (DEVSUPFUN) write_mbbo
};
epicsExportAddress(dset,devMBBOOutput);

};
