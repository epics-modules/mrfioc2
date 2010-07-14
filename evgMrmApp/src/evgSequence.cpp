#include "evgSequence.h"

#include <errlog.h>

#include <mrfCommon.h>


evgSequence::evgSequence(const epicsUInt32 id):
m_id(id),
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
		errlogPrintf("ERROR: Number of events is too large.");
		return ERROR;
	}
	
	m_lock->lock();
	m_eventCode.assign(eventCode, eventCode + size);
	m_lock->unlock();

	return OK;
}

std::vector<epicsUInt8>
evgSequence::getEventCode() {
	return m_eventCode;
}


epicsStatus
evgSequence::setTimeStamp(epicsUInt32* timeStamp, epicsUInt32 size) {
	if(size < 0 || size > 2048) {
		errlogPrintf("ERROR: Number of event is too large.");
		return ERROR;
	}
	
	m_lock->lock();
	m_timeStamp.assign(timeStamp, timeStamp + size);
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


