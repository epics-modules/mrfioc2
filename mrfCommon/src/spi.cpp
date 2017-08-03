/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <map>

#include "mrfCommon.h"
#include "mrf/spi.h"

namespace mrf {

SPIInterface::SPIInterface()
    :optimo(0.1)
{}

SPIInterface::~SPIInterface() {}

void
SPIInterface::select(unsigned id)
{}

void
SPIInterface::cycles(size_t nops,
                     const Operation *ops)
{
    for(size_t n=0; n<nops; n++)
    {
        const Operation& op = ops[n];

        for(size_t i=0; i<op.ncycles; i++) {
            epicsUInt8 O = this->cycle(op.in ? op.in[i] : 0);
            if(op.out)
                op.out[i] = O;
        }
    }
}

double
SPIInterface::timeout() const
{
    SCOPED_LOCK(mutex);
    return optimo;
}

void
SPIInterface::setTimeout(double t)
{
    SCOPED_LOCK(mutex);
    optimo = t;
}

namespace {
struct SPIRegistry {
    epicsMutex mutex;
    typedef std::map<std::string, SPIDevice> devices_t;
    devices_t devices;
};
SPIRegistry * volatile registry;

SPIRegistry *getReg()
{
    SPIRegistry *ret = registry;
    if(!ret) {
        ret = new SPIRegistry;
        registry = ret;
    }
    return ret;
}

} //namespace

bool SPIDevice::lookupDev(const std::string& name, SPIDevice *dev)
{
    SPIRegistry *reg(getReg());
    SCOPED_LOCK2(reg->mutex,G);
    SPIRegistry::devices_t::const_iterator it = reg->devices.find(name);
    if(it!=reg->devices.end()) {
        *dev = it->second;
        return true;
    }
    return false;
}

void SPIDevice::registerDev(const std::string& name, const SPIDevice &dev)
{
    SPIRegistry *reg(getReg());
    SCOPED_LOCK2(reg->mutex,G);
    reg->devices[name] = dev;
}

void SPIDevice::unregisterDev(const std::string& name)
{
    SPIRegistry *reg(getReg());
    SCOPED_LOCK2(reg->mutex,G);
    reg->devices.erase(name);
}

} // namespace mrf
