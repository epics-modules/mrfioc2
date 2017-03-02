/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2017.
 *
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <errlog.h>
#include <generalTimeSup.h>

#include "mrfCommon.h"
#include "mrfCommonIO.h"
#include "evrFRIBRegMap.h"

#include "evr_frib.h"


/*
 * FRIB EVR
 *
 * A sub-set of the functionality of the MRF EVR.
 * Has mapping ram with some functions.
 * Fixed mapping between mapping ram function, prescalar, and HW outputs
 *
 * output 1 -> clock divider  (CLK)
 * output 2 -> pulser 0       (TR0)
 * output 3 -> pulser 1       (TR1)
 */

EVRFRIB::EVRFRIB(const std::string& s,
                 bus_configuration &busConfig,
                 volatile unsigned char *base)
    :base_t(s, busConfig)
    ,base(base)
    ,clockFreq(80.5)
    ,timeoffset(POSIX_TIME_AT_EPICS_EPOCH)
    ,divider(SB()<<s<<":DIV", *this)
    ,pulse0(SB()<<s<<":PULSE"<<0, 0, this)
    ,pulse1(SB()<<s<<":PULSE"<<1, 1, this)
    ,out_divider(SB()<<s<<":DIV:OUT", 1, this)
    ,out_pulse0(SB()<<s<<":PULSE"<<0<<":OUT", 2, this)
    ,out_pulse1(SB()<<s<<":PULSE"<<1<<":OUT", 3, this)
{
    epicsUInt32 info = READ32(base, FWInfo);

    switch((info&FWInfo_Flavor_mask)>>FWInfo_Flavor_shift) {
    case FWInfo_Flavor_EVG:
    {
        epicsTimeStamp now;
        if(epicsTimeOK == generalTimeGetExceptPriority(&now, 0, 50)) {
            // start simulated time from the present
            timeoffset = POSIX_TIME_AT_EPICS_EPOCH + now.secPastEpoch - READ32(base, TimeSec);
        }
        break;
    }
    }

    // disable FIFO
    WRITE32(base, FIFOEna, 0);

    // disable simulation modes
    WRITE32(base, Config, 0);

    // disable w/ divider==0 (/1)
    WRITE32(base, OutSelect, 0);

    // Partial reset and force FPS trip
    WRITE32(base, Command, Command_ResetEVR|Command_NOKForce);

    WRITE32(base, Command, 0);

    for(unsigned i=1; i<=255; i++) {
        // initially, disable all mappings and unmap from FIFO
        WRITE32(base, EvtConfig(i), EvtConfig_FIFOUnMap);
    }

    scanIoInit(&statusScan);
}

EVRFRIB::~EVRFRIB() {}


void EVRFRIB::lock() const
{
    mutex.lock();
}

void EVRFRIB::unlock() const
{
    mutex.unlock();
}

std::string EVRFRIB::model() const
{
    switch((READ32(base, FWInfo)&FWInfo_Version_mask)>>FWInfo_Version_shift)
    {
    case FWInfo_Flavor_EVR: return "EVR";
    case FWInfo_Flavor_EVG: return "EVG";
    default: return "???";
    }
}

epicsUInt32 EVRFRIB::version() const {
    return (READ32(base, FWInfo)&FWInfo_Version_mask)>>FWInfo_Version_shift;
}

double EVRFRIB::clock() const {return clockFreq;}
void EVRFRIB::clockSet(double clk) {
    clockFreq = clk;
}

double EVRFRIB::clockTS() const { return clockFreq; }
void EVRFRIB::clockTSSet(double) {}

bool EVRFRIB::interestedInEvent(epicsUInt32 event,bool set) {
    //TODO
    return false;
}

bool EVRFRIB::TimeStampValid() const
{
    return linkStatus();
}

bool EVRFRIB::getTimeStamp(epicsTimeStamp *ts,epicsUInt32 event)
{
    bool ret = event==epicsTimeEventCurrentTime;

    if(ret)
        ret = linkStatus();

    if(ret && ts) {
        double period=1e9/clockTS();

        if(period<=0 || !isfinite(period))
            return false;

        epicsUInt32 sec = READ32(base, TimeSec);
        epicsUInt32 ns  = READ32(base, TimeFrac)*period;

        if(ns>=1000000000) {
            return false;
        }

        ts->secPastEpoch = sec - timeoffset;
        ts->nsec = ns;
    }

    return ret;
}

bool EVRFRIB::getTicks(epicsUInt32 *tks) {
    *tks = READ32(base, TimeFrac);
    return true;
}

bool EVRFRIB::linkStatus() const {
    return READ32(base, Status) & Status_Alive;
}

OBJECT_BEGIN2(EVRFRIB, EVR)
OBJECT_END(EVRFRIB)


PulserFRIB::PulserFRIB(const std::string& s, unsigned n, EVRFRIB *evr)
    :Pulser(s)
    ,n(n)
    ,evr(evr)
{

}

PulserFRIB::~PulserFRIB() {}

void PulserFRIB::setDelayRaw(epicsUInt32 v)
{
    WRITE32(evr->base, PulserDelay(n), v);
}

