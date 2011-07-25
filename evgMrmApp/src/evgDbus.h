#ifndef EVG_DBUS_H
#define EVG_DBUS_H

#include <epicsTypes.h>
#include "mrf/object.h"

class evgDbus : public mrf::ObjectInst<evgDbus> {
public:
    evgDbus(const std::string&, const epicsUInt32,  volatile epicsUInt8* const);
    ~evgDbus();
    
    void setSource(epicsUInt16);
    epicsUInt16 getSource() const;

private:
    const epicsUInt32          m_id;
    volatile epicsUInt8* const m_pReg;
    
};

#endif //EVG_DBUS_H
