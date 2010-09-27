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
m_runMode(Single),
m_seqRam(0),
m_seqRamMgr(owner->getSeqRamMgr()),
m_ecSize(0),
m_tsSize(0),
m_isEnabled(0) {
	scanIoInit(&ioscanpvt);
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
evgSoftSeq::setEventCode(epicsUInt8* eventCode, epicsUInt32 size) {	
	if(size > 2047)
		throw std::runtime_error("Too many EventCodes.");
	
	m_eventCode.clear();
	m_eventCode.assign(eventCode, eventCode + size);
	m_ecSize = m_eventCode.size();

	return OK;
}

epicsStatus
evgSoftSeq::setTimeStampSec(epicsFloat64* timeStamp, epicsUInt32 size) {
	epicsUInt32 timeStampInt[size];

	//Convert secs to clock ticks
	for(unsigned int i = 0; i < size; i++) {
		timeStampInt[i] = floor(timeStamp[i] * 
						m_owner->getEvtClk()->getEvtClkSpeed() * pow(10,6) + 0.5);
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

	return OK;
}

epicsStatus 
evgSoftSeq::setTrigSrc(SeqTrigSrc trigSrc) {
	m_trigSrc = trigSrc;
	return OK;
}

epicsStatus 
evgSoftSeq::setRunMode(SeqRunMode runMode) {
	m_runMode = runMode;
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
	return m_trigSrc;
}

SeqRunMode 
evgSoftSeq::getRunModeCt() {
	return m_runMode;
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

epicsStatus
evgSoftSeq::load() {
	evgSeqRam* seqRam = 0;
	evgSeqRam* seqRamIter = 0;

	for(unsigned int i = 0; i < m_seqRamMgr->numOfRams(); i++) {
		seqRamIter = m_seqRamMgr->getSeqRam(i);
		if( seqRamIter->IsAlloc() ) {
			if(this == seqRamIter->getSoftSeq()) {
				char err[80];
				sprintf(err, "SeqRam %d is already allocated to SoftSeq %d"
								, seqRamIter->getId(), getId());
				std::string strErr(err);
				throw std::runtime_error(strErr);
			}
		} else {
			if( !seqRam )
				seqRam = seqRamIter;
		}
	}

	if(seqRam != 0) {
		printf("Allocating SeqRam %d to SoftSeq %d\n", seqRam->getId(), getId());
		setSeqRam(seqRam);
		seqRam->alloc(this);
		scanIoRequest(ioscanpvt);
		inspectSoftSeq();
		sync(0);
		
		return OK;	
	} else {
		char err[80];
		sprintf(err, "Cannot allocate SeqRam to SoftSeq %d", getId());
		std::string strErr(err);
		throw std::runtime_error(strErr);
	}
}
	
epicsStatus
evgSoftSeq::unload(dbCommon* pRec) {
	if(!m_seqRam) 
		return OK;

	printf("Unloading SeqRam %d associated with SoftSeq %d\n", 
													m_seqRam->getId(), getId());

	struct evgMrm::irqPvt* irqpvt = 0;
	if(m_seqRam->getId() == 0)
		irqpvt = &m_owner->irqStop0;
	else if(m_seqRam->getId() == 1)
		irqpvt = &m_owner->irqStop1;

	m_seqRam->setRunMode(Single);
	m_seqRam->setTrigSrc(SW_Ram0);
	scanIoRequest(ioscanpvt);

	WRITE32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM(m_seqRam->getId()));	

	if(!m_seqRam->running())
		m_seqRam->disable();
	else {				
		pRec->pact = 1;	

		SCOPED_LOCK2(irqpvt->mutex, guard);
		irqpvt->recList.push_back(pRec);
			
		BITSET32(m_pReg, IrqEnable, EVG_IRQ_STOP_RAM(m_seqRam->getId()));		

		printf("Paused the unloading of SeqRam %d. Waiting for it to be disabled.\n"
					, m_seqRam->getId());
				
		return OK;
	}		
	
	if(pRec->pact == 1) {
		pRec->pact = 0; 	
		printf("Continuing unload of SeqRam %d.\n", m_seqRam->getId());
	}

	m_seqRam->dealloc();
	setSeqRam(0);
	scanIoRequest(ioscanpvt);

	return OK;
}
	
epicsStatus
evgSoftSeq::sync(dbCommon* pRec) {
	if(!m_seqRam) {
		char err[80];
		sprintf(err, "SeqRam is not allocated for SoftSeq %d", getId());
		std::string strErr(err);
		throw std::runtime_error(strErr);
	}

	printf("Syncing SoftSeq %d to SeqRam %d\n", getId(), m_seqRam->getId());

	struct evgMrm::irqPvt* irqpvt = 0;
	if(m_seqRam->getId() == 0)
		irqpvt = &m_owner->irqStop0;
	else if(m_seqRam->getId() == 1)
		irqpvt = &m_owner->irqStop1;

	m_seqRam->setRunMode(Single);
	m_seqRam->setTrigSrc(SW_Ram0);
	scanIoRequest(ioscanpvt);
		
	WRITE32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM(m_seqRam->getId()));

	if(!m_seqRam->running())
		m_seqRam->disable();
	else {	
		if(pRec == 0)
			std::runtime_error("Trying to sync an running sequence.");

		pRec->pact = 1;		

		SCOPED_LOCK2(irqpvt->mutex, guard);
		irqpvt->recList.push_back(pRec);

		BITSET32(m_pReg, IrqEnable, EVG_IRQ_STOP_RAM(m_seqRam->getId()));		

		printf("Paused sync of SoftSeq %d. Waiting for SeqRam %d to be "
				"disabled\n", getId(), m_seqRam->getId());		

		return OK;
	}		

	if(pRec->pact == 1) {
		pRec->pact = 0; 	
		printf("Continue syncing SoftSeq %d\n", getId());
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
evgSoftSeq::commit(dbCommon* pRec) {
	inspectSoftSeq();	

	ctEventCode();
	ctTimeStamp();
	ctTrigSrc();
	ctRunMode();
	
	if(m_seqRam != 0)
		sync(pRec);

	return OK;
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
			m_timeStamp.push_back(1);
			m_eventCode.push_back(0x7f);
		 } else {
			if(m_tsSize == m_timeStamp.size())
				m_timeStamp.push_back(m_timeStamp.back() + 1);
			else 
				m_timeStamp[m_tsSize] = m_timeStamp[m_tsSize - 1] + 1;

			if(m_ecSize == m_eventCode.size())
				m_eventCode.push_back(0x7f);
			else
				m_eventCode[m_ecSize] = 0x7f;
		}

	} else if(m_tsSize < m_ecSize) {

		if(m_tsSize == 0) {
			m_timeStamp.push_back(1);
			m_eventCode[m_tsSize] = 0x7f;
		} else {
			if(m_tsSize == m_timeStamp.size())
				m_timeStamp.push_back(m_timeStamp.back() + 1);
			else 
				m_timeStamp[m_tsSize] = m_timeStamp.back() + 1;

		    m_eventCode[m_tsSize] = 0x7f;
		}
	} else if(m_tsSize > m_ecSize) {

		if(m_ecSize == m_eventCode.size())
			m_eventCode.push_back(0x7f);
		else
			m_eventCode[m_ecSize] = 0x7f;

	}

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
	if(m_seqRam){
		m_seqRam->setRunMode(Single);
		scanIoRequest(ioscanpvt);
	}

	m_isEnabled = false;
	scanIoRequest(ioscanpvt);
	printf("Disabling SoftSeq %d in seqRam %d\n",getId(), m_seqRam->getId());
	return OK;
}

epicsStatus
evgSoftSeq::halt(bool callBack) {
	if( (!m_seqRam) || (!m_seqRam->enabled()) )
		return OK;
	else {
		m_seqRam->disable();
		
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


