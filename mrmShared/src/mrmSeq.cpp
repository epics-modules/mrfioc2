/*************************************************************************\
* Copyright (c) 2016 Michael Davidsaver
* Copyright (c) 2022 Cosylab d.d.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#if defined(__rtems__)
#  include <rtems.h>
#endif

#include <stdio.h>

#include <epicsMath.h>
#include <epicsMutex.h>
#include <errlog.h>

#include <mrfCommonIO.h>

#include "mrmSeq.h"

#include <epicsExport.h>

#define  EVG_SEQ_RAM_RUNNING    0x02000000  /* Sequence RAM is Running (read only) */
#define  EVG_SEQ_RAM_ENABLED    0x01000000  /* Sequence RAM is Enabled (read only) */

//TODO: external enable/trigger bits and hanlding?
#define  EVG_SEQ_RAM_SW_TRIG    0x00200000  /* Sequence RAM Software Trigger Bit */
#define  EVG_SEQ_RAM_RESET      0x00040000  /* Sequence RAM Reset */
#define  EVG_SEQ_RAM_DISABLE    0x00020000  /* Sequence RAM Disable (and stop) */
#define  EVG_SEQ_RAM_ARM        0x00010000  /* Sequence RAM Enable/Arm */

#define EVG_SEQ_RAM_SWMASK         0x0000F000 // Sequence RAM Software mask
#define EVG_SEQ_RAM_SWMASK_shift   12
#define EVG_SEQ_RAM_SWENABLE       0x00000F00 // Sequence RAM Software enable
#define EVG_SEQ_RAM_SWENABLE_shift 8

#define  EVG_SEQ_RAM_WRITABLE_MASK 0x00ffffff
#define  EVG_SEQ_RAM_REPEAT_MASK 0x00180000 /* Sequence RAM Repeat Mode Mask */
#define  EVG_SEQ_RAM_NORMAL     0x00000000  /* Normal Mode: Repeat every trigger */
#define  EVG_SEQ_RAM_SINGLE     0x00100000  /* Single-Shot Mode: Disable on completion */
#define  EVG_SEQ_RAM_RECYCLE    0x00080000  /* Continuous Mode: Repeat on completion */

#define  EVG_SEQ_RAM_SRC_MASK 0x000000ff

#if defined(__rtems__)
#  define DEBUG(LVL, ARGS) do{if(SeqManagerDebug>=(LVL)) {printk ARGS ;}}while(0)
#elif defined(vxWorks)
#  define DEBUG(LVL, ARGS) do{}while(0)
#else
#  define DEBUG(LVL, ARGS) do{if(SeqManagerDebug>=(LVL)) {printf ARGS ;}}while(0)
#endif

namespace {

enum RunMode {Normal=0, Single=2};

}//namespace

struct SoftSequence;

struct SeqHW
{
    SeqManager * const owner;
    const unsigned idx;
    volatile void * const ctrlreg,
                  * const rambase;

    // guarded by interruptLock

    //! current association.  may be NULL
    SoftSequence *loaded;
    //! between SoS and EoS
    bool running;

    epicsUInt32 ctrlreg_user, //!< user requested (based on commited sequence)
                ctrlreg_hw;   //!< current in HW.  either same as _user or trigger disabled

    SeqHW(SeqManager * o,
          unsigned i,
          volatile void *ctrl,
          volatile void *ram)
        :owner(o)
        ,idx(i)
        ,ctrlreg(ctrl)
        ,rambase(ram)
        ,loaded(0)
        ,running(false)
        ,ctrlreg_user(0u)
        ,ctrlreg_hw(0u)
    {
        switch(owner->type) {
        case SeqManager::TypeEVG:
            ctrlreg_user |= 31;
            break;
        case SeqManager::TypeEVR:
            ctrlreg_user |= 63;
            break;
        default:
            return;
        }
        ctrlreg_hw = ctrlreg_user;

        nat_iowrite32(ctrlreg, ctrlreg_hw | EVG_SEQ_RAM_RESET);
    }

