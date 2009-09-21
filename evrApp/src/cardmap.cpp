
#include "cardmap.h"

#include <stdexcept>
#include <map>

typedef std::map<short,EVR*> devices_t;

static
devices_t devices;

void
storeEVR(short id, EVR* dev)
{
  devices_t::const_iterator it=devices.find(id);

  if(it!=devices.end())
    throw std::runtime_error("storeEVR: identifier already used");

  devices[id]=dev;
}

EVR*
getEVRBase(short id)
{
  devices_t::const_iterator it=devices.find(id);

  if(it==devices.end())
    return 0;

  return it->second;
}
