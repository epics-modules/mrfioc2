/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef EVRMRML_H_INC
#define EVRMRML_H_INC

#include "evr/evr.h"

#include <string>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <utility>

#include <dbScan.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsMessageQueue.h>
#include <callback.h>
#include <epicsMutex.h>

#include "drvemInput.h"
#include "drvemOutput.h"
#include "drvemPrescaler.h"
#include "drvemPulser.h"
#include "drvemCML.h"
#include "drvemRxBuf.h"

#include "mrmDataBufTx.h"
#include "sfp.h"

//! @brief Helper to allow one class to have several runable methods
template<class C,void (C::*Method)()>
class epicsThreadRunableMethod : public epicsThreadRunable
{
    C& owner;
public:
    epicsThreadRunableMethod(C& o)
        :owner(o)
    {}
    virtual ~epicsThreadRunableMethod(){}
    virtual void run()
    {
        (owner.*Method)();
    }
};

class EVRMRM;

struct eventCode {
    epicsUInt8 code; // constant
    EVRMRM* owner;

    // For efficiency events will only
    // be mapped into the FIFO when this
    // counter is non-zero.
    size_t interested;

    epicsUInt32 last_sec;
    epicsUInt32 last_evt;

    IOSCANPVT occured;

    typedef std::list<std::pair<EVR::eventCallback,void*> > notifiees_t;
    notifiees_t notifiees;

    CALLBACK done;
    size_t waitingfor;
    bool again;

    eventCode():owner(0), interested(0), last_sec(0)
            ,last_evt(0), notifiees(), waitingfor(0), again(false)
    {
        scanIoInit(&occured);
        // done - initialized in EVRMRM::EVRMRM()
  }
};

/**@brief Modular Register Map Event Receivers
 *
 * 
 */
class EVRMRM : public EVR
{
public:    
    /** @brief Guards access to instance
   *  All callers must take this lock before any operations on
   *  this object.
   */
    mutable epicsMutex evrLock;


    EVRMRM(const std::string& n, const std::string& p,volatile unsigned char*,epicsUInt32);

    virtual ~EVRMRM();
private:
    void cleanup();
public:

    virtual void lock() const{evrLock.lock();};
    virtual void unlock() const{evrLock.unlock();};

    virtual epicsUInt32 model() const;

    virtual epicsUInt32 version() const;

    virtual bool enabled() const;
    virtual void enable(bool v);

    virtual MRMPulser* pulser(epicsUInt32);
    virtual const MRMPulser* pulser(epicsUInt32) const;

    virtual MRMOutput* output(OutputType,epicsUInt32 o);
    virtual const MRMOutput* output(OutputType,epicsUInt32 o) const;

    virtual MRMInput* input(epicsUInt32 idx);
    virtual const MRMInput* input(epicsUInt32) const;

    virtual MRMPreScaler* prescaler(epicsUInt32);
    virtual const MRMPreScaler* prescaler(epicsUInt32) const;

    virtual MRMCML* cml(epicsUInt32 idx);
    virtual const MRMCML* cml(epicsUInt32) const;

    virtual bool specialMapped(epicsUInt32 code, epicsUInt32 func) const;
    virtual void specialSetMap(epicsUInt32 code, epicsUInt32 func,bool);

    virtual double clock() const
        {SCOPED_LOCK(evrLock);return eventClock;}
    virtual void clockSet(double);

    virtual bool pllLocked() const;

    virtual epicsUInt32 irqCount() const{return count_hardware_irq;}

    virtual bool linkStatus() const;
    virtual IOSCANPVT linkChanged() const{return IRQrxError;}
    virtual epicsUInt32 recvErrorCount() const{return count_recv_error;}

    virtual epicsUInt32 uSecDiv() const;

    //! Using external hardware input for inhibit?
    virtual bool extInhib() const;
    virtual void setExtInhib(bool);

    virtual epicsUInt32 tsDiv() const
        {SCOPED_LOCK(evrLock);return shadowCounterPS;}

    virtual void setSourceTS(TSSource);
    virtual TSSource SourceTS() const
        {SCOPED_LOCK(evrLock);return shadowSourceTS;}
    virtual double clockTS() const;
    virtual void clockTSSet(double);
    virtual bool interestedInEvent(epicsUInt32 event,bool set);

    virtual bool TimeStampValid() const;
    virtual IOSCANPVT TimeStampValidEvent() const{return timestampValidChange;}