    // call with interruptLock
    void arm()
    {
        ctrlreg_hw = ctrlreg_user;
        nat_iowrite32(ctrlreg, ctrlreg_hw | EVG_SEQ_RAM_ARM);
    }

    // call with interruptLock
    bool disarm()
    {

        // EVG_SEQ_RAM_DISABLE is really "stop"
        // to avoid aborting a potentially running sequencer, switch the trigger source to disable

        ctrlreg_hw &= ~(EVG_SEQ_RAM_SRC_MASK);

        switch(owner->type) {
        case SeqManager::TypeEVG:
            ctrlreg_hw |= 31;
            break;
        case SeqManager::TypeEVR:
            ctrlreg_hw |= 63;
            break;
        }

        nat_iowrite32(ctrlreg, ctrlreg_hw);
        bool isrun = nat_ioread32(ctrlreg) & EVG_SEQ_RAM_RUNNING;

        return isrun;
    }
};

struct SoftSequence : public mrf::ObjectInst<SoftSequence>
{
    typedef mrf::ObjectInst<SoftSequence> base_t;

    SoftSequence(SeqManager *o, const std::string& name);
    virtual ~SoftSequence();

    virtual void lock() const { mutex.lock(); }
    virtual void unlock() const { mutex.unlock(); }

    std::string getErr() const { SCOPED_LOCK(mutex); return last_err; }
    IOSCANPVT  getErrScan() const { return onErr; }

    epicsUInt32 getTimestampResolution() const
    {
        SCOPED_LOCK(mutex);
        return timeScale;
    }
    void setTimestampResolution(epicsUInt32 val)
    {
        {
            SCOPED_LOCK(mutex);
            timeScale = val;
        }
        DEBUG(4, ("Set time scale\n"));
        scanIoRequest(changed);
    }

private:
    double getTimeScale() const
    {
        double tmult = 1.0;
        SCOPED_LOCK(mutex);
        if(timeScale) {
            double freq = owner->getClkFreq(); // in ticks/sec
            /* tscale==1 -> seconds
             * tscale==1000 -> milliseconds
             * ...
             */
            tmult = freq/timeScale;
        }
        return tmult;
    }
public:

    // dset actions
    // update scratch
    // read back committed

    void setTimestamp(const double *arr, epicsUInt32 count)
    {
        const double tmult = getTimeScale();
        times_t times(count);
        // check for monotonic
        // TODO: not handling overflow (HW supports controlled rollover w/ special 0xffffffff times)
        for(epicsUInt32 i=0; i<count; i++)
        {
            if(!finite(arr[i]) || arr[i]<0.0) {
                std::string msg("Times must be finite >=0");
                last_err = msg;
                scanIoRequest(onErr);
                throw std::runtime_error(msg);
            }

            times[i] = (arr[i]*tmult)+0.5;

            if(i>0 && times[i]<=times[i-1]) {
                std::string msg("Non-monotonic timestamp array");
                last_err = msg;
                scanIoRequest(onErr);
                throw std::runtime_error(msg);
            } else if(times[i]==0xffffffff) {
                std::string msg("Time overflow, rollover not supported");
                last_err = msg;
                scanIoRequest(onErr);
                throw std::runtime_error(msg);
            }
        }
        {
            SCOPED_LOCK(mutex);
            scratch.times.swap(times);
            is_committed = false;
        }
        DEBUG(4, ("Set times\n"));
        scanIoRequest(changed);
    }

    epicsUInt32 getTimestamp(double* arr, epicsUInt32 count) const
    {
        SCOPED_LOCK(mutex);
        const double tmult = getTimeScale();
        epicsUInt32 ret = std::min(size_t(count), committed.times.size());
        for(epicsUInt32 i=0; i<ret; i++) {
            arr[i] = committed.times[i]/tmult;
        }
        return ret;
    }

    void setEventCode(const epicsUInt8* arr, epicsUInt32 count)
    {
        codes_t codes(count);
        std::copy(arr,
                  arr+count,
                  codes.begin());
        {
            SCOPED_LOCK(mutex);
            scratch.codes.swap(codes);
            is_committed = false;
        }
        DEBUG(4, ("Set events\n"));
        scanIoRequest(changed);
    }

