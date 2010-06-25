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

epicsStatus
evgDbus::setDbusMap(epicsUInt16 map) {
	epicsUInt32 mask = map << (4 * m_id);

	epicsUInt32 dbusMap = READ32(m_pReg, DBusMap);
	printf("Old DBus Map: %08x\n", dbusMap);

	//Zeroing out the bits that belong to this Dbus
	dbusMap = dbusMap & ~(0xf << (4 * m_id));

	dbusMap = dbusMap | mask;
	printf("New DBus Map: %08x\n", dbusMap);

	WRITE32(m_pReg, DBusMap, dbusMap);

	return OK;
} 
