#include "evgTrigEvt.h"

#include <iostream>

#include <errlog.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h>

#include "evgRegMap.h"

evgTrigEvt::evgTrigEvt(const epicsUInt32 id, const volatile epicsUInt8* pReg):
id(id),
pReg(pReg) {
}

bool 
evgTrigEvt::enabled() {
	epicsUInt32 trigEvtCtrl = READ32(pReg, TrigEventCtrl(id));
	return trigEvtCtrl &  EVG_TRIG_EVT_ENA;
} 

epicsStatus
evgTrigEvt::enable(bool ena) {
	if(ena)
		BITSET32(pReg, TrigEventCtrl(id), EVG_TRIG_EVT_ENA);
	else 
		BITCLR32(pReg, TrigEventCtrl(id), EVG_TRIG_EVT_ENA);

	return OK;
}

epicsUInt8
evgTrigEvt::getEvtCode() {
	return READ8(pReg, TrigEventCode(id));
}

epicsStatus
evgTrigEvt::setEvtCode(epicsUInt8 evtCode) {
	WRITE8(pReg, TrigEventCode(id), evtCode);
	return OK;
}