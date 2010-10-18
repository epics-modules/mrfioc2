#include "evgMxc.h"

#include <iostream>
#include <stdexcept>
#include <math.h>

#include <mrfCommonIO.h> 
#include <errlog.h>   
#include <mrfCommon.h>

#include "evgMrm.h"
#include "evgRegMap.h"

evgMxc::evgMxc(const epicsUInt32 id, evgMrm* const owner):
m_id(id),
m_owner(owner),
m_pReg(owner->getRegAddr()) {	
}

evgMxc::~evgMxc() {
}

bool 
evgMxc::getMxcOutStatus() {
	return READ32(m_pReg, MuxControl(m_id)) & EVG_MUX_STATUS;
}

bool 
evgMxc::getMxcOutPolarity() {
	return READ32(m_pReg, MuxControl(m_id)) & EVG_MUX_POLARITY;
}

epicsStatus 
evgMxc::setMxcOutPolarity(bool polarity) {
	if(polarity)
		BITSET32(m_pReg, MuxControl(m_id), EVG_MUX_POLARITY);
	else
		BITCLR32(m_pReg, MuxControl(m_id), EVG_MUX_POLARITY);

	return OK;
}

epicsUInt32
evgMxc::getMxcPrescaler() {
	return READ32(m_pReg, MuxPrescaler(m_id));
}

epicsStatus 
evgMxc::setMxcPrescaler(epicsUInt32 preScaler) {
	if(preScaler == 0 || preScaler == 1)
		throw std::runtime_error("Invalid preScaler value in Multiplexed Counter");

	WRITE32(m_pReg, MuxPrescaler(m_id), preScaler);
	return OK;
}

epicsStatus 
evgMxc::setMxcFreq(epicsFloat64 freq) {
	epicsUInt32 clkSpeed = m_owner->getEvtClk()->getEvtClkSpeed() * pow(10, 6);
	epicsUInt32 preScaler = (epicsFloat64)clkSpeed / freq;
	
	setMxcPrescaler(preScaler);
	return OK;
}

epicsFloat64 
evgMxc::getMxcFreq() {
	epicsFloat64 clkSpeed = (epicsFloat64)m_owner->getEvtClk()->getEvtClkSpeed()
 																	* pow(10, 6);
	epicsFloat64 preScaler = (epicsFloat64)getMxcPrescaler();
	return clkSpeed/preScaler;	
}

epicsUInt8
evgMxc::getMxcTrigEvtMap() {
	return READ8(m_pReg, MuxTrigMap(m_id));
}

epicsStatus
evgMxc::setMxcTrigEvtMap(epicsUInt16 trigEvt, bool ena) {
	if(trigEvt > 7)
		throw std::runtime_error("EVG Mxc Trig Event ID too large.");

	epicsUInt8	mask = 1 << trigEvt;
	//Read-Modify-Write
	epicsUInt8 map = READ8(m_pReg, MuxTrigMap(m_id));

	if(ena)
		map = map | mask;
	else
		map = map & ~mask;

	WRITE8(m_pReg, MuxTrigMap(m_id), map);

	return OK;
}

