
#include <stdlib.h>
#include <epicsExport.h>
#include <dbAccess.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*

#include <biRecord.h>
#include <boRecord.h>
#include <mbboRecord.h>

#include "evrmap.h"
#include "evr/evr.h"
#include "evr/input.h"
#include "linkoptions.h"
#include "dsetshared.h"

#include <stdexcept>
#include <string>

struct addr
{
  epicsUInt32 card;
  epicsUInt32 input;
  char prop[20];
};

static const
linkOptionDef eventdef[] = 
{
  linkInt32   (addr, card , "C",  1, 0),
  linkInt32   (addr, input, "I", 1, 0),
  linkString  (addr, prop , "P"  , 1, 0),
  linkOptionEnd
};

static const
prop_entry<Input,bool> props_bool[] = {
  {"Active Level",property<Input,bool>(0, &Input::levelHigh, &Input::levelHighSet)},
  {"Active Edge", property<Input,bool>(0, &Input::edgeRise, &Input::edgeRiseSet)},
  {NULL,          property<Input,bool>()}
};

static const
prop_entry<Input,epicsUInt32> props_epicsUInt32[] = {
  {"External Code", property<Input,epicsUInt32>(0, &Input::extEvt, &Input::extEvtSet)},
  {"Backwards Code",property<Input,epicsUInt32>(0, &Input::backEvt, &Input::backEvtSet)},
  {NULL,            property<Input,epicsUInt32>()}
};

static const
prop_entry<Input,epicsUInt16> props_epicsUInt16[] = {
  {"External Mode", property<Input,epicsUInt16>(0, &Input::extModeraw, &Input::extModeSetraw)},
  {"Backwards Mode",property<Input,epicsUInt16>(0, &Input::backModeraw, &Input::backModeSetraw)},
  {"DBus Mask",     property<Input,epicsUInt16>(0, &Input::dbus, &Input::dbusSet)},
  {NULL,            property<Input,epicsUInt16>()}
};

static
Input* get_input(const char* hwlink, std::string& propname)
{
  addr inst_addr;

  if (linkOptionsStore(eventdef, &inst_addr, hwlink, 0))
    throw std::runtime_error("Couldn't parse link string");

  EVR* card=&evrmap.get(inst_addr.card);
  if(!card)
    throw std::runtime_error("Failed to lookup device");

  Input* inp=card->input(inst_addr.input);

  if(!inp)
    throw std::runtime_error("Input Type is out of range");

  propname=std::string(inst_addr.prop);

  return inp;
}

static long write_mbbo(mbboRecord* prec)
{
  if (!prec->dpvt) return -1;
try {
  property<Input,epicsUInt16> *priv=static_cast<property<Input,epicsUInt16>*>(prec->dpvt);

  switch(prec->rval){
  case TrigNone:
  case TrigLevel:
  case TrigEdge:
    priv->set((epicsUInt16)prec->rval);

    prec->rbv=priv->get();
    break;
  default:
    throw std::out_of_range("Invalid Input trigger mode");
  }

  return 0;
} catch(std::exception& e) {
  recGblRecordError(S_db_noMemory, (void*)prec, e.what());
  return S_db_noMemory;
}
}

/*************** boilerplate *****************/

extern "C" {

PROPERTY_DSET_LONGIN (Input, get_input, props_epicsUInt32);
PROPERTY_DSET_LONGOUT(Input, get_input, props_epicsUInt32);

PROPERTY_DSET_BI(Input, get_input, props_bool);
PROPERTY_DSET_BO(Input, get_input, props_bool);

// impliments add_Input_mbbo
PROPERTY_DSET_ADDFN(mbbo, epicsUInt16, out, Input, get_input, props_epicsUInt16);
PROPERTY_DSET_TABLE(mbbo, epicsUInt16, Input,
                    add_Input_mbbo,
                    init_record_return2,
                    write_mbbo) 

}
