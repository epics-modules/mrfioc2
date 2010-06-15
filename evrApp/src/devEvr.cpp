
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>

#include <longinRecord.h>
#include <longoutRecord.h>

#include "cardmap.h"
#include "evr/evr.h"
#include "dsetshared.h"
#include "linkoptions.h"

#include <stdexcept>
#include <string>

struct addr {
  epicsUInt32 card;
  char prop[20];
};

static const
linkOptionDef eventdef[] = 
{
  linkInt32   (addr, card , "C" , 1, 0),
  linkString  (addr, prop , "P"  , 1, 0),
  linkOptionEnd
};

static const
prop_entry<EVR,double> props_double[] = {
  {"Clock",           property<EVR,double>(0, &EVR::clock, &EVR::clockSet)},
  {"Timestamp Clock", property<EVR,double>(0, &EVR::clockTS, &EVR::clockTSSet)},
  {NULL,              property<EVR,double>()}
};

static const
prop_entry<EVR,bool> props_bool[] = {
  {"Enable",          property<EVR,bool>(0, &EVR::enabled, &EVR::enable)},
  {"PLL Lock Status", property<EVR,bool>(0, &EVR::pllLocked)},
  {"Link Status",     property<EVR,bool>(0, &EVR::linkStatus, 0, &EVR::linkChanged)},
  {NULL,              property<EVR,bool>()}
};

static const
prop_entry<EVR,epicsUInt32> props_epicsUInt32[] = {
  {"Model",               property<EVR,epicsUInt32>(0, &EVR::model)},
  {"Version",             property<EVR,epicsUInt32>(0, &EVR::version)},
  {"Event Clock TS Div",  property<EVR,epicsUInt32>(0, &EVR::uSecDiv)},
  {"Receive Error Count", property<EVR,epicsUInt32>(0, &EVR::recvErrorCount, 0, &EVR::linkChanged)},
  {"Timestamp Prescaler", property<EVR,epicsUInt32>(0, &EVR::tsDiv)},
  {"Timestamp Source",    property<EVR,epicsUInt32>(0, &EVR::SourceTSraw, &EVR::setSourceTSraw)},
  {NULL,                  property<EVR,epicsUInt32>()}
};

static
EVR* get_evr(const char* hwlink, std::string& propname)
{
  addr inst_addr;

  if (linkOptionsStore(eventdef, &inst_addr, hwlink, 0))
    throw std::runtime_error("Couldn't parse link string");

  EVR* card=getEVR<EVR>(inst_addr.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  propname=std::string(inst_addr.prop);

  return card;
}

/************* Boiler plate *****************/

extern "C" {

PROPERTY_DSET_LONGIN (EVR, get_evr, props_epicsUInt32);
PROPERTY_DSET_LONGOUT(EVR, get_evr, props_epicsUInt32);

PROPERTY_DSET_BI(EVR, get_evr, props_bool);
PROPERTY_DSET_BO(EVR, get_evr, props_bool);

PROPERTY_DSET_AI(EVR, get_evr, props_double);
PROPERTY_DSET_AO(EVR, get_evr, props_double);

}
