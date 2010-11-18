
#include "drvem.h"

#include <cstdio>
#include <stdexcept>
#include <algorithm>

#include <epicsMath.h>
#include <errlog.h>

#include <mrfCommon.h>
#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "evrRegMap.h"

#include "mrfFracSynth.h"

#include "drvemIocsh.h"

#include <dbDefs.h>
#include <dbScan.h>
#include <epicsInterrupt.h>

#define DBG evrmrmVerb

#define CBINIT(ptr, prio, fn, valptr) \
do { \
  callbackSetPriority(prio, ptr); \
  callbackSetCallback(fn, ptr);   \
  callbackSetUser(valptr, ptr);   \
  (ptr)->timer=NULL;              \
} while(0)

/*Note: All locking involving the ISR done by disabling interrupts
 *      since the OSI library doesn't provide more efficient
 *      constructs like a ISR safe spinlock.
 *
 *      Since OSI does not provide r/w locks either, writing to
 *      infrequently modified member variables and register
 *      bit fields is also done with interrupts disabled.
 *
 *      This will not be a problem as long as the system
 *      has only one cpu...
 */

// Fractional synthesizer reference clock frequency
static
const double fracref=24.0; // MHz

CardMap<dataBufRx> datarxmap;

EVRMRM::EVRMRM(int i,volatile unsigned char* b)
  :EVR()
  ,id(i)
  ,base(b)
  ,buftx(b+U32_DataTxCtrl, b+U8_DataTx_base)
  ,bufrx(b, 10) // Sets depth of Rx queue
  ,stampClock(0.0)
  ,shadowSourceTS(TSSourceInternal)
  ,shadowCounterPS(0)
  ,count_recv_error(0)
  ,count_hardware_irq(0)
  ,count_heartbeat(0)
  ,outputs()
  ,prescalers()
  ,pulsers()
  ,shortcmls()
  ,events_lock()
{
    epicsUInt32 v = READ32(base, FWVersion),evr,ver;

    if(v==0xFfffFfff)
        throw std::runtime_error("Not card present at address");

    evr=v&FWVersion_type_mask;
    evr>>=FWVersion_type_shift;

    if(evr!=0x1)
        throw std::runtime_error("Address does not correspond to an EVR");

    ver=v&FWVersion_ver_mask;
    ver>>=FWVersion_ver_shift;
    if(ver<3)
        throw std::runtime_error("Firmware version not supported");

    scanIoInit(&IRQmappedEvent);
    scanIoInit(&IRQbufferReady);
    scanIoInit(&IRQheadbeat);
    scanIoInit(&IRQrxError);
    scanIoInit(&IRQfifofull);

    CBINIT(&drain_fifo_cb, priorityMedium, &EVRMRM::drain_fifo, this);
    CBINIT(&data_rx_cb   , priorityHigh, &mrmBufRx::drainbuf, &this->bufrx);
    CBINIT(&drain_log_cb , priorityMedium, &EVRMRM::drain_log , this);
    CBINIT(&poll_link_cb , priorityMedium, &EVRMRM::poll_link , this);

    /*
     * Create subunit instances
     */

    v&=FWVersion_form_mask;
    v>>=FWVersion_form_shift;

    size_t nPul=10; // number of pulsers
    size_t nPS=3;   // number of prescalers
    // # of outputs (Front panel, FP Universal, Rear transition module)
    size_t nOFP=0, nOFPUV=0, nORB=0;
    // # of CML outputs
    size_t nCML=0;
    // # of FP inputs
    size_t nIFP=0;

    switch(v){
    case evrFormCPCI:
        if(DBG) printf("CPCI ");
        nOFPUV=4;
        nIFP=2;
        nORB=6;
        break;
    case evrFormPMC:
        if(DBG) printf("PMC ");
        nOFP=3;
        nIFP=1;
        break;
    case evrFormVME64:
        if(DBG) printf("VME64 ");
        nOFP=7;
        nCML=3; // OFP 4-6 are CML
        nOFPUV=4;
        nORB=16;
        nIFP=2;
        break;
    default:
        printf("Unknown EVR variant %d\n",v);
    }
    if(DBG) printf("Out FP:%u FPUNIV:%u RB:%u IFP:%u\n",
                   (unsigned int)nOFP,(unsigned int)nOFPUV,
                   (unsigned int)nORB,(unsigned int)nIFP);

    // Special output for mapping bus interrupt
    outputs[std::make_pair(OutputInt,0)]=new MRMOutput(base+U16_IRQPulseMap);

    inputs.resize(nIFP);
    for(size_t i=0; i<nIFP; i++){
        inputs[i]=new MRMInput(base,i);
    }

    for(size_t i=0; i<nOFP; i++){
        outputs[std::make_pair(OutputFP,i)]=new MRMOutput(base+U16_OutputMapFP(i));
    }

    for(size_t i=0; i<nOFPUV; i++){
        outputs[std::make_pair(OutputFPUniv,i)]=new MRMOutput(base+U16_OutputMapFPUniv(i));
    }

    for(size_t i=0; i<nORB; i++){
        outputs[std::make_pair(OutputRB,i)]=new MRMOutput(base+U16_OutputMapRB(i));
    }

    prescalers.resize(nPS);
    for(size_t i=0; i<nPS; i++){
        prescalers[i]=new MRMPreScaler(*this,base+U32_Scaler(i));
    }

    pulsers.resize(nPul);
    for(size_t i=0; i<nPul; i++){
        pulsers[i]=new MRMPulser(i,*this);
    }

    if(nCML && ver>=4){
        shortcmls.resize(nCML);
        for(size_t i=0; i<nCML; i++){
            shortcmls[i]=new MRMCML(i,*this);
        }
    }else if(nCML){
        printf("CML outputs not supported with this firmware\n");
    }

    for(size_t i=0; i<NELEMENTS(this->events); i++) {
        events[i].code=0;

        events[i].interested=0;

        events[i].last_sec=0;
        events[i].last_evt=0;

        scanIoInit(&events[i].occured);
    }

    eventClock=FracSynthAnalyze(READ32(base, FracDiv),
                                fracref,0)*1e6;

    shadowCounterPS=READ32(base, CounterPS);

    if(tsDiv()!=0) {
        shadowSourceTS=TSSourceInternal;
    } else {
        bool usedbus4=READ32(base, Control) & Control_tsdbus;

        if(usedbus4)
            shadowSourceTS=TSSourceDBus4;
        else
            shadowSourceTS=TSSourceEvent;
    }

}

