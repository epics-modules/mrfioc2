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

#include "evr/evr.h"

struct EVRFRIB : public mrf::ObjectInst<EVRFRIB, EVR>
{
    typedef mrf::ObjectInst<EVRFRIB, EVR> base_t;
    EVRFRIB(const std::string& s, bus_configuration& busConfig, volatile unsigned char *base);
    virtual ~EVRFRIB();

    volatile unsigned char *base;

    virtual std::string model() const { return "FGPDB"; }
    virtual epicsUInt32 version() const;

    virtual bool enabled() const;
    virtual void enable(bool);

    virtual bool mappedOutputState() const { return false; }

    virtual bool specialMapped(epicsUInt32 code, epicsUInt32 func) const { return false; }
    virtual void specialSetMap(epicsUInt32 code, epicsUInt32 func,bool set) {}

    virtual double clock() const;
    virtual void clockSet(double clk);

    //! Internal PLL Status
    virtual bool pllLocked() const;

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

    IOSCANPVT statusScan;

    virtual bool TimeStampValid() const;
    virtual IOSCANPVT TimeStampValidEvent() const { return statusScan; }

    virtual bool getTimeStamp(epicsTimeStamp *ts,epicsUInt32 event);

    /** Returns the current value of the Timestamp Event Counter
     *@param tks Pointer to be filled with the counter value
     *@return false if the counter value is not valid
     */
    virtual bool getTicks(epicsUInt32 *tks);

    virtual IOSCANPVT eventOccurred(epicsUInt32 event) const;

    typedef void (*eventCallback)(void* userarg, epicsUInt32 event);
    virtual void eventNotifyAdd(epicsUInt32 event, eventCallback, void*);
    virtual void eventNotifyDel(epicsUInt32 event, eventCallback, void*);

    virtual epicsUInt32 irqCount() const { return 0; }

    virtual bool linkStatus() const;
    virtual IOSCANPVT linkChanged() const { return statusScan; }
    virtual epicsUInt32 recvErrorCount() const;

    virtual epicsUInt16 dbus() const { return 0; }

    virtual epicsUInt32 heartbeatTIMOCount() const { return 0; }
    virtual IOSCANPVT heartbeatTIMOOccured() const { return statusScan; }

    virtual epicsUInt32 FIFOFullCount() const { return 0; }
    virtual epicsUInt32 FIFOOverRate() const { return 0; }
    virtual epicsUInt32 FIFOEvtCount() const { return 0; }
    virtual epicsUInt32 FIFOLoopCount() const { return 0; }
};


#endif // EVR_FRIB_H
