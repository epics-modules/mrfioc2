/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef EVRMRML_H_INC
#define EVRMRML_H_INC

#include "evr/evr.h"
#include "mrf/spi.h"

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
#include "delayModule.h"
#include "drvemRxBuf.h"
#include "mrmevrseq.h"

#include "mrmGpio.h"
#include "mrmtimesrc.h"
#include "mrmDataBufTx.h"
#include "sfp.h"
#include "configurationInfo.h"

class EVRMRM;

struct epicsShareClass eventCode {
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
class epicsShareClass EVRMRM : public mrf::ObjectInst<EVRMRM, EVR>,
                                      mrf::SPIInterface,
                               public TimeStampSource
{
    typedef mrf::ObjectInst<EVRMRM, EVR> base_t;
public:
    /** @brief Guards access to instance
   *  All callers must take this lock before any operations on
   *  this object.
   */
    mutable epicsMutex evrLock;

    struct Config {
        const char *model;
        size_t nPul; // number of pulsers
        size_t nPS;   // number of prescalers
        // # of outputs (Front panel, FP Universal, Rear transition module, Backplane)
        size_t nOFP, nOFPUV, nORB, nOBack;
        size_t nOFPDly;  // # of slots== # of delay modules. Some of the FP Universals have GPIOs. Each FPUV==2 GPIO pins, 2 FPUVs in one slot = 4 GPIO pins. One dly module uses 4 GPIO pins.
        // # of CML outputs
        size_t nCML;
        MRMCML::outkind kind;
        // # of FP inputs
        size_t nIFP;
    };

    EVRMRM(const std::string& n, bus_configuration& busConfig,
           const Config *c,
           volatile unsigned char*,epicsUInt32);

    virtual ~EVRMRM();
private:
    void cleanup();
public:

    // SPI access
    virtual void select(unsigned id);
    virtual epicsUInt8 cycle(epicsUInt8 in);

    virtual void lock() const OVERRIDE FINAL {evrLock.lock();}
    virtual void unlock() const OVERRIDE FINAL {evrLock.unlock();};

    virtual std::string model() const OVERRIDE FINAL;
    epicsUInt32 fpgaFirmware();
    formFactor getFormFactor();
    std::string formFactorStr();
    virtual MRFVersion version() const OVERRIDE FINAL;


    virtual bool enabled() const OVERRIDE FINAL;
    virtual void enable(bool v) OVERRIDE FINAL;

    virtual bool mappedOutputState() const OVERRIDE FINAL;

    MRMGpio* gpio();

    virtual bool specialMapped(epicsUInt32 code, epicsUInt32 func) const OVERRIDE FINAL;
    virtual void specialSetMap(epicsUInt32 code, epicsUInt32 func,bool) OVERRIDE FINAL;

    virtual double clock() const OVERRIDE FINAL
        {SCOPED_LOCK(evrLock);return eventClock;}
    virtual void clockSet(double) OVERRIDE FINAL;

    virtual bool pllLocked() const OVERRIDE FINAL;

    virtual epicsUInt32 irqCount() const OVERRIDE FINAL {return count_hardware_irq;}

    virtual bool linkStatus() const OVERRIDE FINAL;
    virtual IOSCANPVT linkChanged() const OVERRIDE FINAL{return IRQrxError;}
    virtual epicsUInt32 recvErrorCount() const OVERRIDE FINAL{return count_recv_error;}

    virtual epicsUInt32 uSecDiv() const OVERRIDE FINAL;

    //! Using external hardware input for inhibit?
    virtual bool extInhib() const OVERRIDE FINAL;
    virtual void setExtInhib(bool) OVERRIDE FINAL;

    virtual epicsUInt32 tsDiv() const OVERRIDE FINAL
        {SCOPED_LOCK(evrLock);return shadowCounterPS;}

    virtual void setSourceTS(TSSource) OVERRIDE FINAL;
    virtual TSSource SourceTS() const OVERRIDE FINAL
        {SCOPED_LOCK(evrLock);return shadowSourceTS;}
    virtual double clockTS() const OVERRIDE FINAL;
    virtual void clockTSSet(double) OVERRIDE FINAL;
    virtual bool interestedInEvent(epicsUInt32 event,bool set) OVERRIDE FINAL;

    virtual bool TimeStampValid() const OVERRIDE FINAL;
    virtual IOSCANPVT TimeStampValidEvent() const OVERRIDE FINAL {return timestampValidChange;}

    virtual bool getTimeStamp(epicsTimeStamp *ts,epicsUInt32 event) OVERRIDE FINAL;
    virtual bool getTicks(epicsUInt32 *tks) OVERRIDE FINAL;
    virtual IOSCANPVT eventOccurred(epicsUInt32 event) const OVERRIDE FINAL;
    virtual void eventNotifyAdd(epicsUInt32, eventCallback, void*) OVERRIDE FINAL;
    virtual void eventNotifyDel(epicsUInt32, eventCallback, void*) OVERRIDE FINAL;

    bool convertTS(epicsTimeStamp* ts);

    virtual epicsUInt16 dbus() const OVERRIDE FINAL;

    virtual epicsUInt32 heartbeatTIMOCount() const OVERRIDE FINAL {return count_heartbeat;}
    virtual IOSCANPVT heartbeatTIMOOccured() const OVERRIDE FINAL {return IRQheartbeat;}

    virtual epicsUInt32 FIFOFullCount() const OVERRIDE FINAL
    {SCOPED_LOCK(evrLock);return count_FIFO_overflow;}
    virtual epicsUInt32 FIFOOverRate() const OVERRIDE FINAL
    {SCOPED_LOCK(evrLock);return count_FIFO_sw_overrate;}
    virtual epicsUInt32 FIFOEvtCount() const OVERRIDE FINAL {return count_fifo_events;}
    virtual epicsUInt32 FIFOLoopCount() const OVERRIDE FINAL {return count_fifo_loops;}

    void enableIRQ(void);

    bool dcEnabled() const;
    void dcEnable(bool v);
    double dcTarget() const;
    void dcTargetSet(double);
    //! Measured delay
    double dcRx() const;
    //! Delay compensation applied
    double dcInternal() const;
    epicsUInt32 dcStatusRaw() const;
    epicsUInt32 topId() const;

    //! Read raw delay register
    epicsUInt32 ECP3DelayRaw() const;
    //! Phase between FIFO Read/Write clocks in 16ths of a cycle
    epicsUInt32 ECP3DPhase() const;
    void setECP3DPhase(epicsUInt32);
    //! Increment/Decrement delay by 1 whole cycle
    bool dummyBool() const {return false;}
    void ECP3DelayIncrease(bool i);
    void ECP3DelayDecrease(bool d);

    epicsUInt32 dummy() const { return 0; }
    void setEvtCode(epicsUInt32 code) OVERRIDE FINAL;

    epicsUInt32 timeSrc() const;
    void setTimeSrc(epicsUInt32 mode);

    static void isr(EVRMRM *evr, bool pci);
    static void isr_pci(void*);
    static void isr_vme(void*);
#if defined(__linux__) || defined(_WIN32)
    const void *isrLinuxPvt;
#endif

    const Config * const conf;
    volatile unsigned char * const base;
    epicsUInt32 baselen;
    mrmDataBufTx buftx;
    mrmBufRx bufrx;
    mrf::auto_ptr<SFP> sfp;
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

    std::vector<DelayModule*> delays;

    typedef std::vector<MRMPreScaler*> prescalers_t;
    prescalers_t prescalers;

    typedef std::vector<MRMPulser*> pulsers_t;
    pulsers_t pulsers;

    typedef std::vector<MRMCML*> shortcmls_t;
    shortcmls_t shortcmls;

    MRMGpio gpio_;

    mrf::auto_ptr<EvrSeqManager> seq;

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

    // Periodic callback to detect when link state goes from down to up
    CALLBACK poll_link_cb;
    static void poll_link(CALLBACK*);

    enum timeSrcMode_t {
        Disable,  // do nothing
        External, // shift out TS on upstream when reset (125) received on downstream
        SysClk,   // generate reset (125) from software timer, shift out TS on upstream
    } timeSrcMode;
    /* in practice
     *   timeSrcMode!=Disable -> listen for 125, react by sending shift 0/1 codes
     *   timeSrcMode==SysClk  -> send soft 125 events
     */
    CALLBACK timeSrc_cb;

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
    bool _ismap(epicsUInt8 evt, epicsUInt8 func) const { return (_mapped[evt] & 1<<(func)) != 0; }
}; // class EVRMRM

#endif // EVRMRML_H_INC
