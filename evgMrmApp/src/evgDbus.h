#ifndef EVGDBUS_H
#define EVGDBUS_H

#include <epicsTypes.h>

class evgDbus {
public:
	evgDbus(const epicsUInt32, volatile epicsUInt8* const);
	~evgDbus();
	
	epicsStatus setDbusMap(epicsUInt16);

private:
	const epicsUInt32 			id;
	volatile epicsUInt8* const	pReg;
	
};

#endif //EVGDBUS_H