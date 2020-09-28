#ifndef POLLIRQ_H
#define POLLIRQ_H

#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <mrf/mrfCommonAPI.h>

extern "C" {
    typedef void (*pollerFN)(void *);
}

class MRFCOMMON_API IRQPoller : protected epicsThreadRunable {

    epicsEvent evt;
    epicsMutex lock;
    bool done;
    const double period;

    const pollerFN fn;
    void * const arg;

    epicsThread runner;

    virtual void run();
public:
    IRQPoller(pollerFN fn, void *arg, double period);
    virtual ~IRQPoller();

private:
    IRQPoller(const IRQPoller&);
    IRQPoller& operator=(const IRQPoller&);
};

#endif // POLLIRQ_H
