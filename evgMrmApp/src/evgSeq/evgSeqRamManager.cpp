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

	for(int i = 0; i < evgNumSeqRam; i++) {
		m_seqRam.push_back(new evgSeqRam(i, pReg));
	}

}

evgSeqRam* 
evgSeqRamMgr::getSeqRam(epicsUInt32 seqRamId) {
	if(	seqRamId < m_seqRam.size() )
		return m_seqRam[seqRamId];
	else 
		return 0;
}

/*
 * Check if any of the sequenceRam is available(i.e. unloaded).
 * if avaiable load and commit the sequence else return an error.
 */
epicsStatus
evgSeqRamMgr::load(evgSequence* seq) {
	if(!seq) {
		errlogPrintf("ERROR: No sequence to load.\n");
		return ERROR;	
	}
		
	evgSeqRam* seqRam = 0;

	for(unsigned int i = 0; i < m_seqRam.size(); i++) {
		if(m_seqRam[i]->loaded()) {
			if(seq == m_seqRam[i]->getSequence()) {
				errlogPrintf("Seq %d already loaded.\n",seq->getId());
				return OK;
			}
		} else {
			if( !seqRam )
				seqRam = m_seqRam[i];
		}
	}

	if(seqRam != 0) {
		printf("Loading Seq %d in SeqRam %d\n",seq->getId(), seqRam->getId());
		seq->setSeqRam(seqRam);
		seqRam->load(seq);
		commit(seq, 0);
		
		return OK; 	
	} else {
		errlogPrintf("ERROR: Cannot load sequence.\n");
		return ERROR;	
	}
}
	
/* Unload does not wait for the sequecne to get over..Should it? */
epicsStatus
evgSeqRamMgr::unload(evgSequence* seq, dbCommon* pRec) {
	if(!seq) {
		errlogPrintf("ERROR: No sequence to unload.\n");
		return ERROR;	
	}

	evgSeqRam* seqRam = seq->getSeqRam();
	if(!seqRam)
		return OK;
	printf("Unloading Seq %d from SeqRam %d\n", seq->getId(), seqRam->getId());

	if( seqRam->enabled() ) {
		//Disable the unloadSeq record.
		pRec->pact = 1;		
		
		//Clear the SeqRam stop flag.
		WRITE32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM(seqRam->getId()));

		//Make the SeqRam single shot if in normal or automatic runMode.
		if(! (READ32(m_pReg, SeqControl(seqRam->getId())) & EVG_SEQ_RAM_SINGLE))
			BITSET32(m_pReg, SeqControl(seqRam->getId()), EVG_SEQ_RAM_SINGLE);

		//Adding the record to the 'list of records to be processed' on SEQ_RAM_STOP 
		//interrupt.
		if(seqRam->getId() == 0) {
 			printf("	Adding Record List 0\n");
			epicsMutexLock(m_owner->irqStop0.mutex);
			m_owner->irqStop0.recList.push_back(pRec);
			epicsMutexUnlock(m_owner->irqStop0.mutex);
    	} else if(seqRam->getId() == 1) {
 			printf("	Adding Record List 1\n");
			epicsMutexLock(m_owner->irqStop1.mutex);
 			m_owner->irqStop1.recList.push_back(pRec);
 			epicsMutexUnlock(m_owner->irqStop1.mutex);
		}

		//Enabling the SEQ_RAM_STOP interrupt
		BITSET32(m_pReg, IrqEnable, EVG_IRQ_STOP_RAM(seqRam->getId()));		

		printf("	Waiting for SeqRam %d to be disabled. Pausing unload.\n", seqRam->getId());		
		return OK;		
	}
	
	//This part of the function is executed only when the sequenceRam is disabled.
	if(pRec->pact == 1) {
		//Enable the commitSeq record.
		pRec->pact = 0; 	
		printf("	SeqRam disabled. Continuing unload.\n");
	}

	seq->setSeqRam(0);
	seqRam->unload();

	return OK;
}
	
/*
 * If the sequenceRam is not enabled then copy the sequence to the 
 * sequenceRam but if the sequenceRam is enabled then add the record 
 * to the 'list of records to be processed' on seqRam stop interupt.
 */