EVRMRM::~EVRMRM()
{
    for(outputs_t::iterator it=outputs.begin();
        it!=outputs.end(); ++it)
    {
        delete it->second;
    }
    outputs.clear();
    for(prescalers_t::iterator it=prescalers.begin();
        it!=prescalers.end(); ++it)
    {
        delete (*it);
    }
    //TODO: cleanup the rest
}

epicsUInt32
EVRMRM::model() const
{
    epicsUInt32 v = READ32(base, FWVersion);

    return (v&FWVersion_form_mask)>>FWVersion_form_shift;
}

epicsUInt32
EVRMRM::version() const
{
    epicsUInt32 v = READ32(base, FWVersion);

    return (v&FWVersion_ver_mask)>>FWVersion_ver_shift;
}

bool
EVRMRM::enabled() const
{
    epicsUInt32 v = READ32(base, Control);
    return v&Control_enable;
}

void
EVRMRM::enable(bool v)
{
    int iflags=epicsInterruptLock();
    if(v)
        BITSET(NAT,32,base, Control, Control_enable|Control_mapena);
    else
        BITCLR(NAT,32,base, Control, Control_enable|Control_mapena);
    epicsInterruptUnlock(iflags);
}

MRMPulser*
EVRMRM::pulser(epicsUInt32 i)
{
    if(i>=pulsers.size())
        throw std::out_of_range("Pulser id is out of range");
    return pulsers[i];
}

