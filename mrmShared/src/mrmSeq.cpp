/*************************************************************************\
* Copyright (c) 2016 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdio.h>

#include <epicsMutex.h>
#include <errlog.h>

#include <mrfCommonIO.h>

#include "mrmSeq.h"

#include <epicsExport.h>

#define  EVG_SEQ_RAM_RUNNING    0x02000000  // Sequence RAM is Running (read only)
#define  EVG_SEQ_RAM_ENABLED    0x01000000  // Sequence RAM is Enabled (read only)

//TODO: external enable/trigger bits and hanlding?
#define  EVG_SEQ_RAM_SW_TRIG    0x00200000  // Sequence RAM Software Trigger Bit
#define  EVG_SEQ_RAM_RESET      0x00040000  // Sequence RAM Reset
#define  EVG_SEQ_RAM_DISABLE    0x00020000  // Sequence RAM Disable
#define  EVG_SEQ_RAM_ARM        0x00010000  // Sequence RAM Enable/Arm

#define  EVG_SEQ_RAM_WRITABLE_MASK 0x00ffffff
#define  EVG_SEQ_RAM_REPEAT_MASK 0x00180000 // Sequence RAM Repeat Mode Mask
#define  EVG_SEQ_RAM_NORMAL     0x00000000  // Normal Mode: Repeat every trigger
#define  EVG_SEQ_RAM_SINGLE     0x00100000  // Single-Shot Mode: Disable on completion
#define  EVG_SEQ_RAM_RECYCLE    0x00080000  // Continuous Mode: Repeat on completion

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
    epicsUInt32 ctrlreg_shadow;

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
        ,ctrlreg_shadow(nat_ioread32(ctrlreg)&0x001800ff)
    {
        nat_iowrite32(ctrlreg, ctrlreg_shadow | EVG_SEQ_RAM_RESET);
    }

    bool disarm()
    {
        interruptLock I;

        /** Interesting race conditions possible on non-RTOS where ISR don't preempt.
         * It is then possible that disarm() can be called after a sequencer has finished
         * running, but before the ISR runs for Start of Sequence interrupt.
         * So we also test for SoS
         */

        nat_iowrite32(ctrlreg, ctrlreg_shadow | EVG_SEQ_RAM_DISABLE);
        bool isrun = nat_ioread32(ctrlreg) & EVG_SEQ_RAM_RUNNING;
        bool started = owner->testStartOfSeq() & (1u<<idx);
        // TODO: test for start of sequence as well?

        return isrun || started || running;
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