epicsStatus
evgSeqRamMgr::commit(evgSequence* seq, dbCommon* pRec) {
	if(!seq) {
		errlogPrintf("ERROR: No sequence to commit.\n");
		return ERROR;	
	}

	evgSeqRam* seqRam = seq->getSeqRam();
	if(!seqRam)
		return OK;

	printf("Commiting Seq %d to SeqRam %d\n", seq->getId(), seqRam->getId());

	if( seqRam->enabled() ) {
		//Disable the commmitSeq record.
		pRec->pact = 1;		
		
		//Clear the SeqRam stop flag.
		WRITE32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM(seqRam->getId()));

		//Make the SeqRam single shot if in normal or automatic runMode.
		if(! (READ32(m_pReg, SeqControl(seqRam->getId())) & EVG_SEQ_RAM_SINGLE))
			BITSET32(m_pReg, SeqControl(seqRam->getId()), EVG_SEQ_RAM_SINGLE);

		//Adding the record to the 'list of records to be processed' on SEQ_RAM_STOP 
		//interrupt.
		if(seqRam->getId() == 0) {
 			printf("	Adding Record List 0\n");
			epicsMutexLock(m_owner->irqStop0.mutex);
			m_owner->irqStop0.recList.push_back(pRec);
			epicsMutexUnlock(m_owner->irqStop0.mutex);
    	} else if(seqRam->getId() == 1) {
 			printf("	Adding Record List 1\n");
			epicsMutexLock(m_owner->irqStop1.mutex);
 			m_owner->irqStop1.recList.push_back(pRec);
 			epicsMutexUnlock(m_owner->irqStop1.mutex);
		}
		
		//Enabling the SEQ_RAM_STOP interrupt
		BITSET32(m_pReg, IrqEnable, EVG_IRQ_STOP_RAM(seqRam->getId()));		

		printf("	Waiting for SeqRam %d to be disabled. Pausing Commit.\n", seqRam->getId());		
		return OK;		
	}

	//This part of the function is executed only when the sequenceRam is disabled.	
	if(pRec->pact == 1) {
		//Enable the commitSeq record.
		pRec->pact = 0; 	
		printf("	SeqRam disabled. Continuing commit.\n");
	}

	seq->getLock()->lock();
	seqRam->setEventCode(seq->getEventCode());
	seqRam->setTimeStamp(seq->getTimeStamp());
	seqRam->setTrigSrc(seq->getTrigSrc());
	seqRam->setRunMode(seq->getRunMode());
	seq->getLock()->unlock();

	enable(seq);
	return OK;
}


epicsStatus
evgSeqRamMgr::enable(evgSequence* seq) {
	if(!seq) {
		errlogPrintf("ERROR: No sequence to enable.\n");
		return ERROR;	
	}

	evgSeqRam* seqRam = seq->getSeqRam();
	if( (!seqRam) || seqRam->enabled() )
		return OK;
	else
		seqRam->enable();

	printf("Enabling seq %d in seqRam %d\n",seq->getId(), seqRam->getId());
	return OK;
}

epicsStatus
evgSeqRamMgr::disable(evgSequence* seq) {
	if(!seq) {
		errlogPrintf("ERROR: No sequence to disable.\n");
		return ERROR;	
	}

	evgSeqRam* seqRam = seq->getSeqRam();
	if( (!seqRam) || (!seqRam->enabled()) )
		return OK;
	else { 
		//If the seq is not in single modem make it run in single mode.
		if(! (READ32(m_pReg, SeqControl(seqRam->getId())) & EVG_SEQ_RAM_SINGLE))
			BITSET32(m_pReg, SeqControl(seqRam->getId()), EVG_SEQ_RAM_SINGLE);
	}

	printf("Disabling seq %d in seqRam %d\n",seq->getId(), seqRam->getId());
	return OK;
}

epicsStatus
evgSeqRamMgr::halt(evgSequence* seq) {
	if(!seq) {
		errlogPrintf("ERROR: No sequence to halt.\n");
		return ERROR;	
	}

	evgSeqRam* seqRam = seq->getSeqRam();
	if( (!seqRam) || (!seqRam->enabled()) )
		return OK;
	else {
		seqRam->disable();

		//satisfy any call back request, on irqStop0 or irqStop1 recList.
		if(seqRam->getId() == 0)
			callbackRequest(&m_owner->irqStop0_cb);
		else 
			callbackRequest(&m_owner->irqStop1_cb);
	}

	return OK;
}

