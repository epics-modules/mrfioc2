#ifndef EVG_MXC_H
#define EVG_MXC_H

#include <epicsTypes.h>   

class evgMrm;

class evgMxc {
public:
	evgMxc(const epicsUInt32, evgMrm* const);
	~evgMxc();

	bool getMxcOutStatus();

	bool getMxcOutPolarity();
	epicsStatus setMxcOutPolarity(bool);

	epicsUInt32 getMxcPrescaler();
	epicsStatus setMxcPrescaler(epicsUInt32);
	epicsFloat64 getMxcFreq();
	epicsStatus setMxcFreq(epicsFloat64);

	epicsUInt8 getMxcTrigEvtMap();
	epicsStatus setMxcTrigEvtMap(epicsUInt16);

private:
	const epicsUInt32 			m_id;
	evgMrm* const				m_owner;
	volatile epicsUInt8* const	m_pReg;

};

#endif//EVG_MXC_H
