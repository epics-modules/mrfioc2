
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

#include "evrmap.h"
#include "evr/pulser.h"
#include "linkoptions.h"
#include "dsetshared.h"

#include <stdexcept>
#include <string>

/**@file devEvrPulserMapping.cpp
 *
 * A special device support to handle arbitrary mappings in the EVR
 * mapping ram to one of the pulser units.
 *
 * The pulser id is given as the signal in the AB_IO output link
 * and the event code is given in the VAL field.
 *
 * '#Lcard Apulserid Cmapram Sfunction @'
 */

/***************** Mapping record ******************/

struct map_priv {
  epicsUInt32 card_id, pulser_id;
  Pulser* pulser;
  epicsUInt32 last_code;
  MapType::type last_func;
  MapType::type next_func;
};

static const
linkOptionEnumType funcEnum[] = {
  {"Trig", MapType::Trigger},
  {"Set",  MapType::Set},
  {"Reset",MapType::Reset},
  {NULL,0}
};

static const
linkOptionDef eventdef[] = 
{
  linkInt32   (map_priv, card_id  , "C"  , 1, 0),
  linkInt32   (map_priv, pulser_id, "I", 1, 0),
  linkEnum    (map_priv, next_func, "Func"  , 1, 0, funcEnum),
  linkOptionEnd
};

static long add_lo(dbCommon* praw)
{
  long ret=0;
  longoutRecord *plo = (longoutRecord*)praw;
try {
  assert(plo->out.type==INST_IO);

  map_priv *priv=new map_priv;
  priv->last_code=0;
  priv->last_func=MapType::None;

  if (linkOptionsStore(eventdef, priv, plo->out.value.instio.string, 0))
    throw std::runtime_error("Couldn't parse link string");

  EVR *card=&evrmap.get(priv->card_id);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  priv->pulser=card->pulser(priv->pulser_id);
  if(!priv->pulser)
    throw std::runtime_error("Failed to lookup pulser unit");

  plo->dpvt=static_cast<void*>(priv);

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
  map_priv* priv=static_cast<map_priv*>(plo->dpvt);
try {

  if (!priv)
    return -2;

  epicsUInt32 code=plo->val;

  switch(priv->next_func){
  case MapType::None:
  case MapType::Trigger:
  case MapType::Set:
  case MapType::Reset:
    break;
  default:
    throw std::runtime_error("Invalid mapping type");
  }

  if( priv->next_func==priv->last_func && code==priv->last_code )
    return 0;

  //TODO: sanity check to catch overloaded mappings

  if(code!=priv->last_code)
    priv->pulser->sourceSetMap(priv->last_code,MapType::None);

  if(code!=0) {
    priv->pulser->sourceSetMap(code,priv->next_func);
  }

  priv->last_code=code;
  priv->last_func=priv->next_func;

  return 0;

} catch(std::exception& e) {
  plo->val=0;
  priv->last_code=0;
  priv->last_func=priv->next_func;
  recGblSetSevr((dbCommon *)plo, WRITE_ALARM, INVALID_ALARM);
  recGblRecordError(S_db_noMemory, (void*)plo, e.what());
  return S_db_noMemory;
}
}

/********************** DSETs ***********************/

extern "C" {

dsxt dxtLOEVRPulserMap={add_lo,del_record_empty};
static
common_dset devLOEVRPulserMap = {
  5,
  NULL,
  dset_cast(&init_dset<&dxtLOEVRPulserMap>),
  (DEVSUPFUN) init_record_empty,
  NULL,
  (DEVSUPFUN) write_lo
};
epicsExportAddress(dset,devLOEVRPulserMap);

};