const MRMPulser*
EVRMRM::pulser(epicsUInt32 i) const
{
    if(i>=pulsers.size())
        throw std::out_of_range("Pulser id is out of range");
    return pulsers[i];
}

MRMOutput*
EVRMRM::output(OutputType otype,epicsUInt32 idx)
{
    outputs_t::iterator it=outputs.find(std::make_pair(otype,idx));
    if(it==outputs.end())
        return 0;
    else
        return it->second;
}

const MRMOutput*
EVRMRM::output(OutputType otype,epicsUInt32 idx) const
{
    outputs_t::const_iterator it=outputs.find(std::make_pair(otype,idx));
    if(it==outputs.end())
        return 0;
    else
        return it->second;
}

MRMInput*
EVRMRM::input(epicsUInt32 i)
{
    if(i>=inputs.size())
        throw std::out_of_range("Input id is out of range");
    return inputs[i];
}

const MRMInput*
EVRMRM::input(epicsUInt32 i) const
{
    if(i>=inputs.size())
        throw std::out_of_range("Input id is out of range");
    return inputs[i];
}

MRMPreScaler*
EVRMRM::prescaler(epicsUInt32 i)
{
    if(i>=prescalers.size())
        throw std::out_of_range("PreScaler id is out of range");
    return prescalers[i];
}

const MRMPreScaler*
EVRMRM::prescaler(epicsUInt32 i) const
{
    if(i>=prescalers.size())
        throw std::out_of_range("PreScaler id is out of range");
    return prescalers[i];
}

MRMCML*
EVRMRM::cml(epicsUInt32 i)
{
    if(i>=shortcmls.size())
        throw std::out_of_range("CML Short id is out of range");
    return shortcmls[i];
}

const MRMCML*
EVRMRM::cml(epicsUInt32 i) const
{
    if(i>=shortcmls.size())
        throw std::out_of_range("CML Short id is out of range");
    return shortcmls[i];
}

bool
EVRMRM::specialMapped(epicsUInt32 code, epicsUInt32 func) const
{
    if(code>255)
        throw std::out_of_range("Event code is out of range");
    if(func>127 || func<96 ||
        (func<=121 && func>=102) )
    {
        throw std::out_of_range("Special function code is out of range");
    }

    if(code==0)
        return false;

    epicsUInt32 bit  =func%32;
    epicsUInt32 mask=1<<bit;

    epicsUInt32 val=READ32(base, MappingRam(0, code, Internal));

    val&=mask;

    return !!val;
}

void
EVRMRM::specialSetMap(epicsUInt32 code, epicsUInt32 func,bool v)
{
    if(code>255)
        throw std::out_of_range("Event code is out of range");
    /* The special function codes are the range 96 to 127
     */
    if(func>127 || func<96 ||
        (func<=121 && func>=102) )
    {
        errlogPrintf("EVR %d code %02x func %3d out of range\n",
            id, code, func);
        throw std::out_of_range("Special function code is out of range");
    }

    if(code==0)
      return;

    /* The way the latch timestamp is implimented in hardware (no status bit)
     * makes it impossible to use the latch mapping and the latch control register
     * bits at the same time.  We use the control register bits.
     * However, there is not much loss of functionality since all events
     * can be timestamped in the FIFO.
     */
    if(func==126)
        throw std::out_of_range("Use of latch timestamp special function code is not allowed");

    epicsUInt32 bit  =func%32;
    epicsUInt32 mask=1<<bit;

    int iflags=epicsInterruptLock();

    epicsUInt32 val=READ32(base, MappingRam(0, code, Internal));

    if (v && _ismap(code,func-96)) {
        // already set
        epicsInterruptUnlock(iflags);
        throw std::runtime_error("Ignore duplicate mapping");

    } else if(v) {
        _map(code,func-96);
        WRITE32(base, MappingRam(0, code, Internal), val|mask);
    } else {
        _unmap(code,func-96);
        WRITE32(base, MappingRam(0, code, Internal), val&~mask);
    }

    epicsInterruptUnlock(iflags);

    errlogPrintf("EVR #%d code %02x func %3d %s\n",
        id, code, func, v?"Set":"Clear");
}