void PulserFRIB::setDelay(double v)
{
    const double clock = evr->clock();

    epicsUInt32 ticks=roundToUInt(v*clock);

    setDelayRaw(ticks);
}

epicsUInt32 PulserFRIB::delayRaw() const
{
    return READ32(evr->base, PulserDelay(n));
}

double PulserFRIB::delay() const
{
    double ticks=double(delayRaw());
    double clk=evr->clock(); // in MHz.  MTicks/second

    return ticks/clk;
}


void PulserFRIB::setWidthRaw(epicsUInt32 v)
{
    WRITE32(evr->base, PulserWidth(n), v);
}

void PulserFRIB::setWidth(double v)
{
    const double clock = evr->clock();

    epicsUInt32 ticks=roundToUInt(v*clock);

    setWidthRaw(ticks);
}

epicsUInt32 PulserFRIB::widthRaw() const
{
    return READ32(evr->base, PulserWidth(n));
}

double PulserFRIB::width() const
{
    double ticks=double(widthRaw());
    double clk=evr->clock(); // in MHz.  MTicks/second

    return ticks/clk;
}

MapType::type PulserFRIB::mappedSource(epicsUInt32 evt) const
{
    if(evt>255)
        throw std::out_of_range("Event code is out of range");

    if(evt==0)
        return MapType::None;

    epicsUInt32 map = READ32(evr->base, EvtConfig(evt));

    bool pulses = map&EvtConfig_Pulse(n);
    bool sets   = map&EvtConfig_Set(n);
    bool clears = map&EvtConfig_Clear(n);

    MapType::type ret = MapType::None;

    unsigned c=0;
    if(pulses) {c++; ret = MapType::Trigger; }
    if(sets)   {c++; ret = MapType::Set; }
    if(clears) {c++; ret = MapType::Reset; }

    if(c>1) {
        errlogPrintf("EVR %s pulser #%d code %02x maps too many actions %08x\n",
            evr->name().c_str(), n, evt, map);
    }

    return ret;
}

void PulserFRIB::sourceSetMap(epicsUInt32 evt, MapType::type action)
{
    if(evt>255)
        throw std::out_of_range("Event code is out of range");

    if(evt==0)
        return;

    const epicsUInt32 mask = EvtConfig_Pulse(n)|EvtConfig_Set(n)|EvtConfig_Clear(n);

    epicsUInt32 map = READ32(evr->base, EvtConfig(evt));

    map &= ~mask;

    switch(action) {
    case MapType::None: break;
    case MapType::Trigger: map |= EvtConfig_Pulse(n); break;
    case MapType::Set: map |= EvtConfig_Set(n); break;
    case MapType::Reset: map |= EvtConfig_Clear(n); break;
    }

    WRITE32(evr->base, EvtConfig(evt), map);
}

void PulserFRIB::lock() const { evr->lock(); }
void PulserFRIB::unlock() const { evr->unlock(); }


PreScalerFRIB::PreScalerFRIB(const std::string& n, EVRFRIB& o)
    :PreScaler(n, o)
{}

PreScalerFRIB::~PreScalerFRIB() {}

epicsUInt32 PreScalerFRIB::prescaler() const
{
    EVRFRIB& evr = static_cast<EVRFRIB&>(owner);
    return (READ32(evr.base, OutSelect)&OutSelect_Divider_mask)>>OutSelect_Divider_shift;
}

void PreScalerFRIB::setPrescaler(epicsUInt32 v)
{
    EVRFRIB& evr = static_cast<EVRFRIB&>(owner);
    epicsUInt32 T = READ32(evr.base, OutSelect);
    T &= ~OutSelect_Divider_mask;
    T |= (v<<OutSelect_Divider_shift)&OutSelect_Divider_mask;
    WRITE32(evr.base, OutSelect, T);
}

void PreScalerFRIB::lock() const { return owner.lock(); }
void PreScalerFRIB::unlock() const { return owner.unlock(); }


OutputFRIB::OutputFRIB(const std::string& n, epicsUInt32 src, EVRFRIB *evr) : Output(n), src(src), evr(evr) {}
OutputFRIB::~OutputFRIB() {}

bool OutputFRIB::enabled() const
{
    switch(src) {
    case 1:
        return READ32(evr->base, OutSelect)&OutSelect_Enable;
    case 2:
    case 3:
        return true;
    default:
        return false;
    }
}

void OutputFRIB::enable(bool v)
{
    switch(src) {
    case 1:
        if(v)
            BITSET32(evr->base, OutSelect, OutSelect_Enable);
        else
            BITCLR32(evr->base, OutSelect, OutSelect_Enable);
        return;

    default:
        break;
    }
}

const char* OutputFRIB::sourceName(epicsUInt32 s) const
{
    switch(s) {
    case 1: return "CLK";
    case 2: return "TR0";
    case 3: return "TR1";
    default: return "???";
    }
}

void OutputFRIB::lock() const { evr->lock(); }
void OutputFRIB::unlock() const { evr->unlock(); }
