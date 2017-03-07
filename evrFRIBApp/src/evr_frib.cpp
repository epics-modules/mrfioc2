/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2017.
 *
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <stdio.h>

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
    ,is_evg(false)
    ,internal_clk(false)
    ,timeoffset(POSIX_TIME_AT_EPICS_EPOCH)
    ,divider(SB()<<s<<":PS0", *this)
    ,pulse0(SB()<<s<<":Pul"<<0, 0, this)
    ,pulse1(SB()<<s<<":Pul"<<1, 1, this)
    ,out_divider(SB()<<s<<":OUT:CLK", 1, this)
    ,out_pulse0(SB()<<s<<":OUT:TR"<<0, 2, this)
    ,out_pulse1(SB()<<s<<":OUT:TR"<<1, 3, this)
    ,mappings(256)
{
    epicsUInt32 info = LE_READ32(base, FWInfo);

    switch((info&FWInfo_Flavor_mask)>>FWInfo_Flavor_shift) {
    case FWInfo_Flavor_EVG:
    {
        epicsTimeStamp now;
        if(epicsTimeOK == generalTimeGetExceptPriority(&now, 0, 50)) {
            // start simulated time from the present
            timeoffset = POSIX_TIME_AT_EPICS_EPOCH + now.secPastEpoch - LE_READ32(base, TimeSec);
        }
        fprintf(stderr, "%s: is FGPDB EVG\n", s.c_str());
        is_evg = true;
        break;
    }
    case FWInfo_Flavor_EVR:
        fprintf(stderr, "%s: is FGPDB EVR\n", s.c_str());
        break;
    default:
        fprintf(stderr, "%s: is Unknown! %08x\n", s.c_str(), (unsigned)info);
        throw std::runtime_error("Invalid FW Flavor");
    }

    // disable FIFO
    LE_WRITE32(base, FIFOEna, 0);

    // disable simulation modes
    LE_WRITE32(base, Config, 0);

    // disable w/ divider==0 (/1)
    LE_WRITE32(base, OutSelect, 0);

    // Partial reset and force FPS trip
    LE_WRITE32(base, Command, Command_ResetEVR|Command_NOKForce);

    LE_WRITE32(base, Command, 0);

    for(unsigned i=1; i<=255; i++) {
        // initially, disable all mappings and unmap from FIFO
        LE_WRITE32(base, EvtConfig(i), EvtConfig_FIFOUnMap);
    }

    scanIoInit(&statusScan);
}

EVRFRIB::~EVRFRIB() {}

epicsUInt32 EVRFRIB::machineCycles() const
{
    return (LE_READ32(base, Status)&Status_CycleCnt_mask)>>Status_CycleCnt_shift;
}

epicsUInt32 EVRFRIB::Config() const
{
    return LE_READ32(base, Config);
}

void EVRFRIB::setConfig(epicsUInt32 v)
{
    internal_clk = !!(v&Config_EVGSim);
    LE_WRITE32(base, Config, v);
}

epicsUInt32 EVRFRIB::Command() const
{
    return LE_READ32(base, Command);
}

void EVRFRIB::setCommand(epicsUInt32 v)
{
    const epicsUInt32 mask = (Command_ForceNPermit|Command_NOKClear|Command_NOKForce);
    v &= mask;
    v |= LE_READ32(base, Command)&(~mask);
    LE_WRITE32(base, Command, v);
}

epicsUInt32 EVRFRIB::FPSCommCnt() const
{
    return LE_READ32(base, FPSComm);
}

epicsUInt32 EVRFRIB::FPSStatus() const
{
    return LE_READ32(base, FPSStatus);
}

epicsUInt32 EVRFRIB::FPSSource() const
{
    return LE_READ32(base, FPSSource);
}

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
    switch((LE_READ32(base, FWInfo)&FWInfo_Flavor_mask)>>FWInfo_Flavor_shift)
    {
    case FWInfo_Flavor_EVR: return "EVR";
    case FWInfo_Flavor_EVG: return "EVG";
    default: return "???";
    }
}

epicsUInt32 EVRFRIB::version() const {
    return (LE_READ32(base, FWInfo)&FWInfo_Version_mask)>>FWInfo_Version_shift;
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
        ret = linkStatus() || (is_evg && internal_clk);

    if(ret && ts) {
        double period=1e9/clockTS();

        if(period<=0 || !isfinite(period))
            return false;

        epicsUInt32 sec = LE_READ32(base, TimeSec);
        epicsUInt32 ns  = LE_READ32(base, TimeFrac)*period;

        if(ns>=1000000000 || sec < timeoffset) {
            return false;
        }

        ts->secPastEpoch = sec - timeoffset;
        ts->nsec = ns;
    }

    return ret;
}

bool EVRFRIB::getTicks(epicsUInt32 *tks) {
    *tks = LE_READ32(base, TimeFrac);
    return true;
}