    epicsUInt32 getEventCode(epicsUInt8* arr, epicsUInt32 count) const
    {
        SCOPED_LOCK(mutex);
        epicsUInt32 ret = std::min(size_t(count), committed.codes.size());
        std::copy(committed.codes.begin(),
                  committed.codes.begin()+ret,
                  arr);
        return ret;
    }

    void setMask(const epicsUInt8 *arr, epicsUInt32 count)
    {
        masks_t masks(count);
        if (*arr>15){
            std::string msg("4 bits code. Authorized range is [0-15]");
            last_err = msg;
            scanIoRequest(onErr);
            throw std::runtime_error(msg);
        }
        std::copy(arr,
                  arr + count,
                  masks.begin());
        {
            SCOPED_LOCK(mutex);
            scratch.masks.swap(masks);
            is_committed = false;
        }
        DEBUG(4, ("Set masks\n"));
        scanIoRequest(changed);
    }

    epicsUInt32 getMask(epicsUInt8 *arr, epicsUInt32 count) const
    {
        SCOPED_LOCK(mutex);
        epicsUInt32 ret = std::min(size_t(count), committed.masks.size());
        std::copy(committed.masks.begin(),
                  committed.masks.begin() + ret,
                  arr);
        return ret;
    }

    void setEna(const epicsUInt8 *arr, epicsUInt32 count)
    {
        if (*arr>15){
            std::string msg("4 bits code. Authorized range is [0-15]");
            last_err = msg;
            scanIoRequest(onErr);
            throw std::runtime_error(msg);
        }
        enables_t enables(count);
        std::copy(arr,
                  arr + count,
                  enables.begin());
        {
            SCOPED_LOCK(mutex);
            scratch.enables.swap(enables);
            is_committed = false;
        }
        DEBUG(4, ("Set enables\n"));
        scanIoRequest(changed);
    }

    epicsUInt32 getEna(epicsUInt8 *arr, epicsUInt32 count) const
    {
        SCOPED_LOCK(mutex);
        epicsUInt32 ret = std::min(size_t(count), committed.enables.size());
        std::copy(committed.enables.begin(),
                  committed.enables.begin() + ret,
                  arr);
        return ret;
    }

    void setTrigSrc(epicsUInt32 src)
    {
        DEBUG(4, ("Setting trig src %x\n", (unsigned)src));
        {
            SCOPED_LOCK(mutex);
            scratch.src = src;
            is_committed = false;
        }
        DEBUG(4, ("Set trig src %x\n", (unsigned)src));
        scanIoRequest(changed);
    }

    epicsUInt32 getTrigSrcCt() const
    {
        SCOPED_LOCK(mutex);
        return committed.src;
    }

    void setRunMode(epicsUInt32 mode)
    {
        switch(mode) {
        case Single:
        case Normal:
            break;
        default:
            std::string msg("Unknown sequencer run mode");
            last_err = msg;
            scanIoRequest(onErr);
            throw std::runtime_error(msg);
        }

        {
            SCOPED_LOCK(mutex);
            scratch.mode = (RunMode)mode;
            is_committed = false;
        }
        DEBUG(4, ("Set run mode %u\n", (unsigned)mode));
        scanIoRequest(changed);
    }

    epicsUInt32 getRunModeCt() const
    {
        SCOPED_LOCK(mutex);
        return committed.mode;
    }

    // dset actions
    // control/status

    bool isLoaded() const { SCOPED_LOCK(mutex); return hw; }
    bool isEnabled() const { SCOPED_LOCK(mutex); return is_enabled; }
    bool isCommited() const { SCOPED_LOCK(mutex); return is_committed; }
    IOSCANPVT stateChange() const { return changed; }

    void load();
    void unload();
    void commit();
    void enable();
    void disable();
    void softTrig();
    
