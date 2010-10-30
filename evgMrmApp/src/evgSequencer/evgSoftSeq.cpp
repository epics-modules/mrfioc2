#include "evgSoftSeq.h"

#include <math.h>
#include <stdexcept>
#include <string>
#include <iostream>

#include <errlog.h>

#include <mrfCommonIO.h>
#include <mrfCommon.h>
#include "evgRegMap.h"

#include "evgMrm.h"

evgSoftSeq::evgSoftSeq(const epicsUInt32 id, evgMrm* const owner):
m_lock(),
m_id(id),
m_owner(owner),
m_pReg(owner->getRegAddr()),
m_trigSrc(Mxc0),
m_runMode(Normal),
m_trigSrcCt(Mxc0),
m_runModeCt(Normal),
m_seqRam(0),
m_seqRamMgr(owner->getSeqRamMgr()),
m_ecSize(0),
m_tsSize(0),
m_isEnabled(0),
m_isCommited(0) {
	scanIoInit(&ioscanpvt);
	scanIoInit(&ioScanPvtErr);
}

const epicsUInt32
evgSoftSeq::getId() const {
	return m_id;
}

epicsStatus 
evgSoftSeq::setDescription(const char* desc) {
	m_desc.assign(desc);
	return OK; 
}

const char* 
evgSoftSeq::getDescription() {
	return m_desc.c_str();
}

epicsStatus 
evgSoftSeq::setErr(std::string err) {
	m_err = err;
	scanIoRequest(ioScanPvtErr);
	return OK;
}

std::string
evgSoftSeq::getErr() {
	return m_err;	
}

epicsStatus
evgSoftSeq::setEventCode(epicsUInt8* eventCode, epicsUInt32 size) {	
	if(size > 2047)
		throw std::runtime_error("Too many EventCodes.");
	
	m_eventCode.clear();
	m_eventCode.assign(eventCode, eventCode + size);
	m_ecSize = m_eventCode.size();

	m_isCommited = false;
	scanIoRequest(ioscanpvt);
	return OK;
}

epicsStatus
evgSoftSeq::setTimeStampSec(epicsFloat64* timeStamp, epicsUInt32 size) {
	epicsUInt32 timeStampInt[size];

	//Convert secs to clock ticks
	for(unsigned int i = 0; i < size; i++) {
		timeStampInt[i] = (epicsUInt32)(floor(timeStamp[i] * 
				   m_owner->getEvtClk()->getEvtClkSpeed() * pow(10,6) + 0.5));
	}

	return setTimeStampTick(timeStampInt, size);
}

epicsStatus
evgSoftSeq::setTimeStampTick(epicsUInt32* timeStamp, epicsUInt32 size) {
	if(size > 2047)
		throw std::runtime_error("Too many TimeStamps.");

	m_timeStamp.clear();
	m_timeStamp.assign(timeStamp, timeStamp + size);
	m_tsSize = m_timeStamp.size();

	m_isCommited = false;
	scanIoRequest(ioscanpvt);
	return OK;
}

epicsStatus 
evgSoftSeq::setTrigSrc(SeqTrigSrc trigSrc) {
	if(trigSrc != m_trigSrc) {
		m_trigSrc = trigSrc;
		m_isCommited = false;
		scanIoRequest(ioscanpvt);
	}
	return OK;
}

epicsStatus 
evgSoftSeq::setRunMode(SeqRunMode runMode) {
	if(runMode != m_runMode) {
		m_runMode = runMode;
		m_isCommited = false;
		scanIoRequest(ioscanpvt);
	}
	return OK;
}

void
evgSoftSeq::ctEventCode() {
	m_eventCodeCt = m_eventCode;
}

void
evgSoftSeq::ctTimeStamp() {
	m_timeStampCt = m_timeStamp;
}

void
evgSoftSeq::ctTrigSrc() {
	m_trigSrcCt = m_trigSrc;
}

void
evgSoftSeq::ctRunMode() {
	m_runModeCt = m_runMode;
}

std::vector<epicsUInt8>
evgSoftSeq::getEventCodeCt() {
	return m_eventCodeCt;
}

std::vector<epicsUInt32>
evgSoftSeq::getTimeStampCt() {
	return m_timeStampCt;
}

SeqTrigSrc 
evgSoftSeq::getTrigSrcCt() {
	return m_trigSrcCt;
}

SeqRunMode 
evgSoftSeq::getRunModeCt() {
	return m_runModeCt;
}

epicsStatus 
evgSoftSeq::setSeqRam(evgSeqRam* seqRam) {
	m_seqRam = seqRam;
	return OK;
}

evgSeqRam* 
evgSoftSeq::getSeqRam() {
	return m_seqRam;
}

bool
evgSoftSeq::isLoaded() {
	if(m_seqRam)
		return true;
	else
		return false;
}

bool
evgSoftSeq::isEnabled() {
	return m_isEnabled;
}

bool
evgSoftSeq::isCommited() {
	return m_isCommited;
}

epicsStatus
evgSoftSeq::load() {
	evgSeqRam* seqRam = 0;
	evgSeqRam* seqRamIter = 0;

	/*Initialize the seqRam if hardware is available(un-allocated)*/
	for(unsigned int i = 0; i < m_seqRamMgr->numOfRams(); i++) {
		seqRamIter = m_seqRamMgr->getSeqRam(i);
		if( seqRamIter->isAllocated() ) {
			if(this == seqRamIter->getSoftSeq()) {
				char err[80];
				sprintf(err, "SoftSeq %d is already loaded", getId());
				std::string strErr(err);
				throw std::runtime_error(strErr);
			}
		} else {
			if( !seqRam )
				seqRam = seqRamIter;
		}
	}

	/*If a seqRam is avialable*/
	if(seqRam != 0) {
		setSeqRam(seqRam);
		seqRam->alloc(this);
		scanIoRequest(ioscanpvt);
		sync(); 
		
		return OK;	
	} else {
		char err[80];
		sprintf(err, "No SeqRam to load SoftSeq %d", getId());
		std::string strErr(err);
		throw std::runtime_error(strErr);
	}
}
	
