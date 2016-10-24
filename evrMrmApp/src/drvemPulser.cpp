/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include "evrRegMap.h"
#include "drvem.h"

#include <stdexcept>
#include <cstring>

#include <errlog.h>
#include <dbDefs.h>
#include <epicsMath.h>

#include "mrfCommonIO.h"
#include "mrfBitOps.h"



#include "drvemPulser.h"

MRMPulser::MRMPulser(const std::string& n, epicsUInt32 i, EVRMRM& o)
  :Pulser(n)
  ,id(i)
  ,owner(o)
{
    if(id>31)
        throw std::out_of_range("pulser id is out of range");

    std::memset(&this->mapped, 0, NELEMENTS(this->mapped));
}

void MRMPulser::lock() const{owner.lock();};
void MRMPulser::unlock() const{owner.unlock();};

bool
MRMPulser::enabled() const
{
    return READ32(owner.base, PulserCtrl(id)) & PulserCtrl_ena;
}

void
MRMPulser::enable(bool s)
{
    if(s)
        BITSET(NAT,32,owner.base, PulserCtrl(id),
             PulserCtrl_ena|PulserCtrl_mtrg|PulserCtrl_mset|PulserCtrl_mrst);
    else
        BITCLR(NAT,32,owner.base, PulserCtrl(id),
             PulserCtrl_ena|PulserCtrl_mtrg|PulserCtrl_mset|PulserCtrl_mrst);
}

void
MRMPulser::setDelayRaw(epicsUInt32 v)
{
    WRITE32(owner.base, PulserDely(id), v);
}

void
MRMPulser::setDelay(double v)
{
    double scal=double(prescaler());
    if (scal<=0) scal=1;
    double clk=owner.clock(); // in MHz.  MTicks/second

    epicsUInt32 ticks=roundToUInt((v*clk)/scal);

    setDelayRaw(ticks);
}

epicsUInt32
MRMPulser::delayRaw() const
{
    return READ32(owner.base,PulserDely(id));
}

double
MRMPulser::delay() const
{
    double scal=double(prescaler());
    double ticks=double(delayRaw());
    double clk=owner.clock(); // in MHz.  MTicks/second
    if (scal<=0) scal=1;

    return (ticks*scal)/clk;
}

void
MRMPulser::setWidthRaw(epicsUInt32 v)
{
    WRITE32(owner.base, PulserWdth(id), v);
}

void
MRMPulser::setWidth(double v)
{
    double scal=double(prescaler());
    double clk=owner.clock(); // in MHz.  MTicks/second
    if (scal<=0) scal=1;

    epicsUInt32 ticks=roundToUInt((v*clk)/scal);

    setWidthRaw(ticks);
}

epicsUInt32
MRMPulser::widthRaw() const
{
    return READ32(owner.base,PulserWdth(id));
}

double
MRMPulser::width() const
{
    double scal=double(prescaler());
    double ticks=double(widthRaw());
    double clk=owner.clock(); // in MHz.  MTicks/second
    if (scal<=0) scal=1;

    return (ticks*scal)/clk;
}

epicsUInt32
MRMPulser::prescaler() const
{
    return READ32(owner.base,PulserScal(id));
}

void
MRMPulser::setPrescaler(epicsUInt32 v)
{
    WRITE32(owner.base, PulserScal(id), v);
}

bool
MRMPulser::polarityInvert() const
{
    return (READ32(owner.base, PulserCtrl(id)) & PulserCtrl_pol) != 0;
}

void
MRMPulser::setPolarityInvert(bool s)
{
    if(s)
        BITSET(NAT,32,owner.base, PulserCtrl(id), PulserCtrl_pol);
    else
        BITCLR(NAT,32,owner.base, PulserCtrl(id), PulserCtrl_pol);
}

MapType::type
MRMPulser::mappedSource(epicsUInt32 evt) const
{
    if(evt>255)
        throw std::out_of_range("Event code is out of range");

    if(evt==0)
        return MapType::None;

    epicsUInt32 map[3];

    map[0]=READ32(owner.base, MappingRam(0,evt,Trigger));
    map[1]=READ32(owner.base, MappingRam(0,evt,Set));
    map[2]=READ32(owner.base, MappingRam(0,evt,Reset));

    epicsUInt32 pmask=1<<id, insanity=0;

    MapType::type ret=MapType::None;

    if(map[0]&pmask){
        ret=MapType::Trigger;
        insanity++;
    }
    if(map[1]&pmask){
        ret=MapType::Set;
        insanity++;
    }
    if(map[2]&pmask){
        ret=MapType::Reset;
        insanity++;
    }
    if(insanity>1){
        errlogPrintf("EVR %s pulser #%d code %02x maps too many actions %08x %08x %08x\n",
            owner.name().c_str(),id,evt,map[0],map[1],map[2]);
    }

    if( (ret==MapType::None) ^ _ismap(evt) ){
        errlogPrintf("EVR %s pulser #%d code %02x mapping (%08x %08x %08x) is out of sync with view (%d)\n",
            owner.name().c_str(),id,evt,map[0],map[1],map[2],_ismap(evt));
    }

    return ret;
}

void
MRMPulser::sourceSetMap(epicsUInt32 evt,MapType::type action)
{
    if(evt>255)
        throw std::out_of_range("Event code is out of range");

    if(evt==0)
        return;

    epicsUInt32 pmask=1<<id;

    if( (action!=MapType::None) && _ismap(evt) )
        throw std::runtime_error("Ignore request for duplicate mapping");

    if(action!=MapType::None)
        _map(evt);
    else
        _unmap(evt);

    if(action==MapType::Trigger)
        BITSET(NAT,32, owner.base, MappingRam(0,evt,Trigger), pmask);
    else
        BITCLR(NAT,32, owner.base, MappingRam(0,evt,Trigger), pmask);

    if(action==MapType::Set)
        BITSET(NAT,32, owner.base, MappingRam(0,evt,Set), pmask);
    else
        BITCLR(NAT,32, owner.base, MappingRam(0,evt,Set), pmask);

    if(action==MapType::Reset)
        BITSET(NAT,32, owner.base, MappingRam(0,evt,Reset), pmask);
    else
        BITCLR(NAT,32, owner.base, MappingRam(0,evt,Reset), pmask);
}
