
#include <stdlib.h>
#include <math.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>

#include <aiRecord.h>
#include <aoRecord.h>
#include <menuConvert.h>

#include "cardmap.h"
#include "evr/pulser.h"
#include "linkoptions.h"
#include "dsetshared.h"

#include <stdexcept>
#include <string>
#include <cfloat>

struct addr
{
  epicsUInt32 card;
  epicsUInt32 pulser;
  char prop[20];
};

static const
linkOptionDef eventdef[] = 
{
  linkInt32   (addr, card , "C", 1, 0),
  linkInt32   (addr, pulser,"I", 1, 0),
  linkString  (addr, prop , "P", 1, 0),
  linkOptionEnd
};

static const
prop_entry<Pulser,double> props_double[] = {
  {"Delay", property<Pulser,double>(0, &Pulser::delay, &Pulser::setDelay)},
  {"Width", property<Pulser,double>(0, &Pulser::width, &Pulser::setWidth)},
  {NULL,property<Pulser,double>()}
};

static const
prop_entry<Pulser,bool> props_bool[] = {
  {"Enable",   property<Pulser,bool>(0, &Pulser::enabled, &Pulser::enable)},
  {"Polarity", property<Pulser,bool>(0, &Pulser::polarityInvert, &Pulser::setPolarityInvert)},
  {NULL,property<Pulser,bool>()}
};

static const
prop_entry<Pulser,epicsUInt32> props_epicsUInt32[] = {
  {"Delay", property<Pulser,epicsUInt32>(0, &Pulser::delayRaw, &Pulser::setDelayRaw)},
  {"Width", property<Pulser,epicsUInt32>(0, &Pulser::widthRaw, &Pulser::setWidthRaw)},
  {"Prescaler", property<Pulser,epicsUInt32>(0, &Pulser::prescaler, &Pulser::setPrescaler)},
  {NULL,property<Pulser,epicsUInt32>()}
};

static
Pulser* get_pulser(const char* hwlink, std::string& propname)
{
  addr inst_addr;

  if (linkOptionsStore(eventdef, &inst_addr, hwlink, 0))
    throw std::runtime_error("Couldn't parse link string");

  EVR* card=getEVR<EVR>(inst_addr.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Pulser* pul=card->pulser(inst_addr.pulser);
  if(!pul)
    throw std::runtime_error("Failed to lookup pulser");

  propname=std::string(inst_addr.prop);

  return pul;
}

/*************** boiler plate *******************/

extern "C" {

PROPERTY_DSET_LONGIN (Pulser, get_pulser, props_epicsUInt32);
PROPERTY_DSET_LONGOUT(Pulser, get_pulser, props_epicsUInt32);

PROPERTY_DSET_BI(Pulser, get_pulser, props_bool);
PROPERTY_DSET_BO(Pulser, get_pulser, props_bool);

PROPERTY_DSET_AI(Pulser, get_pulser, props_double);
PROPERTY_DSET_AO(Pulser, get_pulser, props_double);

}