    epicsUInt32 getSwMask() const;
    void setSwMask(epicsUInt32 src);
    epicsUInt32 getSwEna() const;
    void setSwEna(epicsUInt32 src);


    epicsUInt32 counterStart() const { interruptLock L; return numStart; }
    IOSCANPVT counterStartScan() const { return onStart; }
    epicsUInt32 counterEnd() const { interruptLock L; return numEnd; }
    IOSCANPVT counterEndScan() const { return onEnd; }

    // internal

    void sync();

    SeqManager * const owner;

    //! guarded by our mutex and interruptLock
    //! only write when both held
    //! read when either held
    SeqHW *hw;

    mutable epicsMutex mutex;

    typedef std::vector<epicsUInt64> times_t;
    typedef std::vector<epicsUInt8> codes_t;
    typedef std::vector<epicsUInt8> masks_t;
    typedef std::vector<epicsUInt8> enables_t;

    struct Config {
        times_t times;
        codes_t codes;
        masks_t masks;
        enables_t enables;
        RunMode mode;
        epicsUInt32 src;
        Config()
            :mode(Single)
            ,src(0x03000000) // code for Disable
        {}
        void swap(Config& o)
        {
            std::swap(times, o.times);
            std::swap(codes, o.codes);
            std::swap(masks, o.masks);
            std::swap(enables, o.enables);
            std::swap(mode, o.mode);
            std::swap(src, o.src);
        }
    } scratch,   // guarded by our mutex only
      committed; // guarded by interruptLock only

    //! Whether user has requested enable
    bool is_enabled;
    //! clear when scratch and commited sequences differ
    bool is_committed;
    //! clear when the commited sequence differs from the HW sequence (eg. commit while running)
    //! Guarded by interruptLock only
    bool is_insync;

    //! Guarded by interruptLock only
    epicsUInt32 numStart, numEnd;

    epicsUInt32 timeScale;

    IOSCANPVT changed, onStart, onEnd, onErr;

    std::string last_err;
};

OBJECT_BEGIN(SoftSequence)
  OBJECT_PROP1("ERROR", &SoftSequence::getErr);
  OBJECT_PROP1("ERROR", &SoftSequence::getErrScan);
  OBJECT_PROP1("LOADED", &SoftSequence::isLoaded);
  OBJECT_PROP1("LOADED", &SoftSequence::stateChange);
  OBJECT_PROP1("LOAD", &SoftSequence::load);
  OBJECT_PROP1("UNLOAD", &SoftSequence::unload);
  OBJECT_PROP1("ENABLED", &SoftSequence::isEnabled);
  OBJECT_PROP1("ENABLED", &SoftSequence::stateChange);
  OBJECT_PROP1("ENABLE", &SoftSequence::enable);
  OBJECT_PROP1("DISABLE", &SoftSequence::disable);
  OBJECT_PROP1("COMMITTED", &SoftSequence::isCommited);
  OBJECT_PROP1("COMMITTED", &SoftSequence::stateChange);
  OBJECT_PROP1("COMMIT", &SoftSequence::commit);
  OBJECT_PROP2("SWMASK", &SoftSequence::getSwMask, &SoftSequence::setSwMask);
  OBJECT_PROP1("SWMASK", &SoftSequence::stateChange);
  OBJECT_PROP2("SWENA", &SoftSequence::getSwEna, &SoftSequence::setSwEna);
  OBJECT_PROP1("SWENA", &SoftSequence::stateChange);
  OBJECT_PROP1("SOFT_TRIG", &SoftSequence::softTrig);
  OBJECT_PROP2("TIMES", &SoftSequence::getTimestamp, &SoftSequence::setTimestamp);
  OBJECT_PROP1("TIMES", &SoftSequence::stateChange);
  OBJECT_PROP2("CODES", &SoftSequence::getEventCode, &SoftSequence::setEventCode);
  OBJECT_PROP1("CODES", &SoftSequence::stateChange);
  OBJECT_PROP2("MASK", &SoftSequence::getMask, &SoftSequence::setMask);
  OBJECT_PROP1("MASK", &SoftSequence::stateChange);
  OBJECT_PROP2("ENA", &SoftSequence::getEna, &SoftSequence::setEna);
  OBJECT_PROP1("ENA", &SoftSequence::stateChange);
  OBJECT_PROP1("NUM_RUNS", &SoftSequence::counterEnd);
  OBJECT_PROP1("NUM_RUNS", &SoftSequence::counterEndScan);
  OBJECT_PROP1("NUM_STARTS", &SoftSequence::counterStart);
  OBJECT_PROP1("NUM_STARTS", &SoftSequence::counterStartScan);
  OBJECT_PROP2("TIMEUNITS", &SoftSequence::getTimestampResolution, &SoftSequence::setTimestampResolution);
  OBJECT_PROP2("TRIG_SRC", &SoftSequence::getTrigSrcCt, &SoftSequence::setTrigSrc);
  OBJECT_PROP1("TRIG_SRC", &SoftSequence::stateChange);
  OBJECT_PROP2("RUN_MODE", &SoftSequence::getRunModeCt, &SoftSequence::setRunMode);
  OBJECT_PROP1("RUN_MODE", &SoftSequence::stateChange);
