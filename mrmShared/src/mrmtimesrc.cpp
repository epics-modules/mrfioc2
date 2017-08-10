/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdexcept>
#include <vector>

// support for clock_nanosleep
#if _POSIX_C_SOURCE>=200112L
#  define HAVE_CNS
#  include <time.h>
#endif

#include <epicsGuard.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsThread.h>
#include <epicsTypes.h>
#include <epicsTime.h>
#include <generalTimeSup.h>

#include "mrfCommon.h"
#include "mrmtimesrc.h"

typedef epicsGuard<epicsMutex> Guard;
typedef epicsGuardRelease<epicsMutex> UnGuard;

struct TimeStampSource::Impl
{
    TimeStampSource * const owner;
    Impl(TimeStampSource *owner, double period)
        :owner(owner)
        ,timeoutRun(*this)
#ifdef HAVE_CNS
        ,softsrcRun(*this)
        ,stopsrc(true)
#endif
        ,stop(false)
        ,resync(true)
        ,okCnt(0u)
        ,lastError(-1.0)
        ,period(period*1.1) // our timeout period is 10% longer than the expected reset period
    {}
    ~Impl()
    {
        {
            Guard G(mutex);
            stop = true;
        }
        wakeup.signal();
        if(timeout.get()) timeout->exitWait();
    }

    /* We want to have a timeout on timestamps.
     *
     * That is, to detect and error if tickSecond() isn't called every second.
     * The complication is to do this in a portable manner.
     * We must also be careful to avoid depending on epicsTime
     * as this time source may stop if we stall.
     *
     * So rather hacky, but epicsEvent::wait(double) can be depended
     * upon to timeout.  So have a seperate thread to detect timeout.
     */
    void runTimeout()
    {
        Guard G(mutex);

        while(!stop) {
            bool ok;
            {
                UnGuard U(G);
                ok = wakeup.wait(1.1); // false == timeout
            }
            if(ok && okCnt<5) {
                okCnt++;
            } else if(!ok) {
                okCnt = 0u;
            }
        }
    }

#ifdef HAVE_CNS
    void runSrc()
    {
        Guard G(mutex);
        while(!stopsrc) {
            UnGuard U(G);

            timespec now;
            if(clock_gettime(CLOCK_REALTIME, &now)!=0) {
                wakeupsrc.wait(10.0);
                continue;
            }

            // try to wake up just before the start of the second.
            // reality is that we'll wake up a little later
            // TODO: tune this so that now.tv_nsec~=0 after first run
            now.tv_nsec = 999999000;

            if(clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &now, NULL)!=0) {
                wakeupsrc.wait(10.0);
                continue;
            }

            owner->setEvtCode(MRF_EVENT_TS_COUNTER_RST);

            owner->postSoftSecondsSrc();
        }
    }
#endif

    epicsMutex mutex;
    epicsEvent wakeup;
    epicsThreadRunableMethod<Impl, &Impl::runTimeout> timeoutRun;
    mrf::auto_ptr<epicsThread> timeout;

#ifdef HAVE_CNS
    epicsThreadRunableMethod<Impl, &Impl::runSrc> softsrcRun;
    mrf::auto_ptr<epicsThread> softsrc;
    bool stopsrc;
    epicsEvent wakeupsrc;
#endif

    bool stop;
    bool resync;
    unsigned okCnt;
    double lastError;
    const double period;

    epicsUInt32 next;
};

TimeStampSource::TimeStampSource(double period)
    :impl(new Impl(this, period))
{
    resyncSecond();
}

TimeStampSource::~TimeStampSource()
{
    delete impl;
}

void TimeStampSource::resyncSecond()
{
    Guard G(impl->mutex);
    impl->resync = true;
}

void TimeStampSource::tickSecond()
{
    epicsUInt32 tosend;
    bool ok;

    epicsTimeStamp ts;
    bool valid = epicsTimeOK == generalTimeGetExceptPriority(&ts, 0, ER_PROVIDER_PRIORITY);

    {
        Guard G(impl->mutex);

        ok = impl->okCnt>=5;

        /* delay re-sync request until 1Hz is stable, valid system time is available */
        if(ok && valid && impl->resync)  {
            impl->next = ts.secPastEpoch+POSIX_TIME_AT_EPICS_EPOCH+1;
            impl->resync = false;
        }

        if(ok) {
            tosend = impl->next;
        }

        impl->next++;
        ok &= tosend!=0;

        if(ok && valid) {
            impl->lastError = double(tosend) - (ts.secPastEpoch+POSIX_TIME_AT_EPICS_EPOCH);
        } else {
            impl->lastError = -1.0;
        }

        if(!impl->timeout.get()) {
            // lazy start of timestamp timeout thread
            impl->timeout.reset(new epicsThread(impl->timeoutRun,
                                                "TimeStampTimeout",
                                                epicsThreadGetStackSize(epicsThreadStackSmall)));
            impl->timeout->start();
        }
    }

    impl->wakeup.signal();

    if(!ok) return;

    for(unsigned i = 0; i < 32; tosend <<= 1, i++) {
        if( tosend & 0x80000000 )
            setEvtCode(MRF_EVENT_TS_SHIFT_1);
        else
            setEvtCode(MRF_EVENT_TS_SHIFT_0);
    }
}

bool TimeStampSource::validSeconds() const
{
    Guard G(impl->mutex);
    return impl->okCnt>=5;
}

double TimeStampSource::deltaSeconds() const
{
    Guard G(impl->mutex);
    return impl->lastError;
}


void TimeStampSource::softSecondsSrc(bool enable)
{
#ifdef HAVE_CNS
    Guard G(impl->mutex);
    if(enable && !impl->softsrc.get()) {
        // start it
        impl->stopsrc = false;
        impl->softsrc.reset(new epicsThread(impl->softsrcRun,
                                            "SoftTimeSrc",
                                            epicsThreadGetStackSize(epicsThreadStackSmall),
                                            epicsThreadPriorityHigh));
        impl->softsrc->start();

        resyncSecond();

    } else if(!enable && impl->softsrc.get()) {
        impl->stopsrc = true;
        {
            UnGuard U(G);
            impl->wakeup.signal();
            impl->softsrc->exitWait();
        }
        impl->softsrc.reset();

    }
#else
    if(enable)
        throw std::runtime_error("Soft timestamp source not supported");
#endif
}

bool TimeStampSource::isSoftSeconds() const
{
#ifdef HAVE_CNS
    Guard G(impl->mutex);
    return !!impl->softsrc.get();
#else
    return false;
#endif
}

std::string TimeStampSource::nextSecond() const
{
    epicsTimeStamp raw;
    {
        Guard G(impl->mutex);
        raw.secPastEpoch = impl->next - POSIX_TIME_AT_EPICS_EPOCH;
        raw.nsec = 0;
    }
    epicsTime time(raw);

    std::vector<char> buf(40);

    buf.resize(time.strftime(&buf[0], buf.size(), "%a, %d %b %Y %H:%M:%S"));
    // buf.size() doesn't include trailing nil

    return std::string(&buf[0], buf.size());
}