//    void setTimestampResolution(epicsUInt32 res);
//    epicsUInt32 getTimestampResolution() const;

    // dset actions
    // update scratch
    // read back committed

    void setTimestamp(const double *arr, epicsUInt32 count)
    {
        times_t times(count);
        // check for monotonic
        // TODO: not handling overflow (HW supports controlled rollover w/ special 0xffffffff times)
        for(epicsUInt32 i=0; i<count; i++)
        {
            //TODO: unit conversion
            times[i] = arr[i];
            if(i>0 && times[i]<=times[i-1])
                throw std::runtime_error("Non-monotonic timestamp array");
            else if(times[i]==0xffffffff)
                throw std::runtime_error("Time overflow, rollover not supported");
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
        epicsUInt32 ret = std::min(size_t(count), committed.times.size());
        std::copy(committed.times.begin(),
                  committed.times.begin()+ret,
                  arr);
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
            throw std::runtime_error("Unknown sequencer run mode");
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

    struct Config {
        times_t times;
        codes_t codes;
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
  OBJECT_PROP1("SOFT_TRIG", &SoftSequence::softTrig);
  OBJECT_PROP2("TIMES", &SoftSequence::getTimestamp, &SoftSequence::setTimestamp);
  OBJECT_PROP1("TIMES", &SoftSequence::stateChange);
  OBJECT_PROP2("CODES", &SoftSequence::getEventCode, &SoftSequence::setEventCode);
  OBJECT_PROP1("CODES", &SoftSequence::stateChange);
  OBJECT_PROP1("NUM_RUNS", &SoftSequence::counterEnd);
  OBJECT_PROP1("NUM_RUNS", &SoftSequence::counterEndScan);
  OBJECT_PROP1("NUM_STARTS", &SoftSequence::counterStart);
  OBJECT_PROP1("NUM_STARTS", &SoftSequence::counterStartScan);
//  OBJECT_PROP2("TIMEMODE", &SoftSequence::getTimestampInpMode, &SoftSequence::setTimestampInpMode);
//  OBJECT_PROP2("TIMEUNITS", &SoftSequence::getTimestampResolution, &SoftSequence::setTimestampResolution);
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

    nat_iowrite32(hw->ctrlreg, hw->ctrlreg_shadow | EVG_SEQ_RAM_SW_TRIG);
    DEBUG(2, ("SW Triggered\n") );
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
    }

    if(!hw) {
        last_err = "All HW Seq. in use";
        scanIoRequest(onErr);
        throw alarm_exception(MAJOR_ALARM, WRITE_ALARM);
    }

    // paranoia: disable any external trigger mappings
    owner->mapTriggerSrc(hw->idx, 0x02000000);

    if(!hw->disarm()) {
        interruptLock L;
        sync();
    }

    scanIoRequest(changed);
    DEBUG(1, ("Loaded\n") );
}

void SoftSequence::unload()
{
    SCOPED_LOCK(mutex);
    DEBUG(3, ("Unloading %c\n", hw ? 'L' : 'U') );

    if(!hw) {DEBUG(3, ("Skip\n")); return;}

    hw->disarm();

    {
        interruptLock L;
        assert(hw->loaded=this);
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
    DEBUG(1, ("Committing 1\n") );

    // scratch.times already check for monotonic

    Config conf(scratch); // vector copies

    size_t buflen = std::min(conf.codes.size(),
                             conf.times.size());
    conf.codes.resize(buflen);
    conf.times.resize(buflen);

    // ensure presence of trailing end of sequence marker event 0x7f
    if(conf.codes.empty() || conf.codes.back()!=0x7f)
    {
        if(!conf.times.empty() && conf.times.back()==0xffffffff)
            throw std::runtime_error("Wow, input array is missing 0x7f and maxing out times");

        conf.codes.push_back(0x7f);

        if(conf.times.empty())
            conf.times.push_back(0);
        else
            conf.times.push_back(conf.times.back()+1);
    }
    DEBUG(1, ("Committing 2\n") );

    if(conf.times.size()>2048)
        throw std::runtime_error("Sequence too long");

    {
        interruptLock L;
        committed.swap(conf);
        is_committed = true;
        is_insync = false;
    }
    DEBUG(1, ("Committing 3\n") );

    if(hw && !hw->disarm()) {
        assert(hw->loaded==this);
        interruptLock L;
        sync();
    }

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

        nat_iowrite32(hw->ctrlreg, hw->ctrlreg_shadow | EVG_SEQ_RAM_ARM);
    }
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

    if(hw)
        hw->disarm();

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

    if(nat_ioread32(hw->ctrlreg)&0x03000000) {
        epicsInterruptContextMessage("SoftSequence::sync() while running/enabled");
        return;
    }

    // At this point the sequencer is disabled and not running.
    // From paranoia, reset it anyway
    nat_iowrite32(hw->ctrlreg, hw->ctrlreg_shadow | EVG_SEQ_RAM_RESET);

    hw->ctrlreg_shadow &= ~(EVG_SEQ_RAM_REPEAT_MASK|EVG_SEQ_RAM_SRC_MASK);

    switch(committed.mode) {
    case Single:
        hw->ctrlreg_shadow |= EVG_SEQ_RAM_SINGLE;
        break;
    case Normal:
        hw->ctrlreg_shadow |= EVG_SEQ_RAM_NORMAL;
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

    hw->ctrlreg_shadow |= src;

    // write out the RAM
    volatile epicsUInt32 *ram = static_cast<volatile epicsUInt32 *>(hw->rambase);
    for(size_t i=0, N=committed.codes.size(); i<N; i++)
    {
        nat_iowrite32(ram++, committed.times[i]);
        nat_iowrite32(ram++, committed.codes[i]);
        if(committed.codes[i]==0x7f)
            break;
    }

    {
        epicsUInt32 ctrl = hw->ctrlreg_shadow;
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
