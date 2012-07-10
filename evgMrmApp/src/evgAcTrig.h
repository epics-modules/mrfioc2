#ifndef EVG_AC_TRIG_H
#define EVG_AC_TRIG_H

#include <epicsTypes.h>
#include "mrf/object.h"

class evgAcTrig : public mrf::ObjectInst<evgAcTrig> {
public:
    evgAcTrig(const std::string&, volatile epicsUInt8* const);
    ~evgAcTrig();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    void setDivider(epicsUInt32);
    epicsUInt32 getDivider() const;

    void setPhase(epicsFloat64);
    epicsFloat64 getPhase() const;

    void setBypass(bool);
    bool getBypass() const;

    void setSyncSrc(bool);
    bool getSyncSrc() const;

    void setTrigEvtMap(epicsUInt16, bool);
    epicsUInt32 getTrigEvtMap() const;

private:
    volatile epicsUInt8* const m_pReg;
};

#endif //EVG_AC_TRIG_H

