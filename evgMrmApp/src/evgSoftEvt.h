#ifndef EVG_SOFTEVT_H
#define EVG_SOFTEVT_H

#include <epicsTypes.h>
#include <epicsMutex.h>

class evgSoftEvt {

public:
	evgSoftEvt(volatile epicsUInt8* const);

	epicsStatus enable(bool);
 	bool enabled();

	bool pend();
	
	epicsStatus setEvtCode(epicsUInt32);

private:
	volatile epicsUInt8* const	m_pReg;
	epicsMutex 					m_lock;

};

#endif //EVG_SOFTEVT_H
