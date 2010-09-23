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
	epicsUInt32 muxCtrl = READ32(m_pReg, MuxControl(m_id));
	return muxCtrl & EVG_MUX_STATUS;
}

bool 
evgMxc::getMxcOutPolarity() {
	epicsUInt32 muxCtrl = READ32(m_pReg, MuxControl(m_id));
	return muxCtrl & EVG_MUX_POLARITY;
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
evgMxc::setMxcTrigEvtMap(epicsUInt16 map) {
	if(map > 255)
		throw std::runtime_error("EVG Mxc Trig Event Map value too large.");

	WRITE8(m_pReg, MuxTrigMap(m_id), map);
	return OK;
}
