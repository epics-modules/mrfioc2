
#include "evr230pmc.hpp"

EVR230PMC(const epicsPCIDevice* d) :
   EVR()
  ,scanRecvError()
  ,countRecvError(0)
  ,dev(d)
{
  scanIoInit(&scanRecvError);

  // Try to identify the hardware

  
}

EVR230PMC::~EVR230PMC()
{
}

bool
EVR230PMC::enabled()
{
}

void
EVR230PMC::enable(bool)
{
}

epicsUint32
EVR230PMC::eventClock()
{
}

epicsUint32
EVR230PMC::eventClockDiv()
{
}

void
EVR230PMC::setEventClock(epicsUint32,epicsUint32)
{
}

void
EVR230PMC::tsLatch()
{
}

void
EVR230PMC::tsLatchReset()
{
}

epicsUInt32
EVR230PMC::tsLatchSec()
{
}

epicsUInt32
EVR230PMC::tsLatchCount()
{
}
