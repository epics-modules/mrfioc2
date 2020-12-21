/*************************************************************************\
* Copyright (c) 2020 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <algorithm>

#include "drvem.h"
#include "drvemTSBuffer.h"
#include "devObj.h"

EVRMRMTSBuffer::EVRMRMTSBuffer(const std::string &n, EVRMRM *evr)
    :base_t(n)
    ,evr(evr)
    ,dropped(0u)
    ,timeEvt(0u)
    ,flushEvt(0u)
    ,active(0u)
{
    OBJECT_INIT;

    scanIoInit(&scan);
}

EVRMRMTSBuffer::~EVRMRMTSBuffer()
{

}

void EVRMRMTSBuffer::lock() const
{
    evr->lock();
}

void EVRMRMTSBuffer::unlock() const
{
    evr->unlock();
}

void EVRMRMTSBuffer::flushTimeSet(epicsUInt16 v)
{
    if(v==timeEvt)
        return;
    if(v==flushEvent() || v>0xff)
        throw std::invalid_argument("Can't capture with flush code or >255");

    if(timeEvt) {
        evr->events[timeEvt].tbufs.erase(this);
        evr->interestedInEvent(timeEvt, false);
    }
    if(v) {
        evr->interestedInEvent(v, true);
        evr->events[v].tbufs.insert(this);
    }
    timeEvt = v;
}

void EVRMRMTSBuffer::flushEventSet(epicsUInt16 v)
{
    if(v==flushEvt)
        return;
    if(v==timeEvt || v>0xff)
        throw std::invalid_argument("Can't flush with capture code or >255");

    if(flushEvt) {
        evr->events[flushEvt].tbufs.erase(this);
        evr->interestedInEvent(flushEvt, false);
    }
    if(v) {
        evr->interestedInEvent(v, true);
        evr->events[v].tbufs.insert(this);
    }
    flushEvt = v;
}

// caller must hold evrLock
void EVRMRMTSBuffer::flushNow()
{
    if(epicsTimeGetCurrent(&ebufs[active].flushtime)) {
        ebufs[active].flushtime.secPastEpoch = 0u;
        ebufs[active].flushtime.nsec = 0u;
        ebufs[active].ok = false;
        ebufs[active].drop = false;
    }

    doFlush();
}

void EVRMRMTSBuffer::doFlush()
{
    bool prevok = ebufs[active].ok;
    epicsTimeStamp prevflushtime = ebufs[active].flushtime;

    active ^= 1u;

    ebufs[active].pos = 0u;
    // a valid buffer requires timestamp validity at start and end flush
    ebufs[active].ok = evr->TimeStampValid();
    ebufs[active].drop = false;

    ebufs[active].prevok = prevok;
    ebufs[active].prevflushtime = prevflushtime;

    scanIoRequest(scan);
}

enum TimesRef {
    TimesRefEvt0,
    TimesRefFlush,
    TimesRefPrevFlush,
};

static
epicsUInt32 getTimes(const EVRMRMTSBuffer* self, epicsInt32 *arr, epicsUInt32 count, TimesRef ref)
{
    const EVRMRMTSBuffer::ebuf_t& readout = self->ebufs[self->active^1u];
    dbCommon* prec = CurrentRecord::get();

    if(prec && !readout.ok) {
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
    } else if(prec && ref==TimesRefPrevFlush && !readout.prevok) {
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
    } else if(prec && readout.drop) {
        recGblSetSevr(prec, READ_ALARM, MAJOR_ALARM);
    }

    double period=1e9/self->evr->clockTS(); // in nanoseconds

    size_t len = std::min(readout.pos, size_t(count));

    if(readout.buf.size() < count) {
        const_cast<EVRMRMTSBuffer::ebuf_t&>(readout).buf.resize(count);
    }

    if(period<=0 || !isfinite(period)) {
        if(count>0u) {
            arr[0] = 0;
            count = 1u;
        }
        recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return count;
    }

    epicsTimeStamp tref;
    if(ref==TimesRefFlush) {
        tref = readout.flushtime;
    } else if(ref==TimesRefPrevFlush) {
        tref = readout.prevflushtime;
    }

    for(size_t i=0; i<len; i++) {
        epicsTimeStamp ts = readout.buf[i];

        // readout.ok captures validity of timestamp at start and end of interval.
        // skip validation of timestamps in between.
        ts.secPastEpoch -= POSIX_TIME_AT_EPICS_EPOCH;
        ts.nsec = epicsUInt32(ts.nsec*period);

        if(i==0 && ref==TimesRefEvt0) {
            tref = ts;
        }

        double diff = epicsTimeDiffInSeconds(&ts, &tref);
        arr[i] = epicsInt32(diff*1e9); // ns
    }

    if(prec) {
        prec->time = tref;
    }

    return len;
}

epicsUInt32 EVRMRMTSBuffer::getTimesRelFirst(epicsInt32 *arr, epicsUInt32 count) const
{
    return getTimes(this, arr, count, TimesRefEvt0);
}

epicsUInt32 EVRMRMTSBuffer::getTimesRelFlush(epicsInt32 *arr, epicsUInt32 count) const
{
    return getTimes(this, arr, count, TimesRefFlush);
}

epicsUInt32 EVRMRMTSBuffer::getTimesRelPrevFlush(epicsInt32 *arr, epicsUInt32 count) const
{
    return getTimes(this, arr, count, TimesRefPrevFlush);
}

static
mrf::Object*
buildInstance(const std::string& name, const std::string& klass, const mrf::Object::create_args_t& args)
{
    (void)klass;
    mrf::Object::create_args_t::const_iterator it;

    if((it=args.find("PARENT"))==args.end())
        throw std::runtime_error("No PARENT= (EVR) specified");

    mrf::Object *evrobj = mrf::Object::getObject(it->second);
    if(!evrobj)
        throw std::runtime_error("No such PARENT object");

    EVRMRM *evr = dynamic_cast<EVRMRM*>(evrobj);
    if(!evr)
        throw std::runtime_error("PARENT is not a MRMEVR");

    mrf::auto_ptr<EVRMRMTSBuffer> ret(new EVRMRMTSBuffer(name, evr));

    return ret.release();
}

OBJECT_BEGIN(EVRMRMTSBuffer)
    OBJECT_FACTORY(&buildInstance);
    OBJECT_PROP1("DropCnt", &EVRMRMTSBuffer::dropCount);
    OBJECT_PROP2("TimeEvent", &EVRMRMTSBuffer::timeEvent, &EVRMRMTSBuffer::flushTimeSet);
    OBJECT_PROP2("FlushEvent", &EVRMRMTSBuffer::flushEvent, &EVRMRMTSBuffer::flushEventSet);
    OBJECT_PROP1("FlushManual", &EVRMRMTSBuffer::flushNow);
    OBJECT_PROP1("TimesRelFirst", &EVRMRMTSBuffer::getTimesRelFirst);
    OBJECT_PROP1("TimesRelFirst", &EVRMRMTSBuffer::flushed);
    OBJECT_PROP1("TimesRelFlush", &EVRMRMTSBuffer::getTimesRelFlush);
    OBJECT_PROP1("TimesRelFlush", &EVRMRMTSBuffer::flushed);
    OBJECT_PROP1("TimesRelPrevFlush", &EVRMRMTSBuffer::getTimesRelPrevFlush);
    OBJECT_PROP1("TimesRelPrevFlush", &EVRMRMTSBuffer::flushed);
OBJECT_END(EVRMRMTSBuffer)
