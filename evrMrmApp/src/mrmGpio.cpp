#include "mrmGpio.h"

MRMGpio::MRMGpio(EVRMRM *o)
{
    owner_ = o;
    lock_ = epicsMutexMustCreate();
    printf("GPIO init done\n");
}

epicsUInt32 MRMGpio::getDirection()
{
    epicsUInt32 val = READ32(owner_->base, GPIODir);
    printf("Reading direction: %x\n", val);
    return val;
}

void MRMGpio::setDirection(epicsUInt32 val)
{
    printf("setting direction: %x\n", val);
    WRITE32(owner_->base, GPIODir, val);
}

epicsUInt32 MRMGpio::read()
{
    epicsUInt32 val = READ32(owner_->base, GPIOIn);
    printf("reading input: %x\n", val);
    return val;
}

epicsUInt32 MRMGpio::getOutput()
{
    epicsUInt32 val = READ32(owner_->base, GPIOOut);
    printf("reading output: %x\n", val);
    return val;
}

void MRMGpio::setOutput(epicsUInt32 val)
{
    //printf("writing output: %x\n", val);
    WRITE32(owner_->base, GPIOOut, val);
}

void MRMGpio::lock(){
    epicsMutexMustLock(lock_);
}

void MRMGpio::unlock(){
    epicsMutexUnlock(lock_);
}
