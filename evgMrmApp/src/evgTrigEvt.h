#ifndef EVG_TRIGEVT_H
#define EVG_TRIGEVT_H

#include <epicsTypes.h>
#include "mrf/object.h"

class evgTrigEvt : public mrf::ObjectInst<evgTrigEvt> {
public:
    evgTrigEvt(const std::string&, const epicsUInt32, volatile epicsUInt8* const);
    ~evgTrigEvt();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    void setEvtCode(epicsUInt32);
    epicsUInt32 getEvtCode() const;

private:
    const epicsUInt32          m_id;
    volatile epicsUInt8* const m_pReg;
};

#endif //EVG_TRIGEVT_H
