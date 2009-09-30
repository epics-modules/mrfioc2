
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*

#include <longinRecord.h>
#include <longoutRecord.h>
#include <biRecord.h>
#include <boRecord.h>

#include "cardmap.h"
#include "evr/evr.h"
#include "property.h"

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

struct map_priv {
  EVR* card;
  epicsUInt32 last_code;
  epicsUInt32 last_func;
};

static long init_lo(longoutRecord *plo)
{
try {
  map_priv *priv=new map_priv;
  priv->last_code=0;
  priv->last_func=plo->out.value.vmeio.signal;

  priv->card=getEVR<EVR>(plo->out.value.vmeio.card);
  if(!priv->card)
    throw std::runtime_error("Failed to lookup device");

  plo->dpvt=static_cast<void*>(priv);

  return 0;

} catch(std::runtime_error& e) {
  recGblRecordError(S_dev_noDevice, (void*)plo, e.what());
  return S_dev_noDevice;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)plo, e.what());
  return S_db_noMemory;
}
}

static long write_lo(longoutRecord* plo)
{
try {
  map_priv* priv=static_cast<map_priv*>(plo->dpvt);
  
  epicsUInt32 func=plo->out.value.vmeio.signal;
  
  epicsUInt32 code=plo->val;

  if( func==priv->last_func && code==priv->last_code )
    return 0;

  priv->card->specialSetMap(priv->last_code,priv->last_func,false);

  priv->card->specialSetMap(code,func,true);

  priv->last_code=code;
  priv->last_func=func;
  
  return 0;
  
} catch(std::runtime_error& e) {
  plo->val=0;
  recGblRecordError(S_dev_noDevice, (void*)plo, e.what());
  return S_dev_noDevice;
} catch(std::exception& e) {
  plo->val=0;
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
  DEVSUPFUN  write;
} devLOEVRMap = {
  5,
  NULL,
  NULL,
  (DEVSUPFUN) init_lo,
  NULL,
  (DEVSUPFUN) write_lo
};
epicsExportAddress(dset,devLOEVRMap);

};
