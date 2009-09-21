
#include "evr/evr.h"
#include "evr/pulser.h"
#include "evr/output.h"
#include "evr/prescaler.h"

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
  case MapType::Reset:
    name+=" sets the output to ";
    name+=polarityNorm() ? "High" : "Low";
    return name;
  case MapType::Set:
    name+=" resets the output to ";
    name+=polarityNorm() ? "Low" : "High";
    return name;
  }

  return std::string("Invalid action selected!");
}
