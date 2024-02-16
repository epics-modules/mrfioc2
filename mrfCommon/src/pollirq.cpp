
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>
#include <epicsGuard.h>
#define epicsExportSharedSymbols
#include "mrf/pollirq.h"

IRQPoller::IRQPoller(pollerFN fn, void* arg, double period) :
  done(false),
  period(period),
  fn(fn),
  arg(arg),
  runner(*this, "IRQPoller",
        epicsThreadGetStackSize(epicsThreadStackBig),
        epicsThreadPriorityHigh)
{
    runner.start();
}

IRQPoller::~IRQPoller()
{
    {
        epicsGuard<epicsMutex> G(lock);
        done = true;
    }
    runner.exitWait();
}

void IRQPoller::run()
{
    epicsGuard<epicsMutex> G(lock);
    while(!done) {
        double P = period;
        {
            epicsGuardRelease<epicsMutex> U(G);
            epicsThreadSleep(P);
        }

        (*fn)(arg);
    }
}
