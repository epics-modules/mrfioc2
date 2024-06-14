/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* Copyright (c) 2022 Cosylab d.d.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <cstdio>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <sstream>

#include <epicsMath.h>
#include <errlog.h>
#include <epicsMath.h>
#include <dbDefs.h>
#include <dbScan.h>
#include <epicsInterrupt.h>

#include "mrmDataBufTx.h"
#include "sfp.h"

#include "evrRegMap.h"

#include "mrfFracSynth.h"

#include <mrfCommon.h>
#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "drvemIocsh.h"

#include <evr/evr.h>
#include <evr/pulser.h>
#include <evr/cml.h>
#include <evr/prescaler.h>
#include <evr/input.h>
#include <evr/delay.h>

#if defined(__linux__) || defined(_WIN32)
#  include "devLibPCI.h"
#endif

#include "drvem.h"

#include <epicsExport.h>

/* whether to use features introduced to support
 * parallel handling of shared callback queues.
 */
#if EPICS_VERSION_INT>=VERSION_INT(3,15,0,2)
#  define HAVE_PARALLEL_CB
#endif

int evrMrmTimeDebug;
int evrMrmSeqRxDebug;
//! value in nanoseconds above which a timestamp is considered invalid.
//! below is truncated.  May be necessary when simulating timestamp
//! source in software
int evrMrmTimeNSOverflowThreshold;
extern "C" {
 epicsExportAddress(int, evrMrmSeqRxDebug);
 epicsExportAddress(int, evrMrmTimeDebug);
 epicsExportAddress(int, evrMrmTimeNSOverflowThreshold);
}

using namespace std;

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
 */

extern "C" {
    /* Arbitrary throttleing of FIFO thread.
     * The FIFO thread has to run at a high priority
     * so the callbacks have low latency.  At the same
     * time we want to prevent starvation of lower
     * priority tasks if too many events are received.
     * This would cause the CA server to be starved
     * preventing remote correction of the problem.
     *
     * This should be less than the time between
     * the highest frequency event needed for
     * database processing.
     *
     * Note that the actual rate will be limited by the
     * time needed for database processing.
     *
     * Set to 0.0 to disable
     *
     * No point in making this shorter than the system tick
     */
    double mrmEvrFIFOPeriod = 1.0/ 1000.0; /* 1/rate in Hz */

    epicsExportAddress(double,mrmEvrFIFOPeriod);
}

/* Number of good updates before the time is considered valid */
#define TSValidThreshold 5

/* GTX output offset [FPUniv] */
#define GTX_FPUV_OFFSET 16
/* GTX SFP output offset [on MTCA RF card]*/
#define GTX_SFP_OFFSET 20

// Fractional synthesizer reference clock frequency
static
const double fracref=24.0; // MHz