void
EVRMRM::clockSet(double freq)
{
    double err;
    // Set both the fractional synthesiser and microsecond
    // divider.

    freq/=1e6;

    epicsUInt32 newfrac=FracSynthControlWord(
                        freq, fracref, 0, &err);

    if(newfrac==0)
        throw std::out_of_range("New frequency can't be used");

    int iflags=epicsInterruptLock();

    epicsUInt32 oldfrac=READ32(base, FracDiv);

    if(newfrac!=oldfrac){
        // Changing the control word disturbes the phase
        // of the synthesiser which will cause a glitch.
        // Don't change the control word unless needed.

        WRITE32(base, FracDiv, newfrac);

        eventClock=FracSynthAnalyze(READ32(base, FracDiv),
                                    fracref,0)*1e6;
    }

    // USecDiv is accessed as a 32 bit register, but
    // only 16 are used.
    epicsUInt16 oldudiv=READ32(base, USecDiv);
    epicsUInt16 newudiv=(epicsUInt16)freq;

    if(newudiv!=oldudiv){
        WRITE32(base, USecDiv, newudiv);
    }

    epicsInterruptUnlock(iflags);
}

epicsUInt32
EVRMRM::uSecDiv() const
{
    return READ32(base, USecDiv);
}

bool
EVRMRM::pllLocked() const
{
    return READ32(base, ClkCtrl) & ClkCtrl_cglock;
}

bool
EVRMRM::linkStatus() const
{
    return !(READ32(base, Status) & Status_legvio);
}

IOSCANPVT
EVRMRM::linkChanged()
{
    return IRQrxError;
}

epicsUInt32
EVRMRM::recvErrorCount() const
{
    return count_recv_error;
}

void
EVRMRM::setSourceTS(TSSource src)
{
    double clk=clockTS(), eclk=clock();
    epicsUInt16 div=0;

    if(clk<=0 || !isfinite(clk))
        throw std::out_of_range("TS Clock rate invalid");

    switch(src){
    case TSSourceInternal:
    case TSSourceEvent:
    case TSSourceDBus4:
        break;
    default:
        throw std::out_of_range("TS source invalid");
    }

    int iflags=epicsInterruptLock();

    switch(src){
    case TSSourceInternal:
        // div!=0 selects src internal
        div=(epicsUInt16)(eclk/clk);
        break;
    case TSSourceEvent:
        BITCLR(NAT,32, base, Control, Control_tsdbus);
        // div=0
        break;
    case TSSourceDBus4:
        BITSET(NAT,32, base, Control, Control_tsdbus);
        // div=0
        break;
    }
    WRITE32(base, CounterPS, div);
    shadowCounterPS=div;
    shadowSourceTS=src;

    epicsInterruptUnlock(iflags);
}

double
EVRMRM::clockTS() const
{
    TSSource src=SourceTS();

    if(src!=TSSourceInternal)
        return stampClock;

    epicsUInt16 div=tsDiv();

    return clock()/div;
}

void
EVRMRM::clockTSSet(double clk)
{
    if(clk<=0 || !isfinite(clk))
        throw std::out_of_range("TS Clock rate invalid");

    TSSource src=SourceTS();

    if(src==TSSourceInternal){
        double eclk=clock();
        epicsUInt16 div=(epicsUInt16)(eclk/clk);
        WRITE32(base, CounterPS, div);
        shadowCounterPS=div;
    }

    int iflags=epicsInterruptLock();
    stampClock=clk;
    epicsInterruptUnlock(iflags);
}

bool
EVRMRM::interestedInEvent(epicsUInt32 event,bool set)
{
    if (!event || event>255) return false;

    eventCode *entry=&events[event];

    SCOPED_LOCK(events_lock);

    if (   (set  && entry->interested==0) // first interested
        || (!set && entry->interested==1) // or last un-interested
    ) {
        specialSetMap(event, 127, set);
    }

    if (set)
        entry->interested++;
    else
        entry->interested--;

    return true;
}