OBJECT_END(SoftSequence)

SoftSequence::SoftSequence(SeqManager *o, const std::string& name)
    :base_t(name)
    ,owner(o)
    ,hw(0)
    ,is_enabled(false)
    ,is_committed(false)
    ,is_insync(false)
    ,numStart(0u)
    ,numEnd(0u)
    ,timeScale(0u) // raw/ticks
{
    scanIoInit(&changed);
    scanIoInit(&onStart);
    scanIoInit(&onEnd);
    scanIoInit(&onErr);
}

SoftSequence::~SoftSequence() {}

void SoftSequence::softTrig()
{
    DEBUG(3, ("SW Triggering\n") );
    SCOPED_LOCK(mutex);
    if(!hw || !is_enabled) {DEBUG(3, ("Skip\n")); return;}

    {
        interruptLock L;
        nat_iowrite32(hw->ctrlreg, hw->ctrlreg_hw | EVG_SEQ_RAM_SW_TRIG);
    }
    DEBUG(2, ("SW Triggered\n") );
}

epicsUInt32 SoftSequence::getSwMask() const
{
    epicsUInt32 val;

    DEBUG(3, ("SW Mask getter\n"));
    SCOPED_LOCK(mutex);
    if (!hw || !is_enabled)
    {
        DEBUG(3, ("Skip\n"));
        return 0;
    }

    else
    {
        DEBUG(5, ("Register sequencer : %u\n", nat_ioread32(hw->ctrlreg)));
        epicsUInt32 val = (nat_ioread32(hw->ctrlreg) & EVG_SEQ_RAM_SWMASK) >> EVG_SEQ_RAM_SWMASK_shift;
        DEBUG(5, ("Response sequencer : %u\n", val));
        return val;
    }
}

void SoftSequence::setSwMask(epicsUInt32 src)
{

    if (src>15){
        std::string msg("4 bits code. Authorized range is [0-15]");
        last_err = msg;
        scanIoRequest(onErr);
        throw std::runtime_error(msg);
    }
    DEBUG(3, ("SW Mask setter\n"));
    if (!hw || !is_enabled)
    {
        DEBUG(3, ("Skip\n"));
        return;
    }

    else
    {
        hw->ctrlreg_hw &= ~(EVG_SEQ_RAM_SWMASK);
        hw->ctrlreg_hw |= src << EVG_SEQ_RAM_SWMASK_shift;
        nat_iowrite32(hw->ctrlreg, hw->ctrlreg_hw);
        scanIoRequest(changed);
    }
}

epicsUInt32 SoftSequence::getSwEna() const
{
    epicsUInt32 val;

    DEBUG(3, ("SW Enable getter\n"));
    SCOPED_LOCK(mutex);
    if (!hw || !is_enabled)
    {
        DEBUG(3, ("Skip\n"));
        return 0;
    }

    else
    {
        DEBUG(5, ("Register sequencer : %u\n", nat_ioread32(hw->ctrlreg)));
        epicsUInt32 val = (nat_ioread32(hw->ctrlreg) & EVG_SEQ_RAM_SWENABLE) >> EVG_SEQ_RAM_SWENABLE_shift;
        DEBUG(5, ("Response sequencer : %u\n", val));
        return val;
    }
}

