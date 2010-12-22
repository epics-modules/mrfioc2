/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <stdlib.h>
#include <string.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>
#include "linkoptions.h"
#include "dsetshared.h"

#include <stringinRecord.h>

#include <mrfCommon.h> // for mrfDisableRecord

#include "evrmap.h"
#include "evr/evr.h"

#include <stdexcept>
#include <string>

struct addr {
  EVR* evr;
  epicsUInt32 card;
  epicsUInt32 code;
  epicsUInt32 last_bad;
};

static const
linkOptionDef eventdef[] = 
{
  linkInt32   (addr, card , "C" , 1, 0),
  linkInt32   (addr, code , "Code" , 0, 0),
  linkOptionEnd
};

/***************** Stringin (Timestamp) *****************/

static
long stringin_add(dbCommon *praw)
{
  stringinRecord *prec=(stringinRecord*)(praw);
  long ret=0;
try {
  assert(prec->inp.type==INST_IO);
  addr *priv=new addr;
  priv->code=0;
  priv->last_bad=0;

  if (linkOptionsStore(eventdef, priv, prec->inp.value.instio.string, 0))
    throw std::runtime_error("Couldn't parse link string");

  priv->evr=&evrmap.get(priv->card);
  if(!priv->evr)
    throw std::runtime_error("Failed to lookup device");

  prec->dpvt=static_cast<void*>(priv);

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
  addr *priv=static_cast<addr*>(prec->dpvt);

  epicsTimeStamp ts;

  if(!priv->evr->getTimeStamp(&ts,priv->code)){
    strncpy(prec->val,"EVR time unavailable",sizeof(prec->val));
    return S_dev_deviceTMO;
  }

  if (ts.secPastEpoch==priv->last_bad)
      return 0;

  size_t r=epicsTimeToStrftime(prec->val,
                               sizeof(prec->val),
                               "%a, %d %b %Y %H:%M:%S %z",
                               &ts);
  if(r==0||r==sizeof(prec->val)){
    recGblRecordError(S_dev_badArgument, (void*)prec, 
      "Format string resulted in error");
    priv->last_bad=ts.secPastEpoch;
    return S_dev_badArgument;
  }

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

extern "C" {

dsxt dxtSIEVR={stringin_add,del_record_empty};
static
common_dset devSIEVR = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtSIEVR>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  (DEVSUPFUN) read_si
};
epicsExportAddress(dset,devSIEVR);


};
