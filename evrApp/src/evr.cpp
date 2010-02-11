
#include "evr/evr.h"
#include "evr/pulser.h"
#include "evr/output.h"
#include "evr/input.h"
#include "evr/prescaler.h"
#include "evr/util.h"

#include "dbCommon.h"

/**@file evr.cpp
 *
 * Contains implimentations of the pure virtual
 * destructors of the interfaces for C++ reasons.
 * These must be present event though they are never
 * called.  If they are absent that linking will
 * fail in some cases.
 */

EVR::~EVR()
{
}

Pulser::~Pulser()
{
}

Output::~Output()
{
}

Input::~Input()
{
}

PreScaler::~PreScaler()
{
}

std::string
Pulser::mapDesc(epicsUInt32 src,MapType::type action) const
{
  std::string name(sourceName(src));
  switch(action){
  case MapType::None:
    return name+" is not mapped";
  case MapType::Trigger:
    return name+" triggers the pulser";
  case MapType::Set:
    name+=" sets the output to ";
    name+=polarityInvert() ? "Low" : "High";
    return name;
  case MapType::Reset:
    name+=" resets the output to ";
    name+=polarityInvert() ? "High" : "Low";
    return name;
  }

  return std::string("Invalid action selected!");
}

long get_ioint_info_statusChange(int dir,dbCommon* prec,IOSCANPVT* io)
{
  IOStatus* stat=static_cast<IOStatus*>(prec->dpvt);

  *io=stat->statusChange(dir);
  
  return 0;
}