void SoftSequence::setSwEna(epicsUInt32 src)
{
    if (src>15){
        std::string msg("4 bits code. Authorized range is [0-15]");
        last_err = msg;
        scanIoRequest(onErr);
        throw std::runtime_error(msg);
    }
    DEBUG(3, ("SW Enable setter\n"));
    if (!hw || !is_enabled)
    {
        DEBUG(3, ("Skip\n"));
        return;
    }
    else
    {
        hw->ctrlreg_hw &= ~(EVG_SEQ_RAM_SWENABLE);
        hw->ctrlreg_hw |= src << EVG_SEQ_RAM_SWENABLE_shift;
        nat_iowrite32(hw->ctrlreg, hw->ctrlreg_hw);
        scanIoRequest(changed);
    }
}


void SoftSequence::load()
{
    SCOPED_LOCK(mutex);
    DEBUG(3, ("Loading %c\n", hw ? 'L' : 'U') );
    if(hw) {DEBUG(3, ("Skip\n")); return;}

    // find unused SeqHW
    {
        interruptLock L;

        is_insync = false; // paranoia

        for(size_t i=0, N=owner->hw.size(); i<N; i++) {
            SeqHW *temp = owner->hw[i];
            if(temp && !temp->loaded) {
                temp->loaded = this;
                hw = temp;
                break;
            }
        }

        if(hw) {
            // paranoia: disable any external trigger mappings
            owner->mapTriggerSrc(hw->idx, 0x02000000);

            if(!hw->disarm())
                sync();
        }
    }

    if(!hw) {
        last_err = "All HW Seq. in use";
        scanIoRequest(onErr);
        throw alarm_exception(MAJOR_ALARM, WRITE_ALARM);
    }

    // clear residual error (if any)
    last_err = "";
    scanIoRequest(onErr);

    scanIoRequest(changed);
    DEBUG(1, ("Loaded\n") );
}

void SoftSequence::unload()
{
    SCOPED_LOCK(mutex);
    DEBUG(3, ("Unloading %c\n", hw ? 'L' : 'U') );

    if(!hw) {DEBUG(3, ("Skip\n")); return;}

    assert(hw->loaded=this);

    {
        interruptLock L;

        hw->disarm();

        hw->loaded = NULL;
        hw = NULL;

        is_insync = false;
    }

    scanIoRequest(changed);
    DEBUG(1, ("Unloaded\n") );

}

void SoftSequence::commit()
{
    SCOPED_LOCK(mutex);
    DEBUG(3, ("Committing %c\n", is_committed ? 'Y' : 'N') );

    if(is_committed) {DEBUG(3, ("Skip\n")); return;}

    // scratch.times already check for monotonic

    Config conf(scratch); // vector copies

    size_t buflen = std::min(conf.codes.size(),
                             conf.times.size());
    conf.codes.resize(buflen);
    conf.times.resize(buflen);
    conf.enables.resize(buflen);
    conf.masks.resize(buflen);

    // ensure presence of trailing end of sequence marker event 0x7f
    if(conf.codes.empty() || conf.codes.back()!=0x7f)
    {
        if(!conf.times.empty() && conf.times.back()==0xffffffff) {
            std::string msg("Input array is missing 0x7f and maxing out times");
            last_err = msg;
            scanIoRequest(onErr);
            throw std::runtime_error(msg);
        }

        conf.codes.push_back(0x7f);

        if(conf.times.empty())
            conf.times.push_back(0);
        else
            conf.times.push_back(conf.times.back()+1);

        conf.masks.push_back(0);
        conf.enables.push_back(0);

    }

    if(conf.times.size()>2048) {
        std::string msg("Sequence too long");
        last_err = msg;
        scanIoRequest(onErr);
        throw std::runtime_error(msg);
    }

    assert(!hw || hw->loaded==this);

    {
        interruptLock L;
        committed.swap(conf);
        is_committed = true;
        is_insync = false;

        if(hw && !hw->disarm())
            sync();
    }

    // clear residual error (if any)
    last_err = "";
    scanIoRequest(onErr);

    scanIoRequest(changed);
    DEBUG(1, ("Committed\n") );
}