    virtual bool getTimeStamp(epicsTimeStamp *ts,epicsUInt32 event);
    virtual bool getTicks(epicsUInt32 *tks);
    virtual IOSCANPVT eventOccurred(epicsUInt32 event) const;
    virtual void eventNotifyAdd(epicsUInt32, eventCallback, void*);
    virtual void eventNotifyDel(epicsUInt32, eventCallback, void*);

    bool convertTS(epicsTimeStamp* ts);

    virtual epicsUInt16 dbus() const;

    virtual epicsUInt32 heartbeatTIMOCount() const{return count_heartbeat;}
    virtual IOSCANPVT heartbeatTIMOOccured() const{return IRQheartbeat;}

    virtual epicsUInt32 FIFOFullCount() const
    {SCOPED_LOCK(evrLock);return count_FIFO_overflow;}
    virtual epicsUInt32 FIFOOverRate() const
    {SCOPED_LOCK(evrLock);return count_FIFO_sw_overrate;}
    virtual epicsUInt32 FIFOEvtCount() const{return count_fifo_events;}
    virtual epicsUInt32 FIFOLoopCount() const{return count_fifo_loops;}

    void enableIRQ(void);

    static void isr(void*);
#ifdef __linux__
    const void *isrLinuxPvt;
#endif

    const std::string id;
    volatile unsigned char * const base;
    epicsUInt32 baselen;
    mrmDataBufTx buftx;
    mrmBufRx bufrx;
    std::auto_ptr<SFP> sfp;
private:

    // Set by ISR
    volatile epicsUInt32 count_recv_error;
    volatile epicsUInt32 count_hardware_irq;
    volatile epicsUInt32 count_heartbeat;
    volatile epicsUInt32 count_fifo_events;
    volatile epicsUInt32 count_fifo_loops;

    epicsUInt32 shadowIRQEna;

    // Guarded by evrLock
    epicsUInt32 count_FIFO_overflow;

    // scanIoRequest() from ISR or callback
    IOSCANPVT IRQmappedEvent; // Hardware mapped IRQ
    IOSCANPVT IRQheartbeat;   // Heartbeat timeout
    IOSCANPVT IRQrxError;     // Rx link state change
    IOSCANPVT IRQfifofull;    // Fifo overflow

    // Software events
    IOSCANPVT timestampValidChange;

    // Set by ctor, not changed after

    typedef std::vector<MRMInput*> inputs_t;
    inputs_t inputs;

    typedef std::map<std::pair<OutputType,epicsUInt32>,MRMOutput*> outputs_t;
    outputs_t outputs;

    typedef std::vector<MRMPreScaler*> prescalers_t;
    prescalers_t prescalers;

    typedef std::vector<MRMPulser*> pulsers_t;
    pulsers_t pulsers;

    typedef std::vector<MRMCML*> shortcmls_t;
    shortcmls_t shortcmls;

    // run when FIFO not-full IRQ is received
    void drain_fifo();
    epicsThreadRunableMethod<EVRMRM, &EVRMRM::drain_fifo> drain_fifo_method;
    epicsThread drain_fifo_task;
    epicsMessageQueue drain_fifo_wakeup;
    static void sentinel_done(CALLBACK*);

    epicsUInt32 count_FIFO_sw_overrate;

    eventCode events[256];

    // Buffer received
    CALLBACK data_rx_cb;

    // Called when the Event Log is stopped
    CALLBACK drain_log_cb;
    static void drain_log(CALLBACK*);

    // Periodic callback to detect when link state goes from down to up
    CALLBACK poll_link_cb;
    static void poll_link(CALLBACK*);

    // Set by clockTSSet() with IRQ disabled
    double stampClock;
    TSSource shadowSourceTS;
    epicsUInt32 shadowCounterPS;
    double eventClock; //!< Stored in Hz

    epicsUInt32 timestampValid;
    epicsUInt32 lastInvalidTimestamp;
    epicsUInt32 lastValidTimestamp;
    static void seconds_tick(void*, epicsUInt32);

    // bit map of which event #'s are mapped
    // used as a safty check to avoid overloaded mappings
    epicsUInt32 _mapped[256];

    void _map(epicsUInt8 evt, epicsUInt8 func)   { _mapped[evt] |=    1<<(func);  }
    void _unmap(epicsUInt8 evt, epicsUInt8 func) { _mapped[evt] &= ~( 1<<(func) );}
    bool _ismap(epicsUInt8 evt, epicsUInt8 func) const { return _mapped[evt] & 1<<(func); }
}; // class EVRMRM

#endif // EVRMRML_H_INC
