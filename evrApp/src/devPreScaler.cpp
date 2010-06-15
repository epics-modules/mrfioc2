
#include <cstdlib>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>
#include <menuConvert.h>

#include <aoRecord.h>
#include <longinRecord.h>
#include <longoutRecord.h>

#include <mrfCommon.h> // for mrfDisableRecord

#include "cardmap.h"
#include "evr/evr.h"
#include "evr/prescaler.h"
#include "linkoptions.h"
#include "dsetshared.h"

#include <stdexcept>

/**@file devPreScaler.cpp
 *
 * Device support for accessing PreScaler instances.
 *
 * Uses VME_IO links. '#Cxx Syy @'
 * xx - card number
 * yy - Prescaler number
 */

struct addr
{
  epicsUInt32 card;
  epicsUInt32 scaler;
  char prop[20];
};

static const
linkOptionDef eventdef[] =
{
  linkInt32   (addr, card , "C", 1, 0),
  linkInt32   (addr, scaler,"I", 1, 0),
  linkString  (addr, prop , "P", 1, 0),
  linkOptionEnd
};

static const
prop_entry<PreScaler,epicsUInt32> props_epicsUInt32[] = {
  {"Divide", property<PreScaler,epicsUInt32>(0, &PreScaler::prescaler, &PreScaler::setPrescaler)},
  {NULL,property<PreScaler,epicsUInt32>()}
};

static
PreScaler* get_prescaler(const char* hwlink, std::string& propname)
{
  addr inst_addr;

  if (linkOptionsStore(eventdef, &inst_addr, hwlink, 0))
    throw std::runtime_error("Couldn't parse link string");

  EVR* card=getEVR<EVR>(inst_addr.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  PreScaler* ps=card->prescaler(inst_addr.scaler);
  if(!ps)
    throw std::runtime_error("Prescaler id# is out of range");

  propname=std::string(inst_addr.prop);

  return ps;
}

extern "C" {

PROPERTY_DSET_LONGIN (PreScaler, get_prescaler, props_epicsUInt32);
PROPERTY_DSET_LONGOUT(PreScaler, get_prescaler, props_epicsUInt32);

}; // extern "C"
