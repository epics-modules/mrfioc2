#ifndef EVGTRIGEVT_H
#define EVGTRIGEVT_H

#include  <epicsTypes.h>   

class evgTrigEvt{
public:
	evgTrigEvt(const epicsUInt32, const volatile epicsUInt8*);
	~evgTrigEvt();

	bool enabled();
	epicsStatus enable(bool);

	epicsStatus setEvtCode(epicsUInt8);
	epicsUInt8 getEvtCode();

private:
	const epicsUInt32 			id;
	const volatile epicsUInt8* pReg;
};

#endif //EVGTRIGEVT_H
