#ifndef EVG_MXC_H
#define EVG_MXC_H

#include <epicsTypes.h>
#include "mrf/object.h"

class evgMrm;

class evgMxc : public mrf::ObjectInst<evgMxc> {
    OBJECT_DECL(evgMxc);
public:
    evgMxc(const std::string&, const epicsUInt32, evgMrm* const);
    ~evgMxc();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    bool getStatus() const;

    void setPolarity(bool);
    bool getPolarity() const;

    void setPrescaler(epicsUInt32);
    epicsUInt32 getPrescaler() const;

    void setFrequency(epicsFloat64);
    epicsFloat64 getFrequency() const;

    void setTrigEvtMap(epicsUInt16, bool);
    bool getTrigEvtMap(epicsUInt16) const;

private:
    const epicsUInt32          m_id;
    evgMrm* const              m_owner;
    volatile epicsUInt8* const m_pReg;

};

#endif//EVG_MXC_H
