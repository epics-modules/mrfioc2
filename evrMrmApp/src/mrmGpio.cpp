#include "mrmGpio.h"
#include "drvem.h"

MRMGpio::MRMGpio(EVRMRM &o): owner_(o)
{
}

epicsUInt32 MRMGpio::getDirection()
{
    epicsUInt32 val = READ32(owner_.base, GPIODir);
    return val;
}

void MRMGpio::setDirection(epicsUInt32 val)
{
    WRITE32(owner_.base, GPIODir, val);
}

epicsUInt32 MRMGpio::read()
{
    epicsUInt32 val = READ32(owner_.base, GPIOIn);
    return val;
}

epicsUInt32 MRMGpio::getOutput()
{
    epicsUInt32 val = READ32(owner_.base, GPIOOut);
    return val;
}

void MRMGpio::setOutput(epicsUInt32 val)
{
    WRITE32(owner_.base, GPIOOut, val);
}