EVRMRM::EVRMRM(const std::string& n,
               bus_configuration& busConfig, const Config *c,
               volatile unsigned char* b,
               epicsUInt32 bl)
  :base_t(n,busConfig)
  ,MRMSPI(b+U32_SPIDData)
  ,TimeStampSource(1.0)
  ,evrLock()
  ,conf(c)
  ,base(b)
  ,baselen(bl)
  ,buftx(n+":BUFTX", b+U32_DataTxCtrl, b+U32_DataTx_base)
  ,bufrx(n+":BUFRX", b, 10) // Sets depth of Rx queue
  ,count_recv_error(0)
  ,count_hardware_irq(0)
  ,count_heartbeat(0)
  ,shadowIRQEna(0)
  ,count_FIFO_overflow(0)
  ,outputs()
  ,prescalers()
  ,pulsers()
  ,shortcmls()
  ,gpio_(*this)
  ,drain_fifo_method(*this)
  ,drain_fifo_task(drain_fifo_method, "EVRFIFO",
                   epicsThreadGetStackSize(epicsThreadStackBig),
                   epicsThreadPriorityHigh )
  // 3 because 2 IRQ events, and 1 shutdown event
  ,drain_fifo_wakeup(3,sizeof(int))
  ,count_FIFO_sw_overrate(0)
  ,timeSrcMode(Disable)
  ,stampClock(0.0)
  ,shadowSourceTS(TSSourceInternal)
  ,shadowCounterPS(0)
  ,timestampValid(0)
  ,lastInvalidTimestamp(0)
  ,lastValidTimestamp(0)
{
try{
    const epicsUInt32 rawver = fpgaFirmware();
    const epicsUInt32 boardtype = (rawver&FWVersion_type_mask)>>FWVersion_type_shift;
    const epicsUInt32 formfactor = (rawver&FWVersion_form_mask)>>FWVersion_form_shift;
    const MRFVersion ver(rawver);

    if(boardtype!=0x1)
        throw std::runtime_error("Address does not correspond to an EVR");

    if(ver<MRFVersion(0,3))
        throw std::runtime_error("Firmware 0 version < 3 not supported");
    else if(ver.firmware()==2 && ver<MRFVersion(2,7))
        throw std::runtime_error("Firmware 2 version < 207 not supported");

    if(ver.firmware()==2 && ver<MRFVersion(2,7,6))
        printf("Warning: Recommended minimum firmware 2 version is 207.6, found %s\n", ver.str().c_str());

    if(ver.firmware()!=0 && ver.firmware()!=2)
        printf("Warning: Unknown firmware series %u.  Your milage may vary\n", ver.firmware());

    scanIoInit(&IRQmappedEvent);
    scanIoInit(&IRQheartbeat);
    scanIoInit(&IRQrxError);
    scanIoInit(&IRQfifofull);
    scanIoInit(&timestampValidChange);

    CBINIT(&data_rx_cb   , priorityHigh, &mrmBufRx::drainbuf, &this->bufrx);
    CBINIT(&poll_link_cb , priorityMedium, &EVRMRM::poll_link , this);

    if(ver>=MRFVersion(0, 5)) {
        sfp.reset(new SFP(SB() << n << ":SFP", base + U32_SFPEEPROM_base));
    }

    if(ver>=MRFVersion(2,7)) {
        printf("Sequencer capability detected\n");
        seq.reset(new EvrSeqManager(this));
    }

    /*
     * Create subunit instances
     */

    formFactor form = getFormFactor();

    printf("%s: ", formFactorStr().c_str());

    std::cout << " Model " << conf->model
              << " Pul " << conf->nPul
              << " PS " << conf->nPS
              << " OFP " << conf->nOFP
              << " OFPUNIV " << conf->nOFPUV
              << " ORB " << conf->nORB
              << " OBack " << conf->nOBack
              << " OFPDly " << conf->nOFPDly
              << " CML " << conf->nCML
              << " Kind " << static_cast<int>(conf->kind)
              << " IFP " << conf->nIFP
              << std::endl;

    inputs.resize(conf->nIFP);
    for(size_t i=0; i<conf->nIFP; i++){
        inputs[i] = new MRMInput(SB() << n << ":FPIn" << i, base, i);
    }

    // Special output for mapping bus interrupt
    outputs[std::make_pair(OutputInt,0)]=new MRMOutput(n+":Int", this, OutputInt, 0);

    for(unsigned int i=0; i<conf->nOFP; i++){
        outputs[std::make_pair(OutputFP, i)] = new MRMOutput(SB() << n << ":FrontOut" << i, this, OutputFP, i);
    }

    for(unsigned int i=0; i<conf->nOFPUV; i++){
        outputs[std::make_pair(OutputFPUniv, i)] = new MRMOutput(SB() << n << ":FrontUnivOut" << i, this, OutputFPUniv, i);
    }

    delays.resize(conf->nOFPDly);
    for(unsigned int i=0; i<conf->nOFPDly; i++){
        delays[i] = new DelayModule(SB() << n << ":UnivDlyModule" << i, this, i);
    }

    for(unsigned int i=0; i<conf->nORB; i++){
        outputs[std::make_pair(OutputRB, i)] = new MRMOutput(SB() << n << ":RearUniv" << i, this, OutputRB, i);
    }

    for(unsigned int i=0; i<conf->nOBack; i++){
        outputs[std::make_pair(OutputRB, i)] = new MRMOutput(SB() << n << ":Backplane" << i, this, OutputBackplane, i);
    }

    prescalers.resize(conf->nPS);
    for(size_t i=0; i<conf->nPS; i++){
        prescalers[i] = new MRMPreScaler(SB() << n << ":PS" << i, *this, base + U32_Scaler(i));
    }

    pulsers.resize(32);
    for(epicsUInt32 i=0; i<conf->nPul; i++){
        pulsers[i] = new MRMPulser(SB() << n << ":Pul" << i, i, *this);
    }
    if(ver>=MRFVersion(2,0)) {
        // masking pulsers
        for(epicsUInt32 i=28; i<=31; i++){
            pulsers[i] = new MRMPulser(SB() << n << ":Pul" << i, i, *this);
        }

    }

    if(formfactor==formFactor_CPCIFULL) {
        for(unsigned int i=4; i<8; i++) {
            outputs[std::make_pair(OutputFP, i)] = new MRMOutput(SB() << n << ":FrontOut" << i, this, OutputFP, i);
        }
        shortcmls.resize(8, 0);
        shortcmls[4]=new MRMCML(n+":CML4", 4,*this,MRMCML::typeCML,form);
        shortcmls[5]=new MRMCML(n+":CML5", 5,*this,MRMCML::typeCML,form);
        shortcmls[6]=new MRMCML(n+":CML6", 6,*this,MRMCML::typeTG300,form);
        shortcmls[7]=new MRMCML(n+":CML7", 7,*this,MRMCML::typeTG300,form);

    } else if(formfactor==formFactor_mTCA) {
        int u_idx;
        if (conf->nCML > 0)
            shortcmls.resize(conf->nCML);
        for (size_t i = 0; i < conf->nCML; i++)
        {
            u_idx = GTX_FPUV_OFFSET + i;
            // map GTX outputs (e.g. TCLKA/B) starting from GTX_FPUV_OFFSET (as in the documentation)
            outputs[std::make_pair(OutputFPUniv, u_idx)]
                = new MRMOutput(SB() << n << ":FrontUnivOut" << u_idx, this, OutputFPUniv, u_idx);
            // create CMLs
            if(model().compare("mTCA-EVR-300RF") == 0 && u_idx == GTX_SFP_OFFSET)
                shortcmls[i] = new MRMCML(SB() << n << ":CML" << i, i, *this, MRMCML::typeTG300, form);
            else
                shortcmls[i] = new MRMCML(SB() << n << ":CML" << i, i, *this, conf->kind, form);
        }
    } else if(conf->nCML && ver>=MRFVersion(0,4)){
        shortcmls.resize(conf->nCML);
        for(size_t i=0; i<conf->nCML; i++){
            shortcmls[i] = new MRMCML(SB() << n << ":CML" << i, (unsigned char)i, *this, conf->kind, form);
        }

    }else if(conf->nCML){
        printf("CML outputs not supported with this firmware\n");
    }

    for(epicsUInt32 i=0; i<NELEMENTS(this->events); i++) {
        events[i].code=i;
        events[i].owner=this;
        CBINIT(&events[i].done_cb, priorityLow, &EVRMRM::sentinel_done , &events[i]);
    }

    SCOPED_LOCK(evrLock);

    memset(_mapped, 0, sizeof(_mapped));
    // restore mapping ram to a clean state
    // needed when the IOC is started w/o a device reset (ie Linux)
    //TODO: find a way to do this that doesn't require clearing
    //      mapping which will shortly be set again...
    for(size_t i=0; i<255; i++) {
        WRITE32(base, MappingRam(0, i, Internal), 0);
        WRITE32(base, MappingRam(0, i, Trigger), 0);
        WRITE32(base, MappingRam(0, i, Set), 0);
        WRITE32(base, MappingRam(0, i, Reset), 0);
    }

    // restore default special mappings
    // These may be replaced later
    specialSetMap(MRF_EVENT_TS_SHIFT_0,     96, true);
    specialSetMap(MRF_EVENT_TS_SHIFT_1,     97, true);
    specialSetMap(MRF_EVENT_TS_COUNTER_INC, 98, true);
    specialSetMap(MRF_EVENT_TS_COUNTER_RST, 99, true);
    specialSetMap(MRF_EVENT_HEARTBEAT,      101, true);

    // Except for Prescaler reset, which is set with a record
    specialSetMap(MRF_EVENT_RST_PRESCALERS, 100, false);

    eventClock=FracSynthAnalyze(READ32(base, FracDiv),
                                fracref,0)*1e6;

    shadowCounterPS=READ32(base, CounterPS);

    if(tsDiv()!=0) {
        shadowSourceTS=TSSourceInternal;
    } else {
        bool usedbus4=(READ32(base, Control) & Control_tsdbus) != 0;

        if(usedbus4)
            shadowSourceTS=TSSourceDBus4;
        else
            shadowSourceTS=TSSourceEvent;
    }

    eventNotifyAdd(MRF_EVENT_TS_COUNTER_RST, &seconds_tick, (void*)this);

    drain_fifo_task.start();

    if(busConfig.busType==busType_pci || (busConfig.busType==busType_vme && version()>=MRFVersion(2, 0, 0)))
        mrf::SPIDevice::registerDev(n+":FLASH", mrf::SPIDevice(this, 1));

    #ifndef DBR_UTAG
    std::cout << "WARNING EVRMRM::EVRMRM epicsUTag not supported." << std::endl;
    #endif

} catch (std::exception& e) {
    printf("Aborting EVR initializtion: %s\n", e.what());
    cleanup();
    throw;
}
}

