#ifndef EVG_TRIGEVT_H
#define EVG_TRIGEVT_H

#include <epicsTypes.h>
#include <dbScan.h>
#include "mrf/object.h"

class evgMrm;

class evgTrigEvt : public mrf::ObjectInst<evgTrigEvt> {
public:
    evgTrigEvt(const std::string&, const epicsUInt32, volatile epicsUInt8* const, evgMrm*);
    ~evgTrigEvt();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    void setEvtCode(epicsUInt32);
    epicsUInt32 getEvtCode() const;

    void setSource(epicsUInt32);
    epicsUInt32 getSource() const;

    IOSCANPVT stateChange() const { return changed; }

private:
    const epicsUInt32          m_id;
    volatile epicsUInt8* const m_pReg;
    evgMrm* const              m_owner;
    IOSCANPVT                  changed;
};

#endif //EVG_TRIGEVT_H
