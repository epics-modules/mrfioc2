#ifndef EVG_SOFTEVT_H
#define EVG_SOFTEVT_H

#include <epicsTypes.h>
#include <epicsMutex.h>
#include "mrf/object.h"

class evgSoftEvt : public mrf::ObjectInst<evgSoftEvt> {

public:
    evgSoftEvt(const std::string&, volatile epicsUInt8* const);

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    void enable(bool);
    bool enabled() const;

    bool pend() const;

    void setEvtCode(epicsUInt32);
    epicsUInt32 getEvtCode() const;

private:
    volatile epicsUInt8* const m_pReg;
    epicsMutex                 m_lock;

};

#endif //EVG_SOFTEVT_H
