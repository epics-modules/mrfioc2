
#include "drvem.h"

#include <cstdio>
#include <stdexcept>

#include <math.h>

#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "evrRegMap.h"

#include "mrfFracSynth.h"

#include "drvemIocsh.h"

#include <dbScan.h>
#include <epicsInterrupt.h>

#define READ16 NAT_READ16
#define READ32 NAT_READ32
#define WRITE16 NAT_WRITE16
#define WRITE32 NAT_WRITE32

#define DBG evrmrmVerb

static
const double fracref=24.0; // MHz

EVRMRM::EVRMRM(int i,volatile unsigned char* b)
  :EVR()
  ,id(i)
  ,base(b)
  ,stampClock(1.0/0.0)
  ,count_recv_error(0)
  ,count_hardware_irq(0)
  ,count_heartbeat(0)
  ,outputs()
  ,prescalers()
  ,pulsers()
{
    epicsUInt32 v = READ32(base, FWVersion),evr;

    evr=v&FWVersion_type_mask;
    evr>>=FWVersion_type_shift;

    if(evr!=0x1)
        throw std::runtime_error("Address does not correspond to an EVR");

    scanIoInit(&IRQmappedEvent);
    scanIoInit(&IRQbufferReady);
    scanIoInit(&IRQheadbeat);
    scanIoInit(&IRQrxError);

    /*
     * Create subunit instances
     */

    v&=FWVersion_form_mask;
    v>>=FWVersion_form_shift;

    size_t nPul=10;
    size_t nPS=3;
    size_t nOFP=0, nOFPUV=0, nORB=0;
    size_t nIFP=0;

    switch(v){
    case evrFormCPCI:
        if(DBG) printf("CPCI ");
        break;
    case evrFormPMC:
        if(DBG) printf("PMC ");
        nOFP=4;
        nIFP=2;
        break;
    case evrFormVME64:
        if(DBG) printf("VME64 ");
        nOFP=4;
        nOFPUV=2;
        nORB=16;
        break;
    }
    if(DBG) printf("Out FP:%u FPUNIV:%u RB:%u IFP:%u\n",nOFP,nOFPUV,nORB,nIFP);

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
}

EVRMRM::~EVRMRM()
{
    for(outputs_t::iterator it=outputs.begin();
        it!=outputs.end(); ++it)
    {
        delete &(*it);
    }
    outputs.clear();
    for(prescalers_t::iterator it=prescalers.begin();
        it!=prescalers.end(); ++it)
    {
        delete &(*it);
    }
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
    if(v)
        BITSET(NAT,32,base, Control, Control_enable);
    else
        BITCLR(NAT,32,base, Control, Control_enable);
}

MRMPulser*
EVRMRM::pulser(epicsUInt32 i)
{
    if(i>=pulsers.size())
        throw std::range_error("Pulser id is out of range");
    return pulsers[i];
}

const MRMPulser*
EVRMRM::pulser(epicsUInt32 i) const
{
    if(i>=pulsers.size())
        throw std::range_error("Pulser id is out of range");
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
        throw std::range_error("Input id is out of range");
    return inputs[i];
}

const MRMInput*
EVRMRM::input(epicsUInt32 i) const
{
    if(i>=inputs.size())
        throw std::range_error("Input id is out of range");
    return inputs[i];
}

MRMPreScaler*
EVRMRM::prescaler(epicsUInt32 i)
{
    if(i>=prescalers.size())
        throw std::range_error("PreScaler id is out of range");
    return prescalers[i];
}

const MRMPreScaler*
EVRMRM::prescaler(epicsUInt32 i) const
{
    if(i>=prescalers.size())
        throw std::range_error("PreScaler id is out of range");
    return prescalers[i];
}

