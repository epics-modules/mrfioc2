
#include <epicsMutex.h>

#include <mrfCommonIO.h>

#include "mrmSeq.h"

#define  EVG_SEQ_RAM_RUNNING    0x02000000  // Sequence RAM is Running (read only)
#define  EVG_SEQ_RAM_ENABLED    0x01000000  // Sequence RAM is Enabled (read only)

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

        nat_iowrite32(ctrlreg, ctrlreg_shadow | EVG_SEQ_RAM_DISABLE);
        bool isrun = nat_ioread32(ctrlreg) & EVG_SEQ_RAM_RUNNING;

        return isrun || running;
    }

    void enable()
    {
        interruptLock I;

        nat_iowrite32(ctrlreg, ctrlreg_shadow | EVG_SEQ_RAM_ARM);
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

    void setTrigSrc(epicsInt32 src)
    {
        {
            SCOPED_LOCK(mutex);
            scratch.src = src;
            is_committed = false;
        }
        scanIoRequest(changed);
    }

    epicsInt32 getTrigSrcCt() const
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
        Config() :mode(Single), src(0x0100) {}
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
  OBJECT_FACTORY(SeqManager::buildSW);
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
    SCOPED_LOCK(mutex);
    if(hw || !is_enabled) return;

    nat_iowrite32(hw->ctrlreg, hw->ctrlreg_shadow | EVG_SEQ_RAM_SW_TRIG);
}

void SoftSequence::load()
{
    SCOPED_LOCK(mutex);
    if(hw) return;

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

    owner->mapTriggerSrc(hw->idx, 0);

    if(!hw->disarm()) {
        interruptLock L;
        sync();
    }

    scanIoRequest(changed);
}

void SoftSequence::unload()
{
    SCOPED_LOCK(mutex);

    if(!hw) return;

    hw->disarm();

    {
        interruptLock L;
        assert(hw->loaded=this);
        hw->loaded = NULL;

        hw = NULL;

        is_insync = false;
    }

    scanIoRequest(changed);
}

void SoftSequence::commit()
{
    SCOPED_LOCK(mutex);

    if(is_committed) return;

    // scratch.times already check for monotonic

    Config conf(scratch); // vector copies

    size_t buflen = std::min(conf.codes.size(),
                             conf.times.size());
    conf.codes.resize(buflen);
    conf.times.resize(buflen);

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

    if(conf.times.size()>2048)
        throw std::runtime_error("Sequence too long");

    {
        interruptLock L;
        committed.swap(conf);
        is_committed = true;
        is_insync = false;
    }

    if(!hw) return;
    assert(hw->loaded==this);

    if(!hw->disarm()) {
        interruptLock L;
        sync();
    }

    scanIoRequest(changed);
}

void SoftSequence::enable()
{
    SCOPED_LOCK(mutex);
    if(is_enabled)
        return;

    is_enabled = true;

    if(!hw) return;

    hw->enable();
}

void SoftSequence::disable()
{
    SCOPED_LOCK(mutex);
    if(!is_enabled)
        return;

    is_enabled = false;

    if(!hw) return;

    hw->disarm();
}

// Called from ISR context
void SoftSequence::sync()
{
    if(is_insync)
        return;

    assert(hw);
    // At this point the sequencer is disabled and not running

    if(nat_ioread32(hw->ctrlreg)&0x03000000) {
        epicsInterruptContextMessage("SoftSequence::sync() while running/enabled");
        return;
    }

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
        assert(0);
        return;
    }

    // map trigger source codes
    switch(committed.src>>8) {
    case 0: // raw mapping
        src = committed.src&0xff;
        break;
    case 1: // software trigger mapping
        switch(owner->type) {
        case SeqManager::TypeEVG:
            src = 1u<<(17+hw->idx);
            break;
        case SeqManager::TypeEVR:
            src = 62;
            break;
        }
        break;
    case 2: // external trigger
        if(owner->type==SeqManager::TypeEVG) {
            owner->mapTriggerSrc(hw->idx, committed.src);
            src = 1u<<(24+hw->idx);
        }
        break;
    }

    hw->ctrlreg_shadow |= src;

    volatile epicsUInt32 *ram = static_cast<volatile epicsUInt32 *>(hw->rambase);
    for(size_t i=0, N=committed.codes.size(); i<N; i++)
    {
        nat_iowrite32(ram++, committed.times[i]);
        nat_iowrite32(ram++, committed.codes[i]);
    }

    epicsUInt32 ctrl = hw->ctrlreg_shadow;
    if(is_enabled)
        ctrl |= EVG_SEQ_RAM_ARM;
    else
        ctrl |= EVG_SEQ_RAM_DISABLE; // paranoia

    nat_iowrite32(hw->ctrlreg, ctrl);

    is_insync = true;
}


SeqManager::SeqManager(Type t)
    :type(t)
{}

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
}

void SeqManager::addHW(unsigned i,
                       volatile void *ctrl,
                       volatile void *ram)
{
    hw.resize(std::max(hw.size(), size_t(i+1)), 0);
    assert(!hw[i]);
    hw[i] = new SeqHW(this, i, ctrl, ram);
}
