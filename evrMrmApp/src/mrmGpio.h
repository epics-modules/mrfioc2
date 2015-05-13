#ifndef MRMGPIO_H
#define MRMGPIO_H

#include "evrRegMap.h"
#include "epicsTypes.h"
#include "mrfCommonIO.h"

#include <epicsMutex.h>

class EVRMRM;

class MRMGpio{
public:
    MRMGpio(EVRMRM&);

    epicsUInt32 getDirection(); // returns direction gpio register
    void setDirection(epicsUInt32); // sets direction gpio register

    epicsUInt32 read(); // returns input gpio register

    epicsUInt32 getOutput(); // reads the data from the output register
    void setOutput(epicsUInt32); // writes the data to the output register

    // mutex for locking access to GPIO pins.
    epicsMutex lock_;

private:
    EVRMRM& owner_;
};

#endif // MRMGPIO_H
