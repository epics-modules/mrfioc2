#include "evgMxc.h"

#include <iostream>
#include <stdexcept>

#include <mrfCommonIO.h> 
#include <errlog.h>   
#include <mrfCommon.h>  //ERROR / OK 

#include "evgRegMap.h"

evgMxc::evgMxc(const epicsUInt32 id, volatile epicsUInt8* const pReg):
m_id(id),
m_pReg(pReg) {	
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
	if(preScaler == 0 || preScaler == 1) {
		printf("ERROR: Invalid preScaler value in Multiplexed Counter : %d\n", preScaler);
		return ERROR;
	}
	WRITE32(m_pReg, MuxPrescaler(m_id), preScaler);
	return OK;
}

epicsUInt8
evgMxc::getMxcTrigEvtMap() {
	return READ8(m_pReg, MuxTrigMap(m_id));
}

epicsStatus
evgMxc::setMxcTrigEvtMap(epicsUInt8 map) {
	WRITE8(m_pReg, MuxTrigMap(m_id), map);
	return OK;
}
