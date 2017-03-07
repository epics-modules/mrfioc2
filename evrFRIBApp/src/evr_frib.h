/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2017.
 *
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */
#ifndef EVR_FRIB_H
#define EVR_FRIB_H

#include <stdexcept>
#include <vector>

#include <dbScan.h>
#include <epicsMutex.h>

#include "evr/evr.h"
#include "evr/pulser.h"
#include "evr/prescaler.h"
#include "evr/output.h"

struct EVRFRIB;

struct OutputFRIB : public Output
{
    epicsUInt32 src;
    EVRFRIB *evr;

    OutputFRIB(const std::string& n, epicsUInt32 src, EVRFRIB *evr);
    virtual ~OutputFRIB();

    virtual epicsUInt32 source() const { return src; }
    virtual void setSource(epicsUInt32) {}

    virtual bool enabled() const;
    virtual void enable(bool);

    virtual const char*sourceName(epicsUInt32) const;

    virtual void lock() const;
    virtual void unlock() const;
};

struct PreScalerFRIB : public PreScaler
{
    PreScalerFRIB(const std::string& n, EVRFRIB& o);
    virtual ~PreScalerFRIB();

    virtual epicsUInt32 prescaler() const;
    virtual void setPrescaler(epicsUInt32);

    virtual void lock() const;
    virtual void unlock() const;
};

struct PulserFRIB : public Pulser
{
    unsigned n;
    EVRFRIB *evr;

    PulserFRIB(const std::string& s, unsigned n, EVRFRIB *evr);
    virtual ~PulserFRIB();

    virtual bool enabled() const { return true;}
    virtual void enable(bool) {}

    virtual void setDelayRaw(epicsUInt32);
    virtual void setDelay(double);
    virtual epicsUInt32 delayRaw() const;
    virtual double delay() const;

    virtual void setWidthRaw(epicsUInt32);
    virtual void setWidth(double);
    virtual epicsUInt32 widthRaw() const;
    virtual double width() const;

    virtual epicsUInt32 prescaler() const { return 1u; }
    virtual void setPrescaler(epicsUInt32) {}

    virtual bool polarityInvert() const { return false; }
    virtual void setPolarityInvert(bool) {}

    virtual MapType::type mappedSource(epicsUInt32 src) const;
    virtual void sourceSetMap(epicsUInt32 src,MapType::type action);

    virtual void lock() const;
    virtual void unlock() const;
};

struct EVRFRIB : public mrf::ObjectInst<EVRFRIB, EVR>
{
    typedef mrf::ObjectInst<EVRFRIB, EVR> base_t;

    mutable epicsMutex mutex;

    volatile unsigned char *base;

    double clockFreq; // MHz

    bool is_evg, internal_clk;

    // epicsTime = HWtime - timeoffset
    epicsUInt32 timeoffset;

    IOSCANPVT statusScan;

    PreScalerFRIB divider;
    PulserFRIB pulse0, pulse1;
    OutputFRIB out_divider,
               out_pulse0,
               out_pulse1;

    struct PulseMap {
        MapType::type active;
        unsigned cnt;
        PulseMap() :active(MapType::None), cnt(0u) {}
    };
    struct EvtMap {
        PulseMap pulse[2];
    };
    typedef std::vector<EvtMap> mappings_t;
    mappings_t mappings;

    // our methods

    EVRFRIB(const std::string& s, bus_configuration& busConfig, volatile unsigned char *base);
    virtual ~EVRFRIB();

    // from Status
    epicsUInt32 machineCycles() const;

    epicsUInt32 Config() const;
    void setConfig(epicsUInt32 v);

    // From Command
    epicsUInt32 Command() const;
    void setCommand(epicsUInt32 v);

    epicsUInt32 FPSCommCnt() const;
    epicsUInt32 FPSStatus() const;
    epicsUInt32 FPSSource() const;

    // methods from Object

    virtual void lock() const;
    virtual void unlock() const;

    // methods from EVR

    virtual std::string model() const;
    virtual epicsUInt32 version() const;

    virtual bool enabled() const { return true; }
    virtual void enable(bool) {}

    virtual bool mappedOutputState() const { return false; }

    virtual bool specialMapped(epicsUInt32 code, epicsUInt32 func) const { return false; }
    virtual void specialSetMap(epicsUInt32 code, epicsUInt32 func,bool set) {}

    virtual double clock() const;
    virtual void clockSet(double clk);

    virtual bool pllLocked() const { return linkStatus(); }

    virtual epicsUInt32 uSecDiv() const { return 0; }

    virtual bool extInhib() const { return false; }
    virtual void setExtInhib(bool) {}

    virtual void setSourceTS(TSSource) { }
    virtual TSSource SourceTS() const { return TSSourceInternal; }

    virtual double clockTS() const;
    virtual void clockTSSet(double);

    virtual epicsUInt32 tsDiv() const { return 1u; }

    /** Indicate (lack of) interest in a particular event code.
     *  This allows an EVR to ignore event codes which are not needed.
     */
    virtual bool interestedInEvent(epicsUInt32 event,bool set);

    virtual bool TimeStampValid() const;
    virtual IOSCANPVT TimeStampValidEvent() const { return statusScan; }

    virtual bool getTimeStamp(epicsTimeStamp *ts,epicsUInt32 event);

    /** Returns the current value of the Timestamp Event Counter
     *@param tks Pointer to be filled with the counter value
     *@return false if the counter value is not valid
     */
    virtual bool getTicks(epicsUInt32 *tks);

    virtual IOSCANPVT eventOccurred(epicsUInt32 event) const;

    virtual void eventNotifyAdd(epicsUInt32 event, eventCallback, void*);
    virtual void eventNotifyDel(epicsUInt32 event, eventCallback, void*);

    virtual epicsUInt32 irqCount() const { return 0; }

    virtual bool linkStatus() const;
    virtual IOSCANPVT linkChanged() const { return statusScan; }
    virtual epicsUInt32 recvErrorCount() const { return 0; }

    virtual epicsUInt16 dbus() const { return 0; }

    virtual epicsUInt32 heartbeatTIMOCount() const { return 0; }
    virtual IOSCANPVT heartbeatTIMOOccured() const { return statusScan; }

    virtual epicsUInt32 FIFOFullCount() const { return 0; }
    virtual epicsUInt32 FIFOOverRate() const { return 0; }
    virtual epicsUInt32 FIFOEvtCount() const { return 0; }
    virtual epicsUInt32 FIFOLoopCount() const { return 0; }
};


#endif // EVR_FRIB_H