EVRMRM::~EVRMRM()
{
    if(getBusConfiguration()->busType==busType_pci || (getBusConfiguration()->busType==busType_vme && version()>=MRFVersion(2, 0, 0)))
        mrf::SPIDevice::unregisterDev(name()+":FLASH");
    cleanup();
}

void
EVRMRM::cleanup()
{
    printf("%s shuting down... ", name().c_str());
    int wakeup=1;
    drain_fifo_wakeup.send(&wakeup, sizeof(wakeup));
    drain_fifo_task.exitWait();

    for(outputs_t::iterator it=outputs.begin();
        it!=outputs.end(); ++it)
    {
        delete it->second;
    }
    outputs.clear();
#define CLEANVEC(TYPE, VAR) \
    for(TYPE::iterator it=VAR.begin(); it!=VAR.end(); ++it) \
    { delete (*it); } \
    VAR.clear();

    CLEANVEC(inputs_t, inputs);
    CLEANVEC(prescalers_t, prescalers);
    CLEANVEC(pulsers_t, pulsers);
    CLEANVEC(shortcmls_t, shortcmls);

#undef CLEANVEC
    printf("complete\n");
}

string EVRMRM::model() const
{
    return conf->model;
}

epicsUInt32
EVRMRM::fpgaFirmware(){
    return READ32(base, FWVersion);
}

MRFVersion EVRMRM::version() const
{
    return MRFVersion(READ32(base, FWVersion));
}

formFactor
EVRMRM::getFormFactor(){
    epicsUInt32 v = READ32(base, FWVersion);
    epicsUInt32 form = (v&FWVersion_form_mask)>>FWVersion_form_shift;

    if(form <= formFactor_mTCA) return (formFactor)form;
    else return formFactor_unknown;
}

std::string
EVRMRM::formFactorStr(){
    std::string text;
    formFactor form;

    form = getFormFactor();
    switch(form){
    case formFactor_CPCI:
        text = "CompactPCI 3U";
        break;

    case formFactor_CPCIFULL:
        text = "CompactPCI 6U";
        break;

    case formFactor_CRIO:
        text = "CompactRIO";
        break;

    case formFactor_PCIe:
        text = "PCIe";
        break;

    case formFactor_mTCA:
        text = "mTCA";
        break;

    case formFactor_PXIe:
        text = "PXIe";
        break;

    case formFactor_PMC:
        text = "PMC";
        break;

    case formFactor_VME64:
        text = "VME 64";
        break;

    default:
        text = "Unknown form factor";
    }

    return text;
}

bool
EVRMRM::enabled() const
{
    epicsUInt32 v = READ32(base, Control);
    return (v&Control_enable) != 0;
}

void
EVRMRM::enable(bool v)
{
    SCOPED_LOCK(evrLock);
    if(v)
        BITSET(NAT,32,base, Control, Control_enable|Control_mapena|Control_outena|Control_evtfwd);
    else
        BITCLR(NAT,32,base, Control, Control_enable|Control_mapena|Control_outena|Control_evtfwd);
}

bool
EVRMRM::mappedOutputState() const
{
    NAT_WRITE32(base, IRQFlag, IRQ_HWMapped);
    return NAT_READ32(base, IRQFlag) & IRQ_HWMapped;
}

MRMGpio*
EVRMRM::gpio(){
    return &gpio_;
}

bool
EVRMRM::specialMapped(epicsUInt32 code, epicsUInt32 func) const
{
    if(code>255)
        throw std::out_of_range("Event code is out of range (0-255)");
    if(func>127 || func<96 ||
        (func<=121 && func>=102) )
    {
        throw std::out_of_range("Special function code is out of range. Valid ranges: 96-101 and 122-127");
    }

    if(code==0)
        return false;

    SCOPED_LOCK(evrLock);

    return _ismap(code,func-96);
}