IOSCANPVT EVRFRIB::eventOccurred(epicsUInt32 event) const
{
    return statusScan;
}

void EVRFRIB::eventNotifyAdd(epicsUInt32 event, eventCallback, void*)
{
}

void EVRFRIB::eventNotifyDel(epicsUInt32 event, eventCallback, void*)
{
}


bool EVRFRIB::linkStatus() const {
    return LE_READ32(base, Status) & Status_Alive;
}

OBJECT_BEGIN2(EVRFRIB, EVR)
  OBJECT_PROP2("Config", &EVRFRIB::Config, &EVRFRIB::setConfig);
  OBJECT_PROP2("Command", &EVRFRIB::Command, &EVRFRIB::setCommand);
  OBJECT_PROP1("machineCycles", &EVRFRIB::machineCycles);
  OBJECT_PROP1("FPSCommCnt", &EVRFRIB::FPSCommCnt);
  OBJECT_PROP1("FPSStatus", &EVRFRIB::FPSStatus);
  OBJECT_PROP1("FPSSource", &EVRFRIB::FPSSource);
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
    LE_WRITE32(evr->base, PulserDelay(n), v);
}

void PulserFRIB::setDelay(double v)
{
    const double clock = evr->clock();

    epicsUInt32 ticks=roundToUInt(v*clock);

    setDelayRaw(ticks);
}

epicsUInt32 PulserFRIB::delayRaw() const
{
    return LE_READ32(evr->base, PulserDelay(n));
}

double PulserFRIB::delay() const
{
    double ticks=double(delayRaw());
    double clk=evr->clock(); // in MHz.  MTicks/second

    return ticks/clk;
}


void PulserFRIB::setWidthRaw(epicsUInt32 v)
{
    LE_WRITE32(evr->base, PulserWidth(n), v);
}

void PulserFRIB::setWidth(double v)
{
    const double clock = evr->clock();

    epicsUInt32 ticks=roundToUInt(v*clock);

    setWidthRaw(ticks);
}

epicsUInt32 PulserFRIB::widthRaw() const
{
    return LE_READ32(evr->base, PulserWidth(n));
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

    return evr->mappings[evt].pulse[n].active;
}

void PulserFRIB::sourceSetMap(epicsUInt32 evt, MapType::type action)
{
    if(evt>255)
        throw std::out_of_range("Event code is out of range");

    if(evt==0)
        return;

    EVRFRIB::EvtMap& map = evr->mappings[evt];
    EVRFRIB::PulseMap& pulse = map.pulse[n];

    bool writeout = false;

    if(action==MapType::None) {
        if(pulse.cnt==0u) {
            errlogPrintf("%s: Warning: mapping ref count error evt=%u map=%u", name().c_str(), (unsigned)evt, (unsigned)n);
        } else {
            pulse.cnt--;
            if(pulse.cnt==0) {
                writeout = true;
                pulse.active = action;
            }
        }
    } else {
        if(pulse.cnt==0) {
            writeout = true;
            pulse.active = action;
        } else if(pulse.active!=action) {
            throw std::runtime_error("Not allowed to map one pulse to more than one action for a single event");
        }
        pulse.cnt++;
    }

    if(!writeout) return;

    epicsUInt32 out = EvtConfig_FIFOUnMap;

    for(unsigned i=0; i<2; i++) {

        switch(map.pulse[i].active) {
        case MapType::None: break;
        case MapType::Trigger: out |= EvtConfig_Pulse(n); break;
        case MapType::Set: out |= EvtConfig_Set(n); break;
        case MapType::Reset: out |= EvtConfig_Clear(n); break;
        }
    }

    LE_WRITE32(evr->base, EvtConfig(evt), out);
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
    return (LE_READ32(evr.base, OutSelect)&OutSelect_Divider_mask)>>OutSelect_Divider_shift;
}

void PreScalerFRIB::setPrescaler(epicsUInt32 v)
{
    EVRFRIB& evr = static_cast<EVRFRIB&>(owner);
    epicsUInt32 T = LE_READ32(evr.base, OutSelect);
    T &= ~OutSelect_Divider_mask;
    T |= (v<<OutSelect_Divider_shift)&OutSelect_Divider_mask;
    LE_WRITE32(evr.base, OutSelect, T);
}

void PreScalerFRIB::lock() const { return owner.lock(); }
void PreScalerFRIB::unlock() const { return owner.unlock(); }


OutputFRIB::OutputFRIB(const std::string& n, epicsUInt32 src, EVRFRIB *evr) : Output(n), src(src), evr(evr) {}
OutputFRIB::~OutputFRIB() {}

bool OutputFRIB::enabled() const
{
    switch(src) {
    case 1:
        return LE_READ32(evr->base, OutSelect)&OutSelect_Enable;
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
