#include "evgSeqRamManager.h"

#include <iostream>

#include <errlog.h>
#include <longoutRecord.h>

#include <mrfCommonIO.h>
#include <mrfCommon.h>

#include "evgRegMap.h"
#include "evgMrm.h"

evgSeqRamMgr::evgSeqRamMgr(volatile epicsUInt8* const pReg, evgMrm* owner):
m_owner(owner),
m_pReg(pReg) {
	
	//For testing
	for(int i = 0; i < 3; i++) {
		m_sequence.push_back(new evgSequence(i));
	}

	for(int i = 0; i < evgNumSeqRam; i++) {
		m_seqRam.push_back(new evgSeqRam(i, pReg));
	}

}

evgSeqRam*
evgSeqRamMgr::findSeqRam(evgSequence* seq) {
	for(unsigned int i = 0; i < m_seqRam.size(); i++) {
		if(seq == m_seqRam[i]->getSequence())
			return m_seqRam[i];	
	}

	return 0;
}

epicsUInt32
evgSeqRamMgr::findSeqRamId(evgSequence* seq) {
	evgSeqRam* seqRam = findSeqRam(seq);
	if(seqRam)
		return seqRam->getId();

	return -1;
}

evgSeqRam* 
evgSeqRamMgr::getSeqRam(epicsUInt32 seqRamId) {
	if(	seqRamId < m_seqRam.size() )
		return m_seqRam[seqRamId];
	else 
		return 0;
}

evgSequence* 
evgSeqRamMgr::getSequence(epicsUInt32 seqId) {
	if(seqId < m_sequence.size() )
		return m_sequence[seqId];
	else 
		return 0;
}

/*
 * Check if any of the sequenceRam is available(i.e. unloaded).
 * if avaiable load and commit the sequence else return an error.
 */
epicsStatus
evgSeqRamMgr::load(evgSequence* seq) {
	evgSeqRam* seqRam = 0;

	for(unsigned int i = 0; i < m_seqRam.size(); i++) {
		if(m_seqRam[i]->loaded()) {
			if(seq == m_seqRam[i]->getSequence()) {
				printf("Seq %d already loaded.\n",seq->getId());
				return OK;
			}
		} else {
			if( !seqRam )
				seqRam = m_seqRam[i];
		}
	}

	if(seqRam != 0) {
		printf("Loading Seq %d in SeqRam %d\n",seq->getId(), seqRam->getId());
		seqRam->load(seq);
		commit(seq, 0);
		//enable(seq);
		
		return OK; 	
	} else {
		errlogPrintf("ERROR: Cannot load sequence.\n");
		return ERROR;	
	}
}

epicsStatus
evgSeqRamMgr::unload(evgSequence* seq) {
	evgSeqRam* seqRam = findSeqRam(seq);
	if(seqRam) {
		printf("Unloading seq %d in SeqRam %d\n",seq->getId(), seqRam->getId());
		seqRam->unload();
	}

	return OK;
}

/*
 * If the sequenceRam is not enabled then copy the sequence to the 
 * sequenceRam but if the sequenceRam is enabled then add the record 
 * to the 'list of records to be processed' on seqRam stop interupt.
 */
epicsStatus
evgSeqRamMgr::commit(evgSequence* seq, dbCommon* pRec) {
	evgSeqRam* seqRam = findSeqRam(seq);
	if(!seqRam)
		return OK;

	printf("Commiting Seq %d to SeqRam %d\n", seq->getId(), seqRam->getId());

	if( seqRam->enabled() ) {
		//Disable the commmitSeq record.
		pRec->pact = 1;		
		
		//Make the SeqRam single shot if in normal or automatic runMode.
		if(! (READ32(m_pReg, SeqControl(seqRam->getId())) & EVG_SEQ_RAM_SINGLE))
			BITSET32(m_pReg, SeqControl(seqRam->getId()), EVG_SEQ_RAM_SINGLE);

		//Adding the record to the 'list of records to be processed' on a particular 
		//interrupt and also enabling that interrupt. 
		if(seqRam->getId() == 0) {
			printf("Adding Record List 0\n");
			epicsMutexLock(m_owner->irqStop0.mutex);
			m_owner->irqStop0.recList.push_back(pRec);
			epicsMutexUnlock(m_owner->irqStop0.mutex);
			WRITE32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM0);
			BITSET32(m_pReg, IrqEnable, EVG_IRQ_STOP_RAM0);		
		}
		else if(seqRam->getId() == 1) {
			printf("Adding Record List 1\n");
			epicsMutexLock(m_owner->irqStop1.mutex);
			m_owner->irqStop1.recList.push_back(pRec);
			epicsMutexUnlock(m_owner->irqStop1.mutex);
			WRITE32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM1);
			BITSET32(m_pReg, IrqEnable, EVG_IRQ_STOP_RAM1);
		}
		else {
			printf("ERROR: Corrupted seqRam object.\n");
			return ERROR;
		}

		printf("Waiting for SeqRam %d to be disabled.\n", seqRam->getId());		
		return OK;		
	}
	
	if(pRec->pact == 1) {
		//Enable the commitSeq record.
		pRec->pact = 0; 	
		printf("SeqRam disabled. Continuing\n");
	}

	seqRam->setEventCode(seq->getEventCode());
	seqRam->setTimeStamp(seq->getTimeStamp());
	seqRam->setTrigSrc(seq->getTrigSrc());
	seqRam->setRunMode(seq->getRunMode());
	//enable(seq);

	return OK;
}


epicsStatus
evgSeqRamMgr::enable(evgSequence* seq) {
	evgSeqRam* seqRam = findSeqRam(seq);
	printf("Enabling seq %d in seqRam %d\n",seq->getId(), seqRam->getId());
	if( (!seqRam) || seqRam->enabled() )
		return OK;
	else
		seqRam->enable();

	return OK;
}


epicsStatus
evgSeqRamMgr::disable(evgSequence* seq) {
	evgSeqRam* seqRam = findSeqRam(seq);
	if( (!seqRam) || (!seqRam->enabled()) )
		return OK;
	else
		seqRam->disable();

	return OK;
}