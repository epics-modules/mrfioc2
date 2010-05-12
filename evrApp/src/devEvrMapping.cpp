
#include <cstdlib>
#include <cstring>

#include <epicsExport.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*

#include <longoutRecord.h>
#include <stringinRecord.h>

#include <mrfCommon.h> // for mrfDisableRecord

#include "cardmap.h"
#include "evr/evr.h"
#include "linkoptions.h"
#include "dsetshared.h"

#include <stdexcept>
#include <string>

/**@file devEvrMapping.cpp
 *
 * A special device support to handle arbitrary mappings in the EVR
 * mapping ram.  This is intended to be used with the special codes
 * only (ie heartbeat, or prescaler reset), but can be used to map
 * arbitrary functions to arbitrary event codes.
 *
 * The function code is given as the signal in the VME_IO output link
 * and the event code is given in the VAL field.
 *
 * The meaning of the function code is implimentation specific except
 * that 0 means no event and must disable a mapping.
 */

/***************** Mapping record ******************/

struct map_priv {
  epicsUInt32 card_id;
  EVR* card;
  epicsUInt32 last_code;
  epicsUInt32 last_func;
  int next_func;
};

static const
linkOptionEnumType funcEnum[] = {
  {"FIFO",     127},
  //{"Latch TS",     126}, // Not supported
  {"Blink",    125},
  {"Forward",  124},
  {"Stop Log", 123},
  {"Log",      122},
  {"Heartbeat",101},
  {"Reset PS", 100},
  {"TS reset",  99},
  {"TS tick",   98},
  {"Shift 1",   97},
  {"Shift 0",   96},
  {NULL,0}
};

static const
linkOptionDef eventdef[] = 
{
  linkInt32   (map_priv, card_id  , "C"  , 1, 0),
  linkEnum    (map_priv, next_func, "Func"  , 1, 0, funcEnum),
  linkOptionEnd
};

static long add_lo(dbCommon* praw)
{
  longoutRecord *plo=(longoutRecord*)praw;
  long ret=0;
try {
  assert(plo->out.type==INST_IO);

  map_priv *priv=getdpvt<map_priv>(plo);

  if (linkOptionsStore(eventdef, priv, plo->out.value.instio.string, 0))
    throw std::runtime_error("Couldn't parse link string");

  priv->last_code=0;
  priv->last_func=priv->next_func;

  priv->card=getEVR<EVR>(priv->card_id);
  if(!priv->card)
    throw std::runtime_error("Failed to lookup device");

  setdpvt(plo, priv);

  return 0;

} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)plo, e.what());
  ret=S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)plo, e.what());
  ret=S_db_noMemory;
}
  mrfDisableRecord((dbCommon*)plo);
  return ret;
}

static long write_lo(longoutRecord* plo)
{
try {
  map_priv* priv=static_cast<map_priv*>(plo->dpvt);

  if (!priv)
    return -2;

  epicsUInt32 func=priv->next_func;

  epicsUInt32 code=plo->val;

  if( func==priv->last_func && code==priv->last_code )
    return 0;

  priv->card->specialSetMap(priv->last_code,priv->last_func,false);

  bool restore=false;
  try {
    priv->card->specialSetMap(code,func,true);
  } catch (std::runtime_error& e){
    restore=true;
  }

  if (restore && func==priv->last_func) {
    // Can  (try) to recover previous setting unless
    // function (OUT link) changed.
    priv->card->specialSetMap(priv->last_code,priv->last_func,true);

    plo->val = priv->last_code;
    recGblSetSevr((dbCommon *)plo, WRITE_ALARM, MAJOR_ALARM);

    return -5;
  } else if (restore) {
    // Can't recover
    plo->val = 0;
    recGblSetSevr((dbCommon *)plo, WRITE_ALARM, INVALID_ALARM);
    return -5;
  }

  priv->last_code=code;
  priv->last_func=func;
  
  return 0;
  
} catch(std::exception& e) {
  plo->val=0;
  recGblRecordError(S_db_noMemory, (void*)plo, e.what());
  return S_db_noMemory;
}
}

/********************** DSETs ***********************/

extern "C" {

dsxt dxtLOEVRMap={add_lo,del_record_empty};
static
common_dset devLOEVRMap = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtLOEVRMap>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  (DEVSUPFUN) write_lo
};
epicsExportAddress(dset,devLOEVRMap);

};