bool
EVRMRM::getTimeStamp(epicsTimeStamp *ts,epicsUInt32 event)
{
    if(!ts) return false;

    if(event>0 && event<=255) {
        // Get time of last event code #

        eventCode *entry=&events[event];

        SCOPED_LOCK(events_lock);

        // The timestamp service registers permenant interest
        if (!entry->interested ||
            ( entry->last_sec==0 &&
              entry->last_evt==0) )
        {
            return false;
        }

        ts->secPastEpoch=entry->last_sec;
        ts->nsec=entry->last_evt;


    } else {
        // Get current absolute time

        int iflags=epicsInterruptLock();

        epicsUInt32 ctrl=READ32(base, Control);

        // Latch on
        ctrl|= Control_tsltch;
        WRITE32(base, Control, ctrl);

        ts->secPastEpoch=READ32(base, TSSecLatch);
        ts->nsec=READ32(base, TSEvtLatch);

        // Latch off
        ctrl&= ~Control_tsltch;
        WRITE32(base, Control, ctrl);

        epicsInterruptUnlock(iflags);

        //validate seconds (has it been initialized)?
        if(ts->secPastEpoch==0){
            return false;
        }
    }

    //Link seconds counter is POSIX time
    ts->secPastEpoch-=POSIX_TIME_AT_EPICS_EPOCH;

    // Convert ticks to nanoseconds
    double period=1e9/clockTS(); // in nanoseconds

    if(period<=0 || !isfinite(period))
        return false;

    ts->nsec*=(epicsUInt32)period;

    return true;
}

bool
EVRMRM::getTicks(epicsUInt32 *tks)
{
    *tks=READ32(base, TSEvt);
    return true;
}

IOSCANPVT
EVRMRM::eventOccurred(epicsUInt32 event)
{
    if (event>0 && event<=255)
        return events[event].occured;
    else
        return NULL;
}

void
EVRMRM::eventNotityAdd(epicsUInt32 event, CALLBACK* cb)
{
    if (event==0 || event>255)
        throw std::out_of_range("Invalid event number");

    SCOPED_LOCK2(events_lock, guard);

    if (std::find(events[event].notifiees.begin(),
                  events[event].notifiees.end(),
                  cb)
                    != events[event].notifiees.end())
    {
        throw std::runtime_error("callback already registered for this event");
    }

    events[event].notifiees.push_back(cb);

    guard.unlock();

    interestedInEvent(event, true);
}

void
EVRMRM::eventNotityDel(epicsUInt32 event, CALLBACK* cb)
{
    if (event==0 || event>255)
        throw std::out_of_range("Invalid event number");

    SCOPED_LOCK2(events_lock, guard);

    eventCode::notifiees_t::iterator it;

    it=std::find(events[event].notifiees.begin(),
                 events[event].notifiees.end(),
                 cb);
    if (it==events[event].notifiees.end())
        return;

    events[event].notifiees.erase(it);
    guard.unlock();

    interestedInEvent(event, false);
}

epicsUInt16
EVRMRM::dbus() const
{
    return (READ32(base, Status) & Status_dbus_mask) << Status_dbus_shift;
}

void
EVRMRM::enableHeartbeat(bool)
{
}

IOSCANPVT
EVRMRM::heartbeatOccured()
{
    return IRQheadbeat;
}

// A place to write to which will keep the read
// at the end of the ISR from being optimized out.
// This value should never be used anywhere else.
volatile epicsUInt32 evrMrmIsrFlagsTrashCan;

