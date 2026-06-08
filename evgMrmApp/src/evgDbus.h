#ifndef EVG_DBUS_H
#define EVG_DBUS_H

#include <epicsTypes.h>
#include <dbScan.h>
#include "mrf/object.h"

class evgMrm;

class evgDbus : public mrf::ObjectInst<evgDbus> {
public:
    evgDbus(const std::string&, const epicsUInt32,  volatile epicsUInt8* const, evgMrm*);
    ~evgDbus();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    void setSource(epicsUInt32);
    epicsUInt32 getSource() const;

    IOSCANPVT stateChange() const { return changed; }

private:
    const epicsUInt32          m_id;
    volatile epicsUInt8* const m_pReg;
    evgMrm* const              m_owner;
    IOSCANPVT                  changed;
};

#endif //EVG_DBUS_H
