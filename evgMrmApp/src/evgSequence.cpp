#include "evgSequence.h"

#include <errlog.h>

#include <mrfCommon.h>


evgSequence::evgSequence(const epicsUInt32 id):
m_id(id),
m_trigSrc(mxc0),
m_runMode(single) {
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
		
	m_eventCode.assign(eventCode, eventCode + size);
	return OK;
}

epicsUInt8*
evgSequence::getEventCodeA() {
	epicsUInt8* eventCode = 0;
	std::copy(m_eventCode.begin(), m_eventCode.end(), eventCode);
	return eventCode;
}

std::vector<epicsUInt8>
evgSequence::getEventCodeV() {
	return m_eventCode;
}


epicsStatus
evgSequence::setTimeStamp(epicsUInt32* timeStamp, epicsUInt32 size) {
	if(size < 0 || size > 2048) {
		errlogPrintf("ERROR: Number of event is too large.");
		return ERROR;
	}
		
	m_timeStamp.assign(timeStamp, timeStamp + size);
	return OK;
}

epicsUInt32*
evgSequence::getTimeStampA() {
	epicsUInt32* timeStamp = 0;
	std::copy(m_timeStamp.begin(), m_timeStamp.end(), timeStamp);
	return timeStamp;
}

std::vector<epicsUInt32>
evgSequence::getTimeStampV() {
	return m_timeStamp;
}


epicsStatus 
evgSequence::setTrigSrc(TrigSrc trigSrc) {
	m_trigSrc = trigSrc;
	return OK;
}

TrigSrc 
evgSequence::getTrigSrc() {
	return m_trigSrc;
}


epicsStatus 
evgSequence::setRunMode(RunMode runMode) {
	m_runMode = runMode;
	return OK;
}
	
RunMode 
evgSequence::getRunMode() {
	return m_runMode;
}