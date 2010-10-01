#include "evgSoftEvt.h"

#include <errlog.h> 
#include <stdexcept>

#include <mrfCommonIO.h> 
#include <mrfCommon.h> 

#include "evgRegMap.h"

evgSoftEvt::evgSoftEvt(volatile epicsUInt8* const pReg):
m_pReg(pReg),
m_lock() {
	BITSET8(m_pReg, SwEventControl, SW_EVT_ENABLE);
}

bool 
evgSoftEvt::pend() {
	return READ8(m_pReg, SwEventControl) & SW_EVT_PEND;
}
	
epicsStatus 
evgSoftEvt::setEvtCode(epicsUInt32 evtCode) {
	if(evtCode > 255)
		throw std::runtime_error("Event Code out of range.");
	
	SCOPED_LOCK(m_lock);
	while(pend() == 1);
	WRITE8(m_pReg, SwEventCode, evtCode);
	return OK; 
}
