#include "evgSequence.h"

#include <math.h>

#include <errlog.h>

#include <mrfCommon.h>

#include "evgMrm.h"


evgSequence::evgSequence(const epicsUInt32 id, evgMrm* const owner):
m_id(id),
m_owner(owner),
m_trigSrc(0),
m_runMode(single),
m_seqRam(0),
m_lock(new epicsMutex()) {
}

const epicsUInt32
evgSequence::getId() const {
	return m_id;
}

epicsStatus 
evgSequence::setDescription(const char* desc) {
	m_desc.assign(desc);
	return OK; 
}

const char* 
evgSequence::getDescription() {
	return m_desc.c_str();
}

epicsStatus
evgSequence::setEventCode(epicsUInt8* eventCode, epicsUInt32 size) {
	if(size < 0 || size > 2048) {
		errlogPrintf("ERROR: EventCode array too large.");
		return ERROR;
	}
	
	m_lock->lock();
	m_eventCode.assign(eventCode, eventCode + size);
	m_eventCode.push_back(0x7f);
	m_lock->unlock();

	return OK;
}

std::vector<epicsUInt8>
evgSequence::getEventCode() {
	return m_eventCode;
}

epicsStatus
evgSequence::setTimeStampSec(epicsFloat64* timeStamp, epicsUInt32 size) {
	epicsUInt32 timeStampInt[size];

	//Convert secs to clock ticks
	for(unsigned int i = 0; i < size; i++) {
		timeStampInt[i] = timeStamp[i] * (m_owner->getEvtClk()->getClkSpeed()) * pow(10,6);
	}

	setTimeStampTick(timeStampInt, size);
	return OK;
}

epicsStatus
evgSequence::setTimeStampTick(epicsUInt32* timeStamp, epicsUInt32 size) {
	if(size < 0 || size > 2048) {
		errlogPrintf("ERROR: TimeStamp array too large.");
		return ERROR;
	}
	
	//Check if the timeStamps are sorted and Unique
// 	for(unsigned int i = 0; i < size; i++) {
// 		if(timeStamp[i] >= timeStamp[i+1]) {
// 			errlogPrintf("ERROR: Timestamp values are not sorted and unique\n");
// 			return ERROR;
// 		}
// 	}

	m_lock->lock();
	m_timeStamp.assign(timeStamp, timeStamp + size);
	m_timeStamp.push_back(m_timeStamp.back() + 1);
	m_lock->unlock();
	
	return OK;
}

std::vector<epicsUInt32>
evgSequence::getTimeStamp() {
	return m_timeStamp;
}


epicsStatus 
evgSequence::setTrigSrc(epicsUInt32 trigSrc) {
	m_lock->lock();
	m_trigSrc = trigSrc;
	m_lock->unlock();

	return OK;
}

epicsUInt32 
evgSequence::getTrigSrc() {
	return m_trigSrc;
}


epicsStatus 
evgSequence::setRunMode(SeqRunMode runMode) {
	m_lock->lock();
	m_runMode = runMode;
	m_lock->unlock();

	return OK;
}
	
SeqRunMode 
evgSequence::getRunMode() {
	return m_runMode;
}


epicsStatus 
evgSequence::setSeqRam(evgSeqRam* seqRam) {
	m_seqRam = seqRam;
	return OK;
}

evgSeqRam* 
evgSequence::getSeqRam() {
	return m_seqRam;
}

epicsMutex*
evgSequence::getLock() {
	return m_lock;
}


