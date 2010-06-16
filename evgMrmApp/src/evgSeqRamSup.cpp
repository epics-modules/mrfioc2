#include "evgSeqRamSup.h"

#include <iostream>

#include <errlog.h>
#include <longoutRecord.h>
#include <mrfCommon.h>

#include "evgRegMap.h"
#include "evgMrm.h"

evgSeqRamSup::evgSeqRamSup(volatile epicsUInt8* const pReg, evgMrm* owner):
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
evgSeqRamSup::findSeqRam(epicsUInt32 seqId) {
	evgSequence* seq = 0;

	for(unsigned int i = 0; i < m_seqRam.size(); i++) {
		seq = m_seqRam[i]->getSequence();
		if( seq && seq->getId() == seqId) {
			return m_seqRam[i];	
		}
	}

	return 0;
}

epicsUInt32
evgSeqRamSup::findSeqRamId(epicsUInt32 seqId) {
	evgSeqRam* seqRam = findSeqRam(seqId);
	if(seqRam)
		return seqRam->getId();

	return -1;
}

evgSeqRam* 
evgSeqRamSup::getSeqRam(epicsUInt32 seqRamId) {
	if(	seqRamId < m_seqRam.size() )
		return m_seqRam[seqRamId];
	else 
		return 0;
}

evgSequence* 
evgSeqRamSup::getSequence(epicsUInt32 seqId) {
	if(seqId < m_sequence.size() )
		return m_sequence[seqId];
	else 
		return 0;
}

/*
 * Load()
 * @parm     :  The sequence Id of the sequence to be loaded.
 * @functiom : 	Check if any of the sequenceRam is available(i.e. unloaded).
 *			    if avaiable load and commit the sequence else return an error.
 */
epicsStatus
evgSeqRamSup::load(epicsUInt32 seqId) {
	//printf("Entering evgSeqRamSup::load\n");
	evgSeqRam* seqRam = 0;

	for(unsigned int i = 0; i < m_seqRam.size(); i++) {
		if(m_seqRam[i]->loaded()) {
			evgSequence* seq = m_seqRam[i]->getSequence();
			if(seqId == seq->getId()) {
				printf("Sequence %d already loaded.\n",seqId);
				return OK;
			} 
		}
	}

	for(unsigned int i = 0; i < m_seqRam.size(); i++) {
		if(! m_seqRam[i]->loaded() ) {
			seqRam = m_seqRam[i];
			break; 
		}
	}

	if(seqRam != 0) {
		printf("Loading Seq %d in SeqRam %d\n",seqId, seqRam->getId());
		evgSequence* seq = getSequence(seqId);
		if(!seq) {
			errlogPrintf("Error: Sequence %d not created.\n",seqId);
			return ERROR;
		}
		
		seqRam->load(seq);
		commit(seqId, 0);
		//enable(seqId);
		
		//printf("Leaving evgSeqRamSup::load\n");
		return OK; 	
	} else {
		errlogPrintf("ERROR: Cannot load sequence.\n");
		return ERROR;	
	}
}

/*
 * Unload()
 * @parm 	 :	The sequence Id of the sequence to be unloaded.
 * @function : 	Unload the sequenceRam with given sequence ID. 
 */
epicsStatus
evgSeqRamSup::unload(epicsUInt32 seqId) {
	evgSeqRam* seqRam = findSeqRam(seqId);
	if(seqRam) {
		printf("Unloading seq %d in SeqRam %d\n",seqId, seqRam->getId());
		seqRam->unload();
	}

	return OK;
}

/*
 * Commit()
 * @parm 	 : 	The sequence Id of the sequence to be committed.
 *				Pointer to a commitSeq record. 
 * @function :  If the sequenceRam is not enabled then copy the sequence to the 
 *				sequenceRam but if the sequenceRam is enabled then add the record 
 *				to the list of records to be processed on seqRam stop interupt.
 */
epicsStatus
evgSeqRamSup::commit(epicsUInt32 seqId, dbCommon* pRec) {
	evgSeqRam* seqRam = findSeqRam(seqId);
	if(!seqRam)
		return OK;

// 	if(pRec->pact == 0) {
// 		pRec->pact = 1;
// 		m_owner->irqStop0.recList.push_back(pRec);
// 		return OK;
// 	}

	printf("Commiting Seq %d to SeqRam %d\n", seqId, seqRam->getId());
	if( seqRam->enabled() ) {
		printf("Waiting for SeqRam to be disabled.\n");
		epicsMutexLock(m_owner->irqStop0.mutex);
// 		//Disable the commitSeq record.
// 		pRec->pact = 1;
// 		
//		MutexLock and Unlock
//
// 		//Make the record single shot if not already.
// 		
// 		//recList.push_back(pRec);
// 		if(seqRam->getId() == 0)
// 			irqStop0.push_back(pRec);
// 		else if(seqRam->getId() == 1)
// 			 irqStop1.push_back(pRec);
// 		else {
// 			printf("ERROR: Corrupted seqRam object.\n");
// 			return ERROR;
// 		}
// 		
// 		return OK;
		epicsMutexUnlock(m_owner->irqStop0.mutex);
	}
	
	if(pRec->pact == 1) {
		pRec->pact = 0; 	//Enable the commitSeq record.
		printf("SeqRam disabled. Continuing.\n");
	}

	evgSequence* seq = getSequence(seqId);
	if(!seq) {
		errlogPrintf("Error: Sequence %d not created.\n",seqId);
		return ERROR;
	}
	seqRam->setEventCode(seq->getEventCodeV());
	seqRam->setTimeStamp(seq->getTimeStampV());
	seqRam->setTrigSrc(seq->getTrigSrc());
	seqRam->setRunMode(seq->getRunMode());
	//enable(seqId);

	//printf("Leaving evgSeqRamSup::commit\n");
	return OK;
}


epicsStatus
evgSeqRamSup::enable(epicsUInt32 seqId) {
	evgSeqRam* seqRam = findSeqRam(seqId);
	if( (!seqRam) || seqRam->enabled() )
		return OK;
	else
		seqRam->enable();

	return OK;
}


epicsStatus
evgSeqRamSup::disable(epicsUInt32 seqId) {
	evgSeqRam* seqRam = findSeqRam(seqId);
	if( (!seqRam) || (!seqRam->enabled()) )
		return OK;
	else
		seqRam->disable();

	return OK;
}