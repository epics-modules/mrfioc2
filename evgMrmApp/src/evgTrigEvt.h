#ifndef EVGTRIGEVT_H
#define EVGTRIGEVT_H

#include  <epicsTypes.h>   

class evgTrigEvt{
public:
	evgTrigEvt(const epicsUInt32, volatile epicsUInt8* const);
	~evgTrigEvt();

	bool enabled();
	epicsStatus enable(bool);

	epicsStatus setEvtCode(epicsUInt8);
	epicsUInt8 getEvtCode();

private:
	const epicsUInt32 			m_id;
	volatile epicsUInt8* const  m_pReg;
};

#endif //EVGTRIGEVT_H
