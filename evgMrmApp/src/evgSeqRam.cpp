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
	std::vector<epicsUInt8>::iterator it = eventCode.begin();
	for(int i = 0; it < eventCode.end(); it++, i++) {
		WRITE8(m_pReg, SeqRamEvent(m_id,i), *it);
	}

	return OK;
}


epicsStatus
evgSeqRam::setTimeStamp(std::vector<epicsUInt32> timeStamp){
	std::vector<epicsUInt32>::iterator it = timeStamp.begin();
	for(int i = 0; it < timeStamp.end(); it++, i++) {
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
evgSeqRam::setTrigSrc(TrigSrc trigSrc) {
	WRITE8(m_pReg, SeqTrigSrc(m_id), trigSrc);
	return OK;
}


epicsStatus
evgSeqRam::setRunMode(RunMode mode) {
	switch (mode) {
		case(single):
			BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SINGLE);
			break;
		case(recycle):
			BITCLR32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SINGLE);
			BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_RECYCLE);
			break;
		case(recycleOnTrigger):
			BITCLR32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SINGLE);
			BITCLR32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_RECYCLE);
			break;		
		default:
			break;	
	}
	
	return OK;
}

epicsStatus 
evgSeqRam::enable() {
	BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_ARM);
	return OK;
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

/*If the sequence is not loaded also means it is disabled*/
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