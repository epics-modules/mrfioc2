#include "evgDbus.h"

#include <iostream>

#include <mrfCommonIO.h> 
#include <errlog.h> 
#include <mrfCommon.h> 

#include "evgRegMap.h"

evgDbus::evgDbus(const epicsUInt32 id, volatile epicsUInt8* const pReg):
m_id(id),
m_pReg(pReg) {
}

evgDbus::~evgDbus() {
}

epicsStatus
evgDbus::setDbusMap(epicsUInt16 map) {
    epicsUInt32 mask = map << (4 * m_id);

    //Read-Modify-Write
    epicsUInt32 dbusMap = READ32(m_pReg, DBusMap);

    //Zeroing out the bits that belong to this Dbus
    dbusMap = dbusMap & ~(0xf << (4 * m_id));

    dbusMap = dbusMap | mask;

    WRITE32(m_pReg, DBusMap, dbusMap);

    return OK;
}
 
