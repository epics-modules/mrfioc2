#ifndef EVGDBUS_H
#define EVGDBUS_H

#include <epicsTypes.h>

class evgDbus {
public:
	evgDbus(const epicsUInt32, const volatile epicsUInt8*);
	~evgDbus();
	
	epicsStatus setDbusMap(epicsUInt16);

private:
	const epicsUInt32 			id;
	const volatile epicsUInt8* 	pReg;
	
};

#endif //EVGDBUS_H