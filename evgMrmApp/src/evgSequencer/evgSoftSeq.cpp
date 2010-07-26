#include "evgSoftSeq.h"

#include <math.h>

#include <errlog.h>

#include <mrfCommonIO.h>
#include <mrfCommon.h>
#include "evgRegMap.h"

#include "evgMrm.h"

evgSoftSeq::evgSoftSeq(const epicsUInt32 id, evgMrm* const owner):
m_id(id),
m_owner(owner),
m_pReg(owner->getRegAddr()),
m_trigSrc(0),
m_runMode(single),
m_seqRam(0),
m_seqRamMgr(owner->getSeqRamMgr()),
m_lock(new epicsMutex()) {
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
evgSoftSeq::getEventCode() {
	return m_eventCode;
}

epicsStatus
evgSoftSeq::setTimeStampSec(epicsFloat64* timeStamp, epicsUInt32 size) {
	epicsUInt32 timeStampInt[size];

	//Convert secs to clock ticks
	for(unsigned int i = 0; i < size; i++) {
		timeStampInt[i] = timeStamp[i] * (m_owner->getEvtClk()->getClkSpeed()) 
									   * pow(10,6);
	}

	setTimeStampTick(timeStampInt, size);
	return OK;
}

epicsStatus
evgSoftSeq::setTimeStampTick(epicsUInt32* timeStamp, epicsUInt32 size) {
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
evgSoftSeq::getTimeStamp() {
	return m_timeStamp;
}


epicsStatus 
evgSoftSeq::setTrigSrc(epicsUInt32 trigSrc) {
	m_lock->lock();
	m_trigSrc = trigSrc;
	m_lock->unlock();

	return OK;
}

epicsUInt32 
evgSoftSeq::getTrigSrc() {
	return m_trigSrc;
}


epicsStatus 
evgSoftSeq::setRunMode(SeqRunMode runMode) {
	m_lock->lock();
	m_runMode = runMode;
	m_lock->unlock();

	return OK;
}
	
SeqRunMode 
evgSoftSeq::getRunMode() {
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

/*
 * Check if any of the sequenceRam is available(i.e. unloaded).
 * if avaiable load and commit the sequence else return an error.
 */
epicsStatus
evgSoftSeq::load() {
	evgSeqRam* seqRam = 0;
	evgSeqRam* seqRamIter = 0;

	for(unsigned int i = 0; i < m_seqRamMgr->numOfRams(); i++) {
		seqRamIter = m_seqRamMgr->getSeqRam(i);
		if( seqRamIter->loaded() ) {
			if(this == seqRamIter->getSoftSeq()) {
				errlogPrintf("Seq %d already loaded.\n",getId());
				return OK;
			}
		} else {
			if( !seqRam )
				seqRam = seqRamIter;
		}
	}

	if(seqRam != 0) {
		printf("Loading Seq %d in SeqRam %d\n",getId(), seqRam->getId());
		setSeqRam(seqRam);
		seqRam->load(this);
		commit(0);
		
		return OK; 	
	} else {
		errlogPrintf("ERROR: Cannot load sequence.\n");
		return ERROR;	
	}
}
	
/* Unload does not wait for the sequecne to get over..Should it? */
epicsStatus
evgSoftSeq::unload(dbCommon* pRec) {
	if(!m_seqRam)
		return OK;
	printf("Unloading Seq %d from SeqRam %d\n", getId(), m_seqRam->getId());

	if( m_seqRam->enabled() ) {
		//Disable the unloadSeq record.
		pRec->pact = 1;		
		
		//Clear the SeqRam stop flag.
		WRITE32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM(m_seqRam->getId()));

		//Make the SeqRam single shot if in normal or automatic runMode.
		if(! (READ32(m_pReg, SeqControl(m_seqRam->getId())) & EVG_SEQ_RAM_SINGLE))
			BITSET32(m_pReg, SeqControl(m_seqRam->getId()), EVG_SEQ_RAM_SINGLE);

		//Adding the record to the 'list of records to be processed' on SEQ_RAM_STOP 
		//interrupt.
		if(m_seqRam->getId() == 0) {
			epicsMutexLock(m_owner->irqStop0.mutex);
			m_owner->irqStop0.recList.push_back(pRec);
			epicsMutexUnlock(m_owner->irqStop0.mutex);
    	} else if(m_seqRam->getId() == 1) {
			epicsMutexLock(m_owner->irqStop1.mutex);
 			m_owner->irqStop1.recList.push_back(pRec);
 			epicsMutexUnlock(m_owner->irqStop1.mutex);
		}

		//Enabling the SEQ_RAM_STOP interrupt
		BITSET32(m_pReg, IrqEnable, EVG_IRQ_STOP_RAM(m_seqRam->getId()));		

		printf("Waiting for SeqRam %d to be disabled. Pausing unload.\n",
				m_seqRam->getId());		
		
		return OK;		
	}
	
	//This part of the function is executed only when the sequenceRam is disabled.
	if(pRec->pact == 1) {
		//Enable the commitSeq record.
		pRec->pact = 0; 	
		printf("SeqRam disabled. Continuing unload.\n");
	}

	setSeqRam(0);
	m_seqRam->unload();

	return OK;
}
	
/*
 * If the sequenceRam is not enabled then copy the sequence to the 
 * sequenceRam but if the sequenceRam is enabled then add the record 
 * to the 'list of records to be processed' on seqRam stop interupt.
 */
epicsStatus
evgSoftSeq::commit(dbCommon* pRec) {
	if(!m_seqRam)
		return OK;

	printf("Commiting Seq %d to SeqRam %d\n", getId(), m_seqRam->getId());
	if( m_seqRam->enabled() ) {
		//Disable the commmitSeq record.
		pRec->pact = 1;		
		
		//Clear the SeqRam stop flag.
		WRITE32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM(m_seqRam->getId()));

		//Make the SeqRam single shot if in normal or automatic runMode.
		if(! (READ32(m_pReg, SeqControl(m_seqRam->getId())) & EVG_SEQ_RAM_SINGLE))
			BITSET32(m_pReg, SeqControl(m_seqRam->getId()), EVG_SEQ_RAM_SINGLE);

		//Adding the record to the 'list of records to be processed' on SEQ_RAM_STOP 
		//interrupt.
		if(m_seqRam->getId() == 0) {
			epicsMutexLock(m_owner->irqStop0.mutex);
			m_owner->irqStop0.recList.push_back(pRec);
			epicsMutexUnlock(m_owner->irqStop0.mutex);
    	} else if(m_seqRam->getId() == 1) {
			epicsMutexLock(m_owner->irqStop1.mutex);
 			m_owner->irqStop1.recList.push_back(pRec);
 			epicsMutexUnlock(m_owner->irqStop1.mutex);
		}
		
		//Enabling the SEQ_RAM_STOP interrupt
		BITSET32(m_pReg, IrqEnable, EVG_IRQ_STOP_RAM(m_seqRam->getId()));		

		printf("Waiting for SeqRam %d to be disabled. Pausing Commit.\n",
		 												m_seqRam->getId());		

		return OK;		
	}

	//This part of the function is executed only when the sequenceRam is disabled.	
	if(pRec->pact == 1) {
		//Enable the commitSeq record.
		pRec->pact = 0; 	
		printf("SeqRam disabled. Continuing commit.\n");
	}

	m_lock->lock();
	m_seqRam->setEventCode(getEventCode());
	m_seqRam->setTimeStamp(getTimeStamp());
	m_seqRam->setTrigSrc(getTrigSrc());
	m_seqRam->setRunMode(getRunMode());
	m_lock->unlock();

	enable();
	return OK;
}


epicsStatus
evgSoftSeq::enable() {
	if( (!m_seqRam) || m_seqRam->enabled() )
		return OK;
	else
		m_seqRam->enable();

	printf("Enabling seq %d in seqRam %d\n",getId(), m_seqRam->getId());
	return OK;
}

epicsStatus
evgSoftSeq::disable() {
	if( (!m_seqRam) || (!m_seqRam->enabled()) )
		return OK;
	else { 
		//If the seq is not in single runMode make it run in single mode.
		if(! (READ32(m_pReg, SeqControl(m_seqRam->getId())) & EVG_SEQ_RAM_SINGLE))
			BITSET32(m_pReg, SeqControl(m_seqRam->getId()), EVG_SEQ_RAM_SINGLE);
	}

	printf("Disabling seq %d in seqRam %d\n",getId(), m_seqRam->getId());
	return OK;
}

epicsStatus
evgSoftSeq::halt() {
	if( (!m_seqRam) || (!m_seqRam->enabled()) )
		return OK;
	else {
		m_seqRam->disable();

		//satisfy any call back request, on irqStop0 or irqStop1 recList.
		if(m_seqRam->getId() == 0)
			callbackRequest(&m_owner->irqStop0_cb);
		else 
			callbackRequest(&m_owner->irqStop1_cb);
	}

	return OK;
}

epicsMutex*
evgSoftSeq::getLock() {
	return m_lock;
}