void SoftSequence::enable()
{
    SCOPED_LOCK(mutex);
    DEBUG(3, ("Enabling %c\n", is_enabled ? 'Y' : 'N') );
    if(is_enabled)
        {DEBUG(3, ("Skip\n")); return;}

    is_enabled = true;

    if(hw) {
        interruptLock I;

        hw->arm();
    }

    // clear residual error (if any)
    last_err = "";
    scanIoRequest(onErr);

    scanIoRequest(changed);
    DEBUG(1, ("Enabled\n") );
}

void SoftSequence::disable()
{
    SCOPED_LOCK(mutex);
    DEBUG(3, ("Disabling %c\n", is_enabled ? 'Y' : 'N') );
    if(!is_enabled)
        {DEBUG(3, ("Skip\n")); return;}

    is_enabled = false;

    if(hw) {
        interruptLock L;
        hw->disarm();
    }

    scanIoRequest(changed);
    DEBUG(1, ("Disabled\n") );
}

// Called from ISR context
void SoftSequence::sync()
{
    DEBUG(3, ("Syncing %c\n", is_insync ? 'Y' : 'N') );
    if(is_insync)
        {DEBUG(3, ("Skip\n")); return;}

    assert(hw);

    if(nat_ioread32(hw->ctrlreg)&EVG_SEQ_RAM_RUNNING) {
        // we may still be _ENABLED at this point, but the trigger source is set to
        // Disabled, so this makes no difference.
        epicsInterruptContextMessage("SoftSequence::sync() while running\n");
        return;
    }

    // At this point the sequencer is not running and effectively disabled.
    // From paranoia, reset it anyway
    nat_iowrite32(hw->ctrlreg, hw->ctrlreg_hw | EVG_SEQ_RAM_RESET);

    hw->ctrlreg_user &= ~(EVG_SEQ_RAM_REPEAT_MASK|EVG_SEQ_RAM_SRC_MASK);

    switch(committed.mode) {
    case Single:
        hw->ctrlreg_user |= EVG_SEQ_RAM_SINGLE;
        break;
    case Normal:
        hw->ctrlreg_user |= EVG_SEQ_RAM_NORMAL;
        break;
    }

    epicsUInt8 src;

    // default to disabled
    switch(owner->type) {
    case SeqManager::TypeEVG:
        src = 31;
        break;
    case SeqManager::TypeEVR:
        src = 63;
        break;
    default:
        return;
    }

    // paranoia: disable any external trigger mappings
    owner->mapTriggerSrc(hw->idx, 0x02000000);

    // map trigger source codes
    // MSB governs the type of mapping
    switch(committed.src&0xff000000) {
    case 0x00000000: // raw mapping
        DEBUG(5, ("  Raw mapping %x\n", committed.src));
        // LSB is code
        src = committed.src&0xff;
        break;
    case 0x01000000: // software trigger mapping
        DEBUG(5, ("  SW mapping %x\n", committed.src));
        // ignore 0x00ffffff
        switch(owner->type) {
        case SeqManager::TypeEVG:
            src = 17+hw->idx;
            break;
        case SeqManager::TypeEVR:
            src = 61;
            break;
        }
        break;
    case 0x02000000: // external trigger
        DEBUG(5, ("  EXT mapping %x\n", committed.src));
        if(owner->type==SeqManager::TypeEVG) {
            // pass through to sub-class
            owner->mapTriggerSrc(hw->idx, committed.src);
            src = 24+hw->idx;
        }
        break;
    case 0x03000000: // disable trigger
        DEBUG(5, ("  NO mapping %x\n", committed.src));
        // use default
        break;
    default:
        DEBUG(0, ("unknown sequencer trigger code %08x\n", (unsigned)committed.src));
        break;
    }
    DEBUG(5, ("  Trig Src %x\n", src));

    hw->ctrlreg_user |= src;

    // write out the RAM
    volatile epicsUInt32 *ram = static_cast<volatile epicsUInt32 *>(hw->rambase);
    for(size_t i=0, N=committed.codes.size(); i<N; i++)
    {
        epicsUInt32 codesMasks;

        DEBUG(5, ("  Code %u\n", committed.codes[i]));

        codesMasks = committed.codes[i];
        codesMasks |= committed.enables[i] << EVG_SEQ_RAM_SWENABLE_shift;
        codesMasks |= committed.masks[i] << EVG_SEQ_RAM_SWMASK_shift;

        //
        DEBUG(5, ("  Done, write %u\n", codesMasks));
        nat_iowrite32(ram++, committed.times[i]);
        nat_iowrite32(ram++, codesMasks);
        if (committed.codes[i] == 0x7f)
            break;

    }

    {
        epicsUInt32 ctrl = hw->ctrlreg_hw = hw->ctrlreg_user;
        if(is_enabled)
            ctrl |= EVG_SEQ_RAM_ARM;
        else
            ctrl |= EVG_SEQ_RAM_DISABLE; // paranoia

        DEBUG(3, ("  SeqCtrl %x\n", ctrl));
        nat_iowrite32(hw->ctrlreg, ctrl);
    }

    is_insync = true;
    DEBUG(3, ("In Sync\n") );
}


