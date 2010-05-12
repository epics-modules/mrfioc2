
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
#include "linkoptions.h"
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

struct addr
{
  epicsUInt32 card;
  OutputType otype;
  epicsUInt32 output;
  char prop[20];
};

static const
linkOptionEnumType typeEnum[] = {
  {"Int",      OutputInt},
  {"Front",    OutputFP},
  {"FrontUniv",OutputFPUniv},
  {"RearUniv", OutputRB},
  {NULL,0}
};

static const
linkOptionDef eventdef[] = 
{
  linkInt32   (addr, card ,  "C"  , 1, 0),
  linkEnum    (addr, otype,  "T"  , 1, 0, typeEnum),
  linkInt32   (addr, output ,"I", 1, 0),
  linkString  (addr, prop ,  "P"  , 1, 0),
  linkOptionEnd
};

static const
prop_entry<Output,epicsUInt32> props_epicsUInt32[] = {
  {"Map", property<Output,epicsUInt32>(0, &Output::source, &Output::setSource)},
  {NULL,property<Output,epicsUInt32>()}
};

static
Output* get_output(const char* hwlink, std::string& propname)
{
  addr inst_addr;

  if (linkOptionsStore(eventdef, &inst_addr, hwlink, 0))
    throw std::runtime_error("Couldn't parse link string");

  EVR* card=getEVR<EVR>(inst_addr.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Output* output=card->output(inst_addr.otype, inst_addr.output);

  if(!output)
    throw std::runtime_error("Output Type is out of range");

  propname=std::string(inst_addr.prop);

  return output;
}


/***************** DSET ********************/
extern "C" {

PROPERTY_DSET_LONGIN (Output, get_output, props_epicsUInt32);
PROPERTY_DSET_LONGOUT(Output, get_output, props_epicsUInt32);

};