epicsStatus
evgSoftSeq::unload() {
	if(m_seqRam) {
		m_seqRam->setRunMode(Single);
		m_seqRam->dealloc();
		setSeqRam(0);
		scanIoRequest(ioscanpvt);
	}

	return OK;
}


epicsStatus
evgSoftSeq::commit() {
	inspectSoftSeq();	

	ctEventCode();
	ctTimeStamp();
	ctTrigSrc();
	ctRunMode();
	if(m_seqRam != 0)
		sync();

	m_isCommited = true;
	scanIoRequest(ioscanpvt);
	return OK;
}

epicsStatus
evgSoftSeq::enable() {
	if(m_seqRam) {
		m_seqRam->setTrigSrc(getTrigSrcCt());
		m_seqRam->setRunMode(getRunModeCt());
		m_seqRam->enable();
	}

	m_isEnabled = true;
	scanIoRequest(ioscanpvt);
	return OK;
}

epicsStatus
evgSoftSeq::disable() {
	if(m_seqRam)
		m_seqRam->setRunMode(Single);

	m_isEnabled = false;
	scanIoRequest(ioscanpvt);
	return OK;
}

epicsStatus
evgSoftSeq::halt(bool callBack) {
	if( (!m_seqRam) || (!m_seqRam->isEnabled()) )
		return OK;
	else {
		m_seqRam->disable();
		m_isEnabled = false;
		scanIoRequest(ioscanpvt);

		if(callBack) {
			/*Satisfy any callback request pending on irqStop0 or irqStop1 recList.
			As no 'End of sequence' Intrrupt will be generated. */
			if(m_seqRam->getId() == 0)
				callbackRequest(&m_owner->irqStop0_cb);
			else 
				callbackRequest(&m_owner->irqStop1_cb);
		}
	}

	return OK;
}

epicsStatus
evgSoftSeq::sync() {
	if(!m_seqRam) {
		char err[80];
		sprintf(err, "SeqRam is not allocated for SoftSeq %d", getId());
		std::string strErr(err);
		throw std::runtime_error(strErr);
	}

	if(isRunning())
		return OK;

	if(getEventCodeCt().size() == 0) {
		m_eventCodeCt.push_back(0x7f);
		m_timeStampCt.push_back(1);
		m_trigSrcCt = Mxc0;
		m_runModeCt = Normal;
	}

	m_seqRam->setEventCode(getEventCodeCt());
	m_seqRam->setTimeStamp(getTimeStampCt());
	m_seqRam->setTrigSrc(getTrigSrcCt());
	m_seqRam->setRunMode(getRunModeCt());
	if(m_isEnabled) m_seqRam->enable();

	scanIoRequest(ioscanpvt);

	return OK;
}

epicsStatus
evgSoftSeq::isRunning() {
	m_seqRam->setRunMode(Single);
	m_seqRam->setTrigSrc(SW_Ram0);
		
	WRITE32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM(m_seqRam->getId()));

	if(!m_seqRam->isRunning()) {
		m_seqRam->disable();
		return 0;
	} else {	
		BITSET32(m_pReg, IrqEnable, EVG_IRQ_STOP_RAM(m_seqRam->getId()));		
		return 1;
	}		
}

epicsStatus
evgSoftSeq::inspectSoftSeq() {
	/*Check if the timeStamps are sorted and Unique */
	if(m_timeStamp.size() > 1) {
		for(unsigned int i = 1; i < m_timeStamp.size(); i++) {
			if( m_timeStamp[i-1] >= m_timeStamp[i] ) {
				if( (m_timeStamp[i] == 0) && (m_eventCode[i] == 255))
					continue;
				 else
					throw std::runtime_error("Timestamp values are not Sorted/Unique");
			} 
		}
	}

	/*Check on the sizes of timeStamp and eventCode vectors.
 	  Appending 'End of Sequence' EventCode and TimeStamp. */
	if(m_tsSize == m_ecSize) {

		if(m_tsSize == 0) {
			if(m_tsSize == m_timeStamp.size())
				m_timeStamp.push_back(1);
			else 
				m_timeStamp[m_tsSize] = 1;
		 } else {
			if(m_tsSize == m_timeStamp.size())
				m_timeStamp.push_back(m_timeStamp.back() + 1);
			else 
				m_timeStamp[m_tsSize] = m_timeStamp[m_tsSize - 1] + 1;
		}

		if(m_ecSize == m_eventCode.size())
			m_eventCode.push_back(0x7f);
		else
			m_eventCode[m_ecSize] = 0x7f;

	} else if(m_tsSize < m_ecSize) {

		if(m_tsSize == 0) {
			if(m_tsSize == m_timeStamp.size())
				m_timeStamp.push_back(1);
			else 
				m_timeStamp[m_tsSize] = 1;
		} else {
			if(m_tsSize == m_timeStamp.size())
				m_timeStamp.push_back(m_timeStamp.back() + 1);
			else 
				m_timeStamp[m_tsSize] = m_timeStamp.back() + 1;
		}

		m_eventCode[m_tsSize] = 0x7f;

	} else if(m_tsSize > m_ecSize) {

		if(m_ecSize == m_eventCode.size())
			m_eventCode.push_back(0x7f);
		else
			m_eventCode[m_ecSize] = 0x7f;

		m_timeStamp[m_ecSize] = m_timeStamp[m_ecSize - 1] + 1;
	}

	return OK;
}
