#include "evgSoftEvt.h"

#include <errlog.h> 

#include <mrfCommonIO.h> 
#include <mrfCommon.h> 

#include "evgRegMap.h"

evgSoftEvt::evgSoftEvt(volatile epicsUInt8* const pReg):
m_pReg(pReg) {
}

epicsStatus 
evgSoftEvt::enable(bool ena){	
	if(ena)
		BITSET8(m_pReg, SwEventControl, SW_EVT_ENABLE);
	else
		BITCLR8(m_pReg, SwEventControl, SW_EVT_ENABLE);
	
	return OK;
}


bool 
evgSoftEvt::enabled() {
	epicsUInt8 swEvtCtrl = READ8(m_pReg, SwEventControl);
	return swEvtCtrl & SW_EVT_ENABLE;
}


bool 
evgSoftEvt::pend() {
	epicsUInt8 swEvtCtrl = READ8(m_pReg, SwEventControl);
	return swEvtCtrl & SW_EVT_PEND;
}

	
epicsStatus 
evgSoftEvt::setEvtCode(epicsUInt32 evtCode) {
	if(evtCode < 0 || evtCode > 255) {
		errlogPrintf("ERROR: Event Code out of range.\n");
		return ERROR;		
	}

	//TODO: if(pend == 0), write in an atomic step to SwEvtCode register. 
	WRITE8(m_pReg, SwEventCode, evtCode);
	return OK; 
}
