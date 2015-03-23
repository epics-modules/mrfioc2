#ifndef DELAYMODULE_H
#define DELAYMODULE_H

#include "evr/util.h"
#include "mrf/object.h"


#include <epicsTypes.h>

class EVRMRM;
class MRMGpio;

class DelayModule : public mrf::ObjectInst<DelayModule>
{
public:
    DelayModule(const std::string&, EVRMRM*, unsigned int);

    // helpers
    void setDelay0(epicsUInt16 r){dly0_ = r; setDelay(true, false, r, 0);}
    epicsUInt16 getDelay0() const {return dly0_;}
    void setDelay1(epicsUInt16 r){dly1_ = r; setDelay(false, true, 0, r);}
    epicsUInt16 getDelay1() const {return dly1_;}

    void set(bool a, bool b, epicsUInt16 c, epicsUInt16 d){setDelay(a,b,c,d);}
    void enable();
    void disable();
    void setDelay(epicsUInt16, epicsUInt16);
    void setDelay(bool, bool, epicsUInt16, epicsUInt16);

private:
    //will only change these GPIO bits. The rest do not belong to this Delay Module
    const epicsUInt32 serialData_;
    const epicsUInt32 serialClock_;
    const epicsUInt32 transferLatchClock_;
    const epicsUInt32 outputDisable_;

    //EVRMRM *owner_;
    //const unsigned int N_;
    MRMGpio *gpio_;
    epicsUInt32 dly0_, dly1_;

    void setGpioOutput();
    void pushData(epicsUInt32);

    void lock() const {}
    void unlock() const {}
};

#endif // DELAYMODULE_H
