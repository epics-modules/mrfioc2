#include "evgTrigEvt.h"

#include <iostream>
#include <stdexcept>

#include <errlog.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h>

#include "evgRegMap.h"

evgTrigEvt::evgTrigEvt(const epicsUInt32 id, volatile epicsUInt8* const pReg):
m_id(id),
m_pReg(pReg) {
}

evgTrigEvt::~evgTrigEvt() {
}

bool 
evgTrigEvt::enabled() {
	epicsUInt32 trigEvtCtrl = READ32(m_pReg, TrigEventCtrl(m_id));
	return trigEvtCtrl &  EVG_TRIG_EVT_ENA;
} 

epicsStatus
evgTrigEvt::enable(bool ena) {
	if(ena)
		BITSET32(m_pReg, TrigEventCtrl(m_id), EVG_TRIG_EVT_ENA);
	else 
		BITCLR32(m_pReg, TrigEventCtrl(m_id), EVG_TRIG_EVT_ENA);

	return OK;
}

epicsUInt8
evgTrigEvt::getEvtCode() {
	return READ8(m_pReg, TrigEventCode(m_id));
}

epicsStatus
evgTrigEvt::setEvtCode(epicsUInt32 evtCode) {
	if(evtCode > 255)
		throw std::runtime_error("Event Code out of range.");

	WRITE8(m_pReg, TrigEventCode(m_id), evtCode);
	return OK;
}
