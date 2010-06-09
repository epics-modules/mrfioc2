#include "evgSeqRamSup.h"

#include <iostream>

#include <errlog.h>

#include <mrfCommon.h>

#include "evgRegMap.h"

std::vector<dbCommon*>	recList;

evgSeqRamSup::evgSeqRamSup(volatile epicsUInt8* const pReg):
m_pReg(pReg) {
	
	for(int i = 1; i <= 3; i++) {
		m_sequence.push_back(new evgSequence(i));
	}

	for(int i = 1; i <= evgNumSeqRam; i++) {
		m_seqRam.push_back(new evgSeqRam(i, pReg));
	}

}

/* Functions calling this function should always 
 *  check for valid return values
 */
evgSeqRam*
evgSeqRamSup::findSeqRam(epicsUInt32 seqId) {
	evgSequence* seq = 0;
	evgSeqRam* seqRam = 0;

	std::vector<evgSeqRam*>::iterator it;
	for(it = m_seqRam.begin(); it < m_seqRam.end(); it++) {
		seq = ((evgSeqRam*)(*it))->getSequence();
		if( seq && seq->getId() == seqId) {
			seqRam = *it;	
		}
	}

	return seqRam;
}

/* Functions calling this function should always 
 *  check for valid return values
 */
epicsUInt32
evgSeqRamSup::findSeqRamId(epicsUInt32 seqId) {
	evgSeqRam* seqRam = findSeqRam(seqId);
	if(seqRam)
		return seqRam->getId();

	return -1;
}

/* Functions calling this function should always 
 *  check for valid return values
 */
evgSeqRam* 
evgSeqRamSup::getSeqRam(epicsUInt32 seqRamId) {
	if(	seqRamId && seqRamId <= m_seqRam.size() )
		return m_seqRam[seqRamId - 1];
	else 
		return 0;
}

/* Functions calling this function should always 
 *  check for valid return values
 */
evgSequence* 
evgSeqRamSup::getSequence(epicsUInt32 seqId) {
	if(seqId &&	seqId <= m_sequence.size() )
		return m_sequence[seqId - 1];
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
	printf("Entering evgSeqRamSup::load\n");
	evgSeqRam* seqRam = 0;

	std::vector<evgSeqRam*>::iterator it;
	for(it = m_seqRam.begin(); it < m_seqRam.end(); it++) {
		if(! ((evgSeqRam*)(*it))->loaded() ) {
			seqRam = (*it);
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
		
		printf("Leaving evgSeqRamSup::load\n");
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
	if(seqRam) 
		seqRam->unload();
	
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
	printf("Entering evgSeqRamSup::commit\n");
	evgSeqRam* seqRam = findSeqRam(seqId);
	if(!seqRam)
		return OK;

	printf("Commiting Seq %d to SeqRam %d\n", seqId, seqRam->getId());
	if( seqRam->enabled() ) {
		//Disable the commitSeq record.
		//Make the record single shot if not already.
		recList.push_back(pRec);
		printf("Leaving evgSeqRamSup::commit with Error\n");
		return ERROR;
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

	printf("Leaving evgSeqRamSup::commit\n");
	return OK;
}


epicsStatus
evgSeqRamSup::enable(epicsUInt32 seqId) {
	evgSeqRam* seqRam = findSeqRam(seqId);
	//printf("Enabling Seq %d in SeqRam %d\n", seqId, seqRam->getId());
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