SeqManager::SeqManager(const std::string &name, Type t)
    :base_t(name)
    ,type(t)
{
    switch(type) {
    case TypeEVG:
    case TypeEVR:
        break;
    default:
        throw std::invalid_argument("Bad SeqManager type");
    }
}

SeqManager::~SeqManager() {}

mrf::Object*
SeqManager::buildSW(const std::string& name, const std::string& klass, const mrf::Object::create_args_t& args)
{
    (void)klass;

    mrf::Object::create_args_t::const_iterator it=args.find("PARENT");
    if(it==args.end())
        throw std::runtime_error("No PARENT= (EVG) specified");

    mrf::Object *mgrobj = mrf::Object::getObject(it->second);
    if(!mgrobj)
        throw std::runtime_error("No such PARENT object");

    SeqManager *mgr = dynamic_cast<SeqManager*>(mgrobj);
    if(!mgr)
        throw std::runtime_error("PARENT is not a SeqManager");

    return new SoftSequence(mgr, name);
}

// Called from ISR context
void SeqManager::doStartOfSequence(unsigned i)
{
    assert(i<hw.size());
    SeqHW* HW = hw[i];
    HW->running = true;

    SoftSequence *seq = HW->loaded;

    if(!seq) return;

    seq->numStart++;

    scanIoRequest(seq->onStart);
}

// Called from ISR context
void SeqManager::doEndOfSequence(unsigned i)
{
    assert(i<hw.size());
    SeqHW* HW = hw[i];

    HW->running = false;

    SoftSequence *seq = HW->loaded;

    if(!seq) return;

    if(seq->committed.mode==Single) {
        seq->is_enabled = false;
    }

    seq->numEnd++;

    scanIoRequest(seq->onEnd);

    if(!seq->is_insync)
        seq->sync();

    if(seq->committed.mode==Single)
        scanIoRequest(seq->changed);
}

void SeqManager::addHW(unsigned i,
                       volatile void *ctrl,
                       volatile void *ram)
{
    hw.resize(std::max(hw.size(), size_t(i+1)), 0);
    assert(!hw[i]);
    hw[i] = new SeqHW(this, i, ctrl, ram);
}

int SeqManagerDebug;

OBJECT_BEGIN(SeqManager)
  OBJECT_FACTORY(SeqManager::buildSW);
OBJECT_END(SeqManager)

extern "C" {
epicsExportAddress(int, SeqManagerDebug);
}
