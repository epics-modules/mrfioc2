#include "evgDbus.h"

#include <iostream>

#include <mrfCommonIO.h> 
#include <errlog.h> 
#include <mrfCommon.h> 

#include "evgRegMap.h"

evgDbus::evgDbus(const std::string& name, const epicsUInt32 id,
                 volatile epicsUInt8* const pReg)
    :mrf::ObjectInst<evgDbus>(name)
    ,m_id(id)
    ,m_pReg(pReg)
{
    OBJECT_INIT;
}

evgDbus::~evgDbus() {
}

void
evgDbus::setSource(epicsUInt16 src) {
    epicsUInt32 mask = src << (4 * m_id);

    //Read-Modify-Write
    epicsUInt32 dbusSrc = READ32(m_pReg, DBusSrc);

    //Zeroing out the bits that belong to this Dbus
    dbusSrc = dbusSrc & ~(0xf << (4 * m_id));

    dbusSrc = dbusSrc | mask;
    WRITE32(m_pReg, DBusSrc, dbusSrc);
}

epicsUInt16
evgDbus::getSource() const {
    epicsUInt32 dbusSrc = READ32(m_pReg, DBusSrc);
    return dbusSrc & (0xf << (4 * m_id));
}
 
