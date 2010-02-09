
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
#include "evr/pulser.h"
#include "property.h"

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
  Pulser* pulser;
  epicsUInt32 last_code;
  MapType::type last_func;
};

static long init_lo(longoutRecord *plo)
{
  long ret=0;
try {
  assert(plo->out.type==AB_IO);

  map_priv *priv=new map_priv;
  priv->last_code=0;
  priv->last_func=MapType::None;

  EVR *card=getEVR<EVR>(plo->out.value.abio.link);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  priv->pulser=card->pulser(plo->out.value.abio.adapter);
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
try {
  map_priv* priv=static_cast<map_priv*>(plo->dpvt);

  epicsUInt32 code=plo->val;
  MapType::type func;

  switch(plo->out.value.abio.signal){
  case MapType::None:
  case MapType::Trigger:
  case MapType::Set:
  case MapType::Reset:
    func=(MapType::type)plo->out.value.abio.signal;
    break;
  default:
    throw std::runtime_error("Invalid mapping type");
  }

  if( func==priv->last_func && code==priv->last_code )
    return 0;

  //TODO: sanity check to catch overloaded mappings

  if(code!=priv->last_code)
    priv->pulser->sourceSetMap(priv->last_code,MapType::None);

  if(code==0)
    return 0;

  priv->pulser->sourceSetMap(code,func);

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

struct {
  long num;
  DEVSUPFUN  report;
  DEVSUPFUN  init;
  DEVSUPFUN  init_record;
  DEVSUPFUN  get_ioint_info;
  DEVSUPFUN  write;
} devLOEVRPulserMap = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_lo,
  NULL,
  (DEVSUPFUN) write_lo
};
epicsExportAddress(dset,devLOEVRPulserMap);

};