void
EVRMRM::isr(void *arg)
{
    EVRMRM *evr=static_cast<EVRMRM*>(arg);

    epicsUInt32 flags=READ32(evr->base, IRQFlag);

    epicsUInt32 enable=READ32(evr->base, IRQEnable);

    epicsUInt32 active=flags&enable;

    if(!active){
        epicsInterruptContextMessage("ISR does nothing\n");
        return;
    }

    if(active&IRQ_BufFull){
        // Silence interrupt
        BITSET(NAT,32,evr->base, DataBufCtrl, DataBufCtrl_stop);

        callbackRequest(&evr->data_rx_cb);
        scanIoRequest(evr->IRQbufferReady);
    }
    if(active&IRQ_HWMapped){
        evr->count_hardware_irq++;
        scanIoRequest(evr->IRQmappedEvent);
    }
    if(active&IRQ_Event){
        //FIFO not-full
        enable &= ~IRQ_Event;
        callbackRequest(&evr->drain_fifo_cb);
    }
    if(active&IRQ_Heartbeat){
        evr->count_heartbeat++;
        scanIoRequest(evr->IRQheadbeat);
    }
    if(active&IRQ_FIFOFull){
        enable &= ~IRQ_FIFOFull;
        callbackRequest(&evr->drain_fifo_cb);

        scanIoRequest(evr->IRQfifofull);
    }
    if(active&IRQ_RXErr){
        evr->count_recv_error++;
        scanIoRequest(evr->IRQrxError);

        enable &= ~IRQ_RXErr;
        callbackRequest(&evr->poll_link_cb);
    }

    WRITE32(evr->base, IRQEnable, enable|IRQ_Enable);
    WRITE32(evr->base, IRQFlag, flags);
    // Ensure IRQFlags is written before returning.
    evrMrmIsrFlagsTrashCan=READ32(evr->base, IRQFlag);
}

void
EVRMRM::drain_fifo(CALLBACK* cb)
{
    void *vptr;
    callbackGetUser(vptr,cb);
    EVRMRM *evr=static_cast<EVRMRM*>(vptr);

    SCOPED_LOCK2(evr->events_lock, guard);

    epicsUInt32 status;

    // Bound the number of events taken from the FIFO
    // at one time.
    for(size_t i=0; i<512; i++) {

        status=READ32(evr->base, IRQFlag);
        if (!(status&IRQ_Event))
            break;

        epicsUInt32 evt=READ32(evr->base, EvtFIFOCode);
        if (!evt)
            break;

        if (evt>NELEMENTS(evr->events)) {
            epicsUInt32 evt2=READ32(evr->base, EvtFIFOCode);
            if (evt2>NELEMENTS(evr->events)) {
                printf("Really weird event 0x%08x 0x%08x\n", evt, evt2);
                break;
            } else
                evt=evt2;
        }
        evt &= 0xff;

        evr->events[evt].last_sec=READ32(evr->base, EvtFIFOSec);
        evr->events[evt].last_evt=READ32(evr->base, EvtFIFOEvt);

        scanIoRequest(evr->events[evt].occured);

        for(eventCode::notifiees_t::const_iterator it=evr->events[evt].notifiees.begin();
            it!=evr->events[evt].notifiees.begin();
            ++it)
        {
            callbackRequest(*it);
        }
    }

    int iflags=epicsInterruptLock();

    if (status&Status_fifostop) {
        BITSET(NAT,32, evr->base, Control, Control_fiforst);
    }

    BITSET(NAT,32, evr->base, IRQEnable, IRQ_Event|IRQ_BufFull);

    epicsInterruptUnlock(iflags);
}

void
EVRMRM::drain_log(CALLBACK*)
{
}

void
EVRMRM::poll_link(CALLBACK* cb)
{
    void *vptr;
    callbackGetUser(vptr,cb);
    EVRMRM *evr=static_cast<EVRMRM*>(vptr);

    epicsUInt32 flags=READ32(evr->base, IRQFlag);

    if(flags&IRQ_RXErr){
        // Still down
        callbackRequestDelayed(&evr->poll_link_cb, 0.1); // poll again in 100ms
        WRITE32(evr->base, IRQFlag, IRQ_RXErr);
    }else{
        scanIoRequest(evr->IRQrxError);
        int iflags=epicsInterruptLock();
        BITSET(NAT,32, evr->base, IRQEnable, IRQ_RXErr);
        epicsInterruptUnlock(iflags);
    }
}