bool
EVRMRM::specialMapped(epicsUInt32 code, epicsUInt32 func) const
{
    if(code>255)
        throw std::range_error("Event code is out of range");
    if(func>127 || func<96 ||
        (func<=121 && func>=102) )
    {
        throw std::range_error("Special function code is out of range");
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
        throw std::range_error("Event code is out of range");
    if(func>127 || func<96 ||
        (func<=121 && func>=102) )
    {
        throw std::range_error("Special function code is out of range");
    }

    if(code==0)
      return;

    epicsUInt32 bit  =func%32;
    epicsUInt32 mask=1<<bit;

    if(v)
        BITSET(NAT,32,base, MappingRam(0, code, Internal), mask);
    else
        BITCLR(NAT,32,base, MappingRam(0, code, Internal), mask);
}

const char*
EVRMRM::idName(epicsUInt32 src) const
{
    // Special Mappings
    switch(src){
        case 127: return "Save FIFO";
        case 126: return "Latch Timestamp";
        case 125: return "Blink LED";
        case 124: return "Forward Event";
        case 123: return "Stop Event Log";
        case 122: return "Log Event";
        // 102 -> 121 are reserved
        case 101: return "Heartbeat";
        case 100: return "Reset Prescalers (dividers)";
        case 99:  return "Timestamp reset";
        case 98:  return "Timestamp clock";
        case 97:  return "Seconds shift 1";
        case 96:  return "Seconds shift 0";
        // 74 -> 95 are reserved
        case 73:  return "Trigger pulser #9";
        case 72:  return "Trigger pulser #8";
        case 71:  return "Trigger pulser #7";
        case 70:  return "Trigger pulser #6";
        case 69:  return "Trigger pulser #5";
        case 68:  return "Trigger pulser #4";
        case 67:  return "Trigger pulser #3";
        case 66:  return "Trigger pulser #2";
        case 65:  return "Trigger pulser #1";
        case 64:  return "Trigger pulser #0";
        // 42 -> 63 are reserved
        case 41:  return "Set pulser #9";
        case 40:  return "Set pulser #8";
        case 39:  return "Set pulser #7";
        case 38:  return "Set pulser #6";
        case 37:  return "Set pulser #5";
        case 36:  return "Set pulser #4";
        case 35:  return "Set pulser #3";
        case 34:  return "Set pulser #2";
        case 33:  return "Set pulser #1";
        case 32:  return "Set pulser #0";
        // 10 -> 31 are reserved
        case 9:   return "Reset pulser #9";
        case 8:   return "Reset pulser #8";
        case 7:   return "Reset pulser #7";
        case 6:   return "Reset pulser #6";
        case 5:   return "Reset pulser #5";
        case 4:   return "Reset pulser #4";
        case 3:   return "Reset pulser #3";
        case 2:   return "Reset pulser #2";
        case 1:   return "Reset pulser #1";
        case 0:   return "Reset pulser #0";

        default:  return "Invalid";
    }
}

double
EVRMRM::clock() const
{
    return FracSynthAnalyze(READ32(base, FracDiv),
                            fracref,0)*1e6;
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
        throw std::range_error("New frequency can't be used");

    epicsUInt32 oldfrac=READ32(base, FracDiv);

    if(newfrac!=oldfrac){
        // Changing the control word disturbes the phase
        // of the synthesiser which will cause a glitch.
        // Don't change the control word unless needed.

        WRITE32(base, FracDiv, newfrac);
    }

    // USecDiv is accessed as a 32 bit register, but
    // only 16 are used.
    epicsUInt16 oldudiv=READ32(base, USecDiv);
    epicsUInt16 newudiv=(epicsUInt16)freq;

    if(newudiv!=oldudiv){
        WRITE32(base, USecDiv, newudiv);
    }
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

epicsUInt32
EVRMRM::tsDiv() const
{
    return READ32(base, CounterPS);
}

void
EVRMRM::setSourceTS(TSSource src)
{
    double clk=clockTS(), eclk;
    epicsUInt16 div=0;

    if(clk<=0 || !isfinite(clk))
        throw std::range_error("TS Clock rate invalid");

    switch(src){
    case TSSourceInternal:
        eclk=clock();
        div=eclk/clk;
        break;
    case TSSourceEvent:
        BITCLR(NAT,32, base, Control, Control_tsdbus);
        break;
    case TSSourceDBus4:
        BITSET(NAT,32, base, Control, Control_tsdbus);
        break;
    default:
        throw std::range_error("TS source invalid");
    }
    WRITE32(base, CounterPS, div);
}

TSSource
EVRMRM::SourceTS() const
{
    epicsUInt32 tdiv=tsDiv();

    if(tdiv!=0)
        return TSSourceInternal;

    bool usedbus4=READ32(base, Control) & Control_tsdbus;

    if(usedbus4)
        return TSSourceDBus4;
    else
        return TSSourceEvent;
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
        throw std::range_error("TS Clock rate invalid");

    TSSource src=SourceTS();

    if(src==TSSourceInternal){
        double eclk=clock();
        epicsUInt16 div=eclk/clk;
        WRITE32(base, CounterPS, div);
    }

    stampClock=clk;
}

bool
EVRMRM::getTimeStamp(epicsTimeStamp *ts,TSMode mode)
{
    if(!ts) return false;

    switch(mode){
    case TSModeLatch:
        ts->secPastEpoch=READ32(base, TSSecLatch);
        ts->nsec=READ32(base, TSEvtLatch);
        break;
    case TSModeFree:
        ts->secPastEpoch=READ32(base, TSSec);
        ts->nsec=READ32(base, TSEvt);
        break;
    default:
        throw std::range_error("TS mode invalid");
    }

    //validate seconds (has it been initialized)?
    if(ts->secPastEpoch==0){
        return false;
    }

    //Link seconds counter is POSIX time
    ts->secPastEpoch-=POSIX_TIME_AT_EPICS_EPOCH;

    // Convert ticks to nanoseconds
    double period=1e9/clockTS(); // in nanoseconds

    if(period<=0 || !isfinite(period))
        return false;

    ts->nsec*=period;

    return true;
}

void
EVRMRM::tsLatch(bool latch)
{
    if(latch)
        BITSET(NAT,32,base, Control, Control_tsltch);
    else
        BITSET(NAT,32,base, Control, Control_tsrst);
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

void
EVRMRM::isr(void *arg)
{
    EVRMRM *evr=static_cast<EVRMRM*>(arg);

    epicsUInt32 flags=READ32(evr->base, IRQFlag);

    //TODO: Locking...
    epicsInterruptContextMessage("IRQ\n");

    if(flags&IRQ_BufFull){
        scanIoRequest(evr->IRQbufferReady);
    }
    if(flags&IRQ_HWMapped){
        evr->count_hardware_irq++;
        scanIoRequest(evr->IRQmappedEvent);
    }
    if(flags&IRQ_Event){
        //What is this?
    }
    if(flags&IRQ_Heartbeat){
        evr->count_heartbeat++;
        scanIoRequest(evr->IRQheadbeat);
    }
    if(flags&IRQ_FIFOFull){
    }
    if(flags&IRQ_RXErr){
        evr->count_recv_error++;
        scanIoRequest(evr->IRQrxError);
    }

    WRITE32(evr->base, IRQFlag, flags);
}
