#include "evgSequence.h"

#include <errlog.h>

#include <mrfCommon.h>


evgSequence::evgSequence(const epicsUInt32 id):
m_id(id),
m_eventCodeA(0),
m_timeStampA(0),
m_trigSrc(mxc0),
m_runMode(single) {
	//For Testing purpose
 	epicsUInt8 eventCode[] = {1, 2, 3, 5, 127};
 	epicsUInt32 timeStamp[] = {125000000, 250000000, 375000000, 500000000, 625000000};

 	setEventCode(eventCode, 5);
 	setTimeStamp(timeStamp, 5);		
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
		
	std::copy(eventCode, eventCode + size, m_eventCodeA);
	m_eventCodeV.assign(eventCode, eventCode + size);
	
	return OK;
}

epicsUInt8*
evgSequence::getEventCodeA() {
	return m_eventCodeA;
}

std::vector<epicsUInt8>
evgSequence::getEventCodeV() {
	return m_eventCodeV;
}


epicsStatus
evgSequence::setTimeStamp(epicsUInt32* timeStamp, epicsUInt32 size) {
	if(size < 0 || size > 2048) {
		errlogPrintf("ERROR: Number of event is too large.");
		return ERROR;
	}
	
	std::copy(timeStamp, timeStamp + size, m_timeStampA);	
	m_timeStampV.assign(timeStamp, timeStamp + size);

	return OK;
}

epicsUInt32*
evgSequence::getTimeStampA() {
	return m_timeStampA;
}

std::vector<epicsUInt32>
evgSequence::getTimeStampV() {
	return m_timeStampV;
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