void
EVRMRM::specialSetMap(epicsUInt32 code, epicsUInt32 func,bool v)
{
    if(code>255)
        throw std::out_of_range("Event code is out of range");
    /* The special function codes are the range 96 to 127, excluding 102 to 121
     */
    if(func>127 || func<96 ||
        (func<=121 && func>=102) )
    {
        errlogPrintf("EVR %s code %02x func %3d out of range. Code range is 0-255, where function rangs are 96-101 and 122-127\n",
            name().c_str(), code, func);
        throw std::out_of_range("Special function code is out of range.  Valid ranges: 96-101 and 122-127");
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

    SCOPED_LOCK(evrLock);

    epicsUInt32 val=READ32(base, MappingRam(0, code, Internal));

    if (v == _ismap(code,func-96)) {
        // mapping already set defined

    } else if(v) {
        _map(code,func-96);
        WRITE32(base, MappingRam(0, code, Internal), val|mask);
    } else {
        _unmap(code,func-96);
        WRITE32(base, MappingRam(0, code, Internal), val&~mask);
    }
}

// Set both the fractional synthesiser and microsecond divider.
void EVRMRM::clockSet(double freq)
{
    double err;
    double clk_soft = freq; //[Hz]

    freq/=1e6;

    epicsUInt32 newfrac=FracSynthControlWord(freq, fracref, 0, &err);

    if(newfrac==0)
        throw std::out_of_range("New frequency can't be used");

    SCOPED_LOCK(evrLock);

    epicsUInt32 oldfrac=READ32(base, FracDiv);

    if(newfrac!=oldfrac){
        // Changing the control word disturbes the phase
        // of the synthesiser which will cause a glitch.
        // Don't change the control word unless needed.

        WRITE32(base, FracDiv, newfrac);

        double clk = FracSynthAnalyze(READ32(base, FracDiv), fracref,0) * 1e6;
        // Apply the soft clock if the registry is not implemented (EVRD/U)
        eventClock = (clk == 0.0) ? clk_soft : clk;
    }

    printf("Set EVR %s %s clock %f newfrac %d (oldfrac %d)\n", model().c_str(), name().c_str(), eventClock, newfrac, oldfrac);

    // USecDiv is accessed as a 32 bit register, but
    // only 16 are used.
    epicsUInt16 oldudiv=READ32(base, USecDiv);
    epicsUInt16 newudiv=(epicsUInt16)freq;

    if(newudiv!=oldudiv){
        WRITE32(base, USecDiv, newudiv);
    }
}

epicsUInt16
EVRMRM::clockMode() const
{
    return (READ32(base, ClkCtrl)&ClkCtrl_clkmd_MASK)>>ClkCtrl_clkmd_SHIFT;
}

void
EVRMRM::clockModeSet(epicsUInt16 mode)
{
    epicsUInt32 cur = READ32(base, ClkCtrl);
    cur &= ~ClkCtrl_clkmd_MASK;
    cur |= (epicsUInt32(mode)<<ClkCtrl_clkmd_SHIFT)&ClkCtrl_clkmd_MASK;
    WRITE32(base, ClkCtrl, cur);
}

epicsUInt32
EVRMRM::uSecDiv() const
{
    return READ32(base, USecDiv);
}

bool
EVRMRM::extInhib() const
{
    epicsUInt32 v = READ32(base, Control);
    return (v&Control_GTXio) != 0;
}

void
EVRMRM::setExtInhib(bool v)
{
    SCOPED_LOCK(evrLock);
    if(v)
        BITSET(NAT,32,base, Control, Control_GTXio);
    else
        BITCLR(NAT,32,base, Control, Control_GTXio);
}

bool
EVRMRM::pllLocked() const
{
    epicsUInt32 cur = READ32(base, ClkCtrl);
    epicsUInt32 mask = ClkCtrl_cglock;
    if(version()>=MRFVersion(2, 7, 0))
        mask |= ClkCtrl_plllock;
    return (cur&mask)==mask;
}

void
EVRMRM::setPLLBandwidth(epicsUInt16 pllBandwidth)
{
    if(pllBandwidth > PLLBandwidth_MAX) {
        throw std::out_of_range("PLL bandwidth selected is not available.");
    }

    epicsUInt32 temp = READ32(base, ClkCtrl);
    temp &= ~ClkCtrl_pllbw;
    temp |= (epicsUInt8)pllBandwidth << ClkCtrl_pllbw_SHIFT;
    WRITE32(base, ClkCtrl, temp);
}

epicsUInt16
EVRMRM::getPLLBandwidth() const
{
    epicsUInt32 temp = (READ32(base, ClkCtrl) & ClkCtrl_pllbw);
    temp = temp >> ClkCtrl_pllbw_SHIFT;

    return (epicsUInt16) temp;
}

bool
EVRMRM::linkStatus() const
{
    return !(READ32(base, Status) & Status_legvio);
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

    SCOPED_LOCK(evrLock);

    switch(src){
    case TSSourceInternal:
        // div!=0 selects src internal
        div=roundToUInt(eclk/clk, 0xffff);
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
}

double
EVRMRM::clockTS() const
{
    //Note: acquires evrLock 3 times.

    TSSource src=SourceTS();
    double eclk=clock();

    if( (src!=TSSourceInternal) ||
       ((src==TSSourceInternal) && (stampClock>eclk)))
        return stampClock;

    epicsUInt16 div=tsDiv();

    return eclk/div;
}

void
EVRMRM::clockTSSet(double clk)
{
    if(clk<0.0 || !isfinite(clk))
        throw std::out_of_range("TS Clock rate invalid");

    TSSource src=SourceTS();
    double eclk=clock();

    /* There is an issue with this and the embedded EVRs of the mTCA EVM vers
     * 280b0207. The register holding the fractional synthesizer configuration
     * word is empty, so eclk always resolves to 0, and so does clk. This messes
     * up with the delay generators width/delay settings. By removing this check
     * clockTS() can be used for the calculation of the width/delay.
     * if(clk>eclk*1.01 || clk==0.0)
     */
    if(clk==0.0)
        clk=eclk;

    SCOPED_LOCK(evrLock);

    if(src==TSSourceInternal){
        epicsUInt16 div=roundToUInt(eclk/clk, 0xffff);
        WRITE32(base, CounterPS, div);

        shadowCounterPS=div;
    }

    stampClock=clk;
}

bool
EVRMRM::interestedInEvent(epicsUInt32 event,bool set)
{
    if (!event || event>255) return false;

    eventCode *entry=&events[event];

    SCOPED_LOCK(evrLock);

    if (   (set  && entry->interested==0) // first interested
        || (!set && entry->interested==1) // or last un-interested
    ) {
        specialSetMap(event, ActionFIFOSave, set);
    }

    if (set)
        entry->interested++;
    else
        entry->interested--;

    return true;
}

bool
EVRMRM::TimeStampValid() const
{
    SCOPED_LOCK(evrLock);
    return timestampValid>=TSValidThreshold;
}

bool
EVRMRM::getTimeStamp(epicsTimeStamp *ret,epicsUInt32 event)
{
    if(!ret) throw std::runtime_error("Invalid argument");
    epicsTimeStamp ts;

    SCOPED_LOCK(evrLock);
    if(timestampValid<TSValidThreshold) return false;

    if(event>0 && event<=255) {
        // Get time of last event code #

        eventCode *entry=&events[event];

        // Fail if event is not mapped
        if (!entry->interested ||
            ( entry->last_sec==0 &&
              entry->last_evt==0) )
        {
            return false;
        }

        ts.secPastEpoch=entry->last_sec;
        ts.nsec=entry->last_evt;


    } else {
        // Get current absolute time

        epicsUInt32 ctrl=READ32(base, Control);

        // Latch timestamp
        WRITE32(base, Control, ctrl|Control_tsltch);

        ts.secPastEpoch=READ32(base, TSSecLatch);
        ts.nsec=READ32(base, TSEvtLatch);

        /* BUG: There was a firmware bug which occasionally
         * causes the previous write to fail with a VME bus
         * error, and 0 the Control register.
         *
         * This issues has been fixed in VME firmwares EVRv 5
         * pre2 and EVG v3 pre2.  Feb 2011
         */
        epicsUInt32 ctrl2=READ32(base, Control);
        if (ctrl2!=ctrl) { // tsltch bit is write-only
            printf("Get timestamp: control register write fault. Written: %08x, readback: %08x\n",ctrl,ctrl2);
            WRITE32(base, Control, ctrl);
        }

    }

    if(!convertTS(&ts))
        return false;

    *ret = ts;
    return true;
}

/** @brief In place conversion between raw posix sec+ticks to EPICS sec+nsec.
 @returns false if conversion failed
 */
bool
EVRMRM::convertTS(epicsTimeStamp* ts)
{
    // First validate the input

    //Has it been initialized?
    if(ts->secPastEpoch==0 || ts->nsec==0){
        return false;
    }

    // recurrence of an invalid time
    if(ts->secPastEpoch==lastInvalidTimestamp) {
        timestampValid=0;
        scanIoRequest(timestampValidChange);
        if(evrMrmTimeDebug>0)
            errlogPrintf("TS convert repeats known bad value new %08x bad %08x\n",
                         (unsigned)ts->secPastEpoch, (unsigned)lastInvalidTimestamp);
        return false;
    }

    /* Reported seconds timestamp should be no more
     * then 1sec in the future.
     * reporting values in the past should be caught by
     * generalTime
     */
    if(ts->secPastEpoch > lastValidTimestamp+1)
    {
        errlogPrintf("EVR %s %s ignoring invalid TS %08x %08x (expect %08x)\n",
                    model().c_str(), name().c_str(), ts->secPastEpoch, ts->nsec, lastValidTimestamp);
        timestampValid=0;
        scanIoRequest(timestampValidChange);
        return false;
    }

    // Convert ticks to nanoseconds
    double period=1e9/clockTS(); // in nanoseconds

    if(period<=0 || !isfinite(period))
        return false;

    ts->nsec=(epicsUInt32)(ts->nsec*period);

    // 1 sec. reset is late
    if(ts->nsec>=1000000000u) {
        if(evrMrmTimeDebug>0)
            errlogPrintf("TS convert NS overflow %08x %08x oflow=%u\n",
                         (unsigned)ts->secPastEpoch, (unsigned)ts->nsec,
                         unsigned(ts->nsec-1000000000u));

        // out of bounds
        if(int(ts->nsec-1000000000u)>=evrMrmTimeNSOverflowThreshold) {
            errlogPrintf("TS out of bounds %08x %08x oflow %d evrMrmTimeNSOverflowThreshold %u diff %d\n",
                         (unsigned)ts->secPastEpoch, (unsigned)ts->nsec,
                         int(ts->nsec-1000000000u), evrMrmTimeNSOverflowThreshold,
                         int(ts->nsec-1000000000u)-evrMrmTimeNSOverflowThreshold);
            timestampValid=0;
            lastInvalidTimestamp=ts->secPastEpoch;
            scanIoRequest(timestampValidChange);

            return false;
        }
        // otherwise, truncate
        ts->nsec = 999999999u;
    }

    //Link seconds counter is POSIX time
    ts->secPastEpoch-=POSIX_TIME_AT_EPICS_EPOCH;

    return true;
}

bool
EVRMRM::getTicks(epicsUInt32 *tks)
{
    *tks=READ32(base, TSEvt);
    return true;
}

IOSCANPVT
EVRMRM::eventOccurred(epicsUInt32 event) const
{
    if (event>0 && event<=255)
        return events[event].occured;
    else
        return NULL;
}

void
EVRMRM::eventNotifyAdd(epicsUInt32 event, eventCallback cb, void* arg)
{
    if (event==0 || event>255)
        throw std::out_of_range("Invalid event number");

    SCOPED_LOCK2(evrLock, guard);

    events[event].notifiees.push_back( std::make_pair(cb,arg));

    interestedInEvent(event, true);
}

void
EVRMRM::eventNotifyDel(epicsUInt32 event, eventCallback cb, void* arg)
{
    if (event==0 || event>255)
        throw std::out_of_range("Invalid event number");

    SCOPED_LOCK2(evrLock, guard);

    events[event].notifiees.remove(std::make_pair(cb,arg));

    interestedInEvent(event, false);
}

epicsUInt16
EVRMRM::dbus() const
{
    return (READ32(base, Status) & Status_dbus_mask) >> Status_dbus_shift;
}

bool
EVRMRM::dcEnabled() const
{
    return READ32(base, Control) & Control_DCEna;
}

void
EVRMRM::dcEnable(bool v)
{
    if(v)
        BITSET32(base, Control, Control_DCEna);
    else
        BITCLR32(base, Control, Control_DCEna);
}

double
EVRMRM::dcTarget() const
{
    double period=1e9/clock(); // in nanoseconds
    return double(READ32(base, DCTarget))/65536.0*period;
}

void
EVRMRM::dcTargetSet(double val)
{
    double period=1e9/clock(); // in nanoseconds

    val /= period;
    val *= 65536.0;
    WRITE32(base, DCTarget, val);
}

double
EVRMRM::dcRx() const
{
    double period=1e9/clock(); // in nanoseconds
    return double(READ32(base, DCRxVal))/65536.0*period;
}

double
EVRMRM::dcInternal() const
{
    double period=1e9/clock(); // in nanoseconds
    return double(READ32(base, DCIntVal))/65536.0*period;
}

epicsUInt32
EVRMRM::dcStatusRaw() const
{
    return READ32(base, DCStatus);
}

bool
EVRMRM::psPolarity() const {
	return READ32(base, Control) & Control_pspol;
}

void
EVRMRM::psPolaritySet(bool v)
{
    if(v)
        BITSET32(base, Control, Control_pspol);
    else
        BITCLR32(base, Control, Control_pspol);
}

epicsUInt32
EVRMRM::topId() const
{
    return READ32(base, TOPID);
}

void
EVRMRM::setEvtCode(epicsUInt32 code)
{
    if(code==0) return;
    else if(code>255) throw std::runtime_error("Event code out of range");
    SCOPED_LOCK(evrLock);

    unsigned i;

    // spin fast
    for(i=0; i<100 && READ32(base, SwEvent) & SwEvent_Pend; i++) {}

    if(i==100) {
        // spin slow for <= 50ms
        for(i=0; i<5 && READ32(base, SwEvent) & SwEvent_Pend; i++)
            epicsThreadSleep(0.01);

        if(i==5)
            throw std::runtime_error("SwEvent timeout");
    }

    WRITE32(base, SwEvent, (code<<SwEvent_Code_SHIFT)|SwEvent_Ena);
}

epicsUInt32 EVRMRM::timeSrc() const
{
    SCOPED_LOCK(evrLock);
    return timeSrcMode;
}

void EVRMRM::setTimeSrc(epicsUInt32 raw)
{
    switch((timeSrcMode_t)raw) {
    case Disable:
    case External:
    case SysClk:
        break;
    default:
        throw std::runtime_error("Unsupported time source mode");
    }
    timeSrcMode_t mode((timeSrcMode_t)raw);

    bool changed;
    {
        SCOPED_LOCK(evrLock);

        changed = timeSrcMode!=mode;

        timeSrcMode = mode;
    }

    if(changed)
        softSecondsSrc(mode==SysClk);
}

OBJECT_BEGIN2(EVRMRM, EVR)
  OBJECT_PROP2("Clock Mode", &EVRMRM::clockMode, &EVRMRM::clockModeSet);
  OBJECT_PROP2("DCEnable", &EVRMRM::dcEnabled, &EVRMRM::dcEnable);
  OBJECT_PROP2("DCTarget", &EVRMRM::dcTarget, &EVRMRM::dcTargetSet);
  OBJECT_PROP1("DCRx",     &EVRMRM::dcRx);
  OBJECT_PROP1("DCInt",    &EVRMRM::dcInternal);
  OBJECT_PROP1("DCStatusRaw", &EVRMRM::dcStatusRaw);
  OBJECT_PROP1("DCTOPID", &EVRMRM::topId);
  OBJECT_PROP2("PSPolarity", &EVRMRM::psPolarity, &EVRMRM::psPolaritySet);
  OBJECT_PROP2("EvtCode", &EVRMRM::dummy, &EVRMRM::setEvtCode);
  OBJECT_PROP2("TimeSrc", &EVRMRM::timeSrc, &EVRMRM::setTimeSrc);
    {
      std::string (EVRMRM::*getter)() const = &EVRMRM::nextSecond;
      OBJECT_PROP1("NextSecond", getter);
    }
    {
      double (EVRMRM::*getter)() const = &EVRMRM::deltaSeconds;
      OBJECT_PROP1("Time Error", getter);
    }
    {
      void (EVRMRM::*cmd)() = &EVRMRM::resyncSecond;
      OBJECT_PROP1("Sync TS", cmd);
    }
  OBJECT_PROP2("PLL Bandwidth", &EVRMRM::getPLLBandwidth, &EVRMRM::setPLLBandwidth);
OBJECT_END(EVRMRM)


void
EVRMRM::enableIRQ(void)
{
    interruptLock I;

    shadowIRQEna =  IRQ_Enable
                   |IRQ_RXErr    |IRQ_BufFull
                   |IRQ_Heartbeat
                   |IRQ_Event    |IRQ_FIFOFull
                   |IRQ_SoS      |IRQ_EoS;

    // IRQ PCIe enable flag should not be changed. Possible RACER here
    shadowIRQEna |= (IRQ_PCIee & (READ32(base, IRQEnable)));

    WRITE32(base, IRQEnable, shadowIRQEna);
}

void
EVRMRM::isr_pci(void *arg) {
    EVRMRM *evr=static_cast<EVRMRM*>(arg);

    // Calling the default platform-independent interrupt routine
    evr->isr(evr, true);

#if defined(__linux__) || defined(_WIN32)
    if(devPCIEnableInterrupt((const epicsPCIDevice*)evr->isrLinuxPvt)) {
        printf("Failed to re-enable interrupt.  Stuck...\n");
    }
#endif
}

void
EVRMRM::isr_vme(void *arg) {
    EVRMRM *evr=static_cast<EVRMRM*>(arg);

    // Calling the default platform-independent interrupt routine
    evr->isr(evr, false);
}

void
EVRMRM::isr_poll(void *arg) {
    EVRMRM *evr=static_cast<EVRMRM*>(arg);

    // Calling the default platform-independent interrupt routine
    evr->isr(evr, true);
}

// A place to write to which will keep the read
// at the end of the ISR from being optimized out.
// This value should never be used anywhere else.
volatile epicsUInt32 evrMrmIsrFlagsTrashCan;

void
EVRMRM::isr(EVRMRM *evr, bool pci)
{

    epicsUInt32 flags=READ32(evr->base, IRQFlag);

    epicsUInt32 active=flags&evr->shadowIRQEna;

#if defined(vxWorks) || defined(__rtems__)
    if(!active) {
#  ifdef __rtems__
        if(!pci)
            printk("EVRMRM::isr with no active VME IRQ 0x%08x 0x%08x\n", flags, evr->shadowIRQEna);
#else
        (void)pci;
#  endif
        // this is a shared interrupt
        return;
    }
    // Note that VME devices do not normally shared interrupts
#else
    // for Linux, shared interrupts are detected by the kernel module
    // so any notifications to userspace are real interrupts by this device
    (void)pci;
#endif

    if(active&IRQ_RXErr){
        evr->count_recv_error++;
        scanIoRequest(evr->IRQrxError);

        evr->shadowIRQEna &= ~IRQ_RXErr;
        callbackRequest(&evr->poll_link_cb);
    }
    if(active&IRQ_BufFull){
        // Silence interrupt
        BITSET(NAT,32,evr->base, DataBufCtrl, DataBufCtrl_stop);

        callbackRequest(&evr->data_rx_cb);
    }
    if(active&IRQ_HWMapped){
        evr->shadowIRQEna &= ~IRQ_HWMapped;
        //TODO: think of a way to use this feature...
    }
    if(active&IRQ_Event){
        //FIFO not-empty
        evr->shadowIRQEna &= ~IRQ_Event;
        int wakeup=0;
        evr->drain_fifo_wakeup.trySend(&wakeup, sizeof(wakeup));
    }
    if(active&IRQ_Heartbeat){
        evr->count_heartbeat++;
        scanIoRequest(evr->IRQheartbeat);
    }
    if(active&IRQ_FIFOFull){
        evr->shadowIRQEna &= ~IRQ_FIFOFull;
        int wakeup=0;
        evr->drain_fifo_wakeup.trySend(&wakeup, sizeof(wakeup));

        scanIoRequest(evr->IRQfifofull);
    }
    if(active&IRQ_SoS && evr->seq.get()){
        evr->seq->doStartOfSequence(0);
    }
    if(active&IRQ_EoS && evr->seq.get()){
        evr->seq->doEndOfSequence(0);
    }
    evr->count_hardware_irq++;

    // IRQ PCIe enable flag should not be changed. Possible RACER here
    evr->shadowIRQEna |= (IRQ_PCIee & (READ32(evr->base, IRQEnable)));

    WRITE32(evr->base, IRQFlag, flags);
    WRITE32(evr->base, IRQEnable, evr->shadowIRQEna);
    // Ensure IRQFlags is written before returning.
    evrMrmIsrFlagsTrashCan=READ32(evr->base, IRQFlag);
}

// Caller must hold evrLock
static
void
eventInvoke(eventCode& event)
{
#ifdef HAVE_PARALLEL_CB
    // bit mask of priorities for which scans have been queued
    unsigned prio_queued =
#endif
    scanIoRequest(event.occured);

    for(eventCode::notifiees_t::const_iterator it=event.notifiees.begin();
        it!=event.notifiees.end();
        ++it)
    {
        (*it->first)(it->second, event.code);
    }

    event.waitingfor=0; // assume caller handles waitingfor>0
    for(unsigned p=0; p<NUM_CALLBACK_PRIORITIES; p++) {
#ifdef HAVE_PARALLEL_CB
        // only sync priorities where work is queued
        if((prio_queued&(1u<<p))==0) continue;
#endif
        event.waitingfor++;
        event.done_cb.priority=p;
        callbackRequest(&event.done_cb);
    }
}

void
EVRMRM::drain_fifo()
{
    size_t i;
    printf("EVR FIFO task start\n");

    SCOPED_LOCK2(evrLock, guard);

    while(true) {
        int msg, err;

        guard.unlock();

        err=drain_fifo_wakeup.receive(&msg, sizeof(msg));

        if (err<0) {
            errlogPrintf("FIFO wakeup error %d\n",err);
            epicsThreadSleep(0.1); // avoid message flood
            guard.lock();
            continue;

        } else if(msg==1) {
            // Request thread stop
            guard.lock();
            break;
        }

        guard.lock();

        count_fifo_loops++;

        epicsUInt32 status;

        // Bound the number of events taken from the FIFO
        // at one time.
        for(i=0; i<512; i++) {

            status=READ32(base, IRQFlag);
            if (!(status&IRQ_Event))
                break;
            if (status&IRQ_RXErr)
                break;

            epicsUInt32 code=READ32(base, EvtFIFOCode);
            if (!code)
                break;

            if (code>NELEMENTS(events)) {
                // BUG: we get occasional corrupt VME reads of this register
                // Fixed in firmware.  Feb 2011
                epicsUInt32 code2=READ32(base, EvtFIFOCode);
                if (code2>NELEMENTS(events)) {
                    printf("Really weird event 0x%08x 0x%08x\n", code, code2);
                    break;
                } else
                    code=code2;
            }
            code &= 0xff; // (in)santity check

            count_fifo_events++;

            eventCode& evt = events[code];

            // cache of last time
            evt.last_sec=READ32(base, EvtFIFOSec);
            evt.last_evt=READ32(base, EvtFIFOEvt);

            // update any timestamp buffers
            for(eventCode::tbufs_t::const_iterator it(evt.tbufs.begin()), end(evt.tbufs.end());
                it!=end; ++it)
            {
                EVRMRMTSBuffer* tbuf = *it;

                if(tbuf->timeEvt==code) {
                    EVRMRMTSBuffer::ebuf_t& buf = tbuf->ebufs[tbuf->active];
                    // add code to buffer
                    if(buf.pos < buf.buf.size()) {
                        // append raw time to buffer
                        buf.buf[buf.pos].secPastEpoch = evt.last_sec;
                        buf.buf[buf.pos].nsec = evt.last_evt;
                        buf.pos++;

                    } else {
                        buf.drop = true;
                        tbuf->dropped++;
                    }
                }

                if(tbuf->flushEvt==code) {
                    // flush
                    EVRMRMTSBuffer::ebuf_t& active = tbuf->ebufs[tbuf->active];
                    active.flushtime.secPastEpoch = evt.last_sec;
                    active.flushtime.nsec = evt.last_evt;

                    active.ok &= convertTS(&active.flushtime);

                    tbuf->doFlush();
                }
            }

            if (evt.again) {
                // ignore extra events in buffer.
            } else if (evt.waitingfor>0) {
                // already queued, but received again before all
                // callbacks finished.  Un-map event until complete
                evt.again=true;
                specialSetMap(code, ActionFIFOSave, false);
                count_FIFO_sw_overrate++;
            } else {
                // needs to be queued
                eventInvoke(evt);
            }

        }

        if (status&IRQ_FIFOFull) {
            count_FIFO_overflow++;
        }

        if (status&(IRQ_FIFOFull|IRQ_RXErr)) {
            // clear fifo if link lost or buffer overflow
            BITSET(NAT,32, base, Control, Control_fiforst);
        }

        int iflags=epicsInterruptLock();

        //*
        shadowIRQEna |= IRQ_Event | IRQ_FIFOFull;
        // IRQ PCIe enable flag should not be changed. Possible RACER here
        shadowIRQEna |= (IRQ_PCIee & (READ32(base, IRQEnable)));

        WRITE32(base, IRQEnable, shadowIRQEna);

        epicsInterruptUnlock(iflags);

        // wait a fixed interval before checking again
        // Prevents this thread from starving others
        // if a high frequency event is accidentally
        // mapped into the FIFO.
        if(mrmEvrFIFOPeriod>0.0) {
            guard.unlock();
            epicsThreadSleep(mrmEvrFIFOPeriod);
            guard.lock();
        }
    }

    printf("FIFO task exiting\n");
}

void
EVRMRM::sentinel_done(CALLBACK* cb)
{
try {
    void *vptr;
    callbackGetUser(vptr,cb);
    eventCode *sent=static_cast<eventCode*>(vptr);

    SCOPED_LOCK2(sent->owner->evrLock, guard);

    // Is this the last callback queue?
    if (--sent->waitingfor)
        return;

    bool run=sent->again;
    sent->again=false;

    // Re-enable mapping if disabled
    if (run && sent->interested) {
        sent->owner->specialSetMap(sent->code, ActionFIFOSave, true);
    }
} catch(std::exception& e) {
    epicsPrintf("exception in sentinel_done callback: %s\n", e.what());
}
}

void
EVRMRM::poll_link(CALLBACK* cb)
{
try {
    void *vptr;
    callbackGetUser(vptr,cb);
    EVRMRM *evr=static_cast<EVRMRM*>(vptr);

    epicsUInt32 flags=READ32(evr->base, IRQFlag);

    if(flags&IRQ_RXErr){
        // Still down
        callbackRequestDelayed(&evr->poll_link_cb, 0.1); // poll again in 100ms
        {
            SCOPED_LOCK2(evr->evrLock, guard);
            if(evr->timestampValid && evrMrmTimeDebug>0)
                errlogPrintf("TS invalid as link goes down\n");
            evr->timestampValid=0;

            evr->lastInvalidTimestamp=evr->lastValidTimestamp;
            scanIoRequest(evr->timestampValidChange);
        }
        WRITE32(evr->base, IRQFlag, IRQ_RXErr);
    }else{
        scanIoRequest(evr->IRQrxError);
        int iflags=epicsInterruptLock();
        evr->shadowIRQEna |= IRQ_RXErr;
        // IRQ PCIe enable flag should not be changed. Possible RACER here
        evr->shadowIRQEna |= (IRQ_PCIee & (READ32(evr->base, IRQEnable)));
        WRITE32(evr->base, IRQEnable, evr->shadowIRQEna);
        epicsInterruptUnlock(iflags);
    }
} catch(std::exception& e) {
    epicsPrintf("exception in poll_link callback: %s\n", e.what());
}
}

static
void send_timestamp(CALLBACK *cb)
{
    void *raw;
    callbackGetUser(raw, cb);
    EVRMRM *evr=static_cast<EVRMRM*>(raw);

    evr->tickSecond();
}

void
EVRMRM::seconds_tick(void *raw, epicsUInt32)
{
    EVRMRM *evr=static_cast<EVRMRM*>(raw);

    SCOPED_LOCK2(evr->evrLock, guard);

    // Don't bother to latch since we are only reading the seconds
    epicsUInt32 newSec=READ32(evr->base, TSSec);

    bool valid=true;

    /* Received a known bad value */
    if(evr->lastInvalidTimestamp==newSec) {
        valid=false;
        if(evrMrmTimeDebug>0)
            errlogPrintf("TS reset repeats known bad value new %08x bad %08x\n",
                         (unsigned)newSec, (unsigned)evr->lastInvalidTimestamp);
    }

    /* Received a value which is inconsistent with a previous value */
    if(evr->timestampValid>0
       &&  evr->lastValidTimestamp!=(newSec-1) ) {
        valid=false;
        if(evrMrmTimeDebug>0)
            errlogPrintf("TS reset with inconsistent value new %08x\n",
                         (unsigned)newSec);
    }

    /* received the previous value again */
    else if(evr->lastValidTimestamp==newSec) {
        valid=false;
        if(evrMrmTimeDebug>0)
            errlogPrintf("TS reset repeats previous value new %08x last %08x\n",
                         (unsigned)newSec, (unsigned)evr->lastValidTimestamp);
    }


    if (!valid)
    {
        if (evr->timestampValid>0) {
            if(evrMrmTimeDebug>0)
                errlogPrintf("TS reset w/ old or invalid seconds %08x (%08x %08x)\n",
                             newSec, evr->lastValidTimestamp, evr->lastInvalidTimestamp);
            scanIoRequest(evr->timestampValidChange);
        }
        evr->timestampValid=0;
        evr->lastInvalidTimestamp=newSec;
        if(evrMrmTimeDebug>2)
            errlogPrintf("TS reset invalid new %08x\n", (unsigned)newSec);

    } else {
        if(evr->timestampValid<=TSValidThreshold) evr->timestampValid++;
        evr->lastValidTimestamp=newSec;

        if (evr->timestampValid == TSValidThreshold) {
            if(evrMrmTimeDebug>0)
                errlogPrintf("TS becomes valid after fault %08x\n",newSec);
            scanIoRequest(evr->timestampValidChange);

        } else if(evrMrmTimeDebug>2) {
            errlogPrintf("TS reset valid new %08x %u\n",
                         (unsigned)newSec, (unsigned)evr->timestampValid);
        }
    }

    if(evr->timeSrcMode==External) {
        // avoid lock ordering problem with EVR lock and generalTime locks
        callbackSetCallback(&send_timestamp, &evr->timeSrc_cb);
        callbackSetUser(evr, &evr->timeSrc_cb);
        callbackSetPriority(priorityMedium, &evr->timeSrc_cb);
        callbackRequest(&evr->timeSrc_cb);
    }
}

#ifdef DBR_UTAG
// Get UTAG value for specific event
epicsUTag
EVRMRM::getUtag(const epicsUInt32 event) const {
    if(event==0) return 0;
    else if(event>255) throw std::runtime_error("Event code out of range");
    SCOPED_LOCK(evrLock);

    return events[event].utag;
}

// Set UTAG value for specific event
void
EVRMRM::setUtag(epicsUTag tag, const epicsUInt32 event) {
    if(event==0) return;
    else if(event>255) throw std::runtime_error("Event code out of range");
    SCOPED_LOCK(evrLock);

    // set UTAG value to particular event
    events[event].utag = tag;
    return;
}
#endif
