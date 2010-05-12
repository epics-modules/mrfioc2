#ifndef EVGMXC_H
#define EVGMXC_H

#include <epicsTypes.h>   

class evgMxc {
public:
	evgMxc(const epicsUInt32, const volatile epicsUInt8*);
	~evgMxc();

	bool getMxcOutStatus();

	bool getMxcOutPolarity();
	epicsStatus setMxcOutPolarity(bool);

	epicsUInt32 getMxcPrescaler();
	epicsStatus setMxcPrescaler(epicsUInt32);
	
	epicsUInt8 getMxcTrigEvtMap();
	epicsStatus setMxcTrigEvtMap(epicsUInt8);

private:
	const epicsUInt32 			id;
	const volatile epicsUInt8*	pReg;
};

#endif//EVGMXC_H