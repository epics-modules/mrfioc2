
#include "cardmap.h"

#include <stdexcept>
#include <map>

typedef std::map<short,EVR*> devices_t;

static
devices_t devices;

void
storeEVRBase(short id, EVR* dev)
{
  if(!dev)
    throw std::runtime_error("storeEVR: Can not store NULL."
      "  (initialization probably failed)");

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
