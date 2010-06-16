#include "evgSeqRam.h"

#include <iostream>

#include <errlog.h>

#include <mrfCommonIO.h>
#include <mrfCommon.h>

#include "evgRegMap.h"

evgSeqRam::evgSeqRam(const epicsUInt32 id, volatile epicsUInt8* const pReg):
m_id(id),
m_pReg(pReg),
m_allocated(0),
m_seq(0) {
}

const epicsUInt32 
evgSeqRam::getId() {
	return m_id;
}

epicsStatus
evgSeqRam::setEventCode(std::vector<epicsUInt8> eventCode) {
	for(unsigned int i = 0; i < eventCode.size(); i++) {
		WRITE8(m_pReg, SeqRamEvent(m_id,i), eventCode[i]);
	}

	return OK;
}


epicsStatus
evgSeqRam::setTimeStamp(std::vector<epicsUInt32> timeStamp){
	for(unsigned int i = 0; i < timeStamp.size(); i++) {
		WRITE32(m_pReg, SeqRamTS(m_id,i), timeStamp[i]);
	}
	
	return OK;
}


epicsStatus
evgSeqRam::setSoftTrig(bool enable) {
	if(enable)
		BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SW_TRIG);
	else
		BITCLR32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SW_TRIG);

	return OK;
}

epicsStatus
evgSeqRam::setTrigSrc(SeqTrigSrc trigSrc) {
	WRITE8(m_pReg, SeqTrigSrc(m_id - 1), trigSrc);
	return OK;
}


epicsStatus
evgSeqRam::setRunMode(SeqRunMode mode) {
	switch (mode) {
		case(Single):
			BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SINGLE);
			break;
		case(Recycle):
			BITCLR32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SINGLE);
			BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_RECYCLE);
			break;
		case(RecycleOnTrig):
			BITCLR32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SINGLE);
			BITCLR32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_RECYCLE);
			break;		
		default:
			break;	
	}
	
	return OK;
}

/*Sequence Ram has to be loaded to enable it*/
epicsStatus 
evgSeqRam::enable() {
	if(loaded()) {
		BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_ARM);
		return OK;
	} else 
		return ERROR;
}


epicsStatus 
evgSeqRam::disable() {
	BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_DISABLE);
	return OK;
}


epicsStatus 
evgSeqRam::reset() {
	BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_RESET);	
	BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_DISABLE);
	return OK;
}


bool 
evgSeqRam::enabled() const {
	epicsUInt32 seqCtrl =  READ32(m_pReg, SeqControl(m_id)); 
	return seqCtrl & EVG_SEQ_RAM_ENABLED;
}

bool 
evgSeqRam::running() const {
	epicsUInt32 seqCtrl =  READ32(m_pReg, SeqControl(m_id)); 
	return seqCtrl & EVG_SEQ_RAM_RUNNING;
}

epicsStatus
evgSeqRam::load(evgSequence* seq) {
	m_seq = seq;
	m_allocated = true;
	return OK;
}

epicsStatus
evgSeqRam::unload() {
	reset();
	m_seq = 0;
	m_allocated = false;
	return OK;
}

bool
evgSeqRam::loaded() const {
	bool ret;
	
	if(m_allocated) {
		if(m_seq)
			ret = true;
		else
			ret = false;
	} else {
		ret = false;
	}

	return ret;
}

evgSequence* 
evgSeqRam::getSequence() {
	return m_seq;
}