#ifndef EVG_AC_TRIG_H
#define EVG_AC_TRIG_H

#include <epicsTypes.h>

class evgAcTrig {
public:
    evgAcTrig(volatile epicsUInt8* const);
    ~evgAcTrig();

    epicsStatus setDivider(epicsUInt32);
    epicsStatus setPhase(epicsFloat64);
    epicsStatus bypass(bool);
    epicsStatus setSyncSrc(bool);
    epicsStatus setTrigEvtMap(epicsUInt16, bool);

private:
	volatile epicsUInt8* const m_pReg;
};

#endif //EVG_AC_TRIG_H

