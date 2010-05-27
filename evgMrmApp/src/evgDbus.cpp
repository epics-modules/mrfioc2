#include "evgDbus.h"

#include <iostream>

#include <mrfCommonIO.h> 
#include <errlog.h> 
#include <mrfCommon.h> 

#include "evgRegMap.h"

evgDbus::evgDbus(const epicsUInt32 id, volatile epicsUInt8* const pReg):
id(id),
pReg(pReg) {
}

epicsStatus
evgDbus::setDbusMap(epicsUInt16 map) {
	epicsUInt32 dbusMap = map << (4 * id);
	WRITE32(pReg, DBusMap, dbusMap);
	return OK;
} 