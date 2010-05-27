#include "evgSeqRamSup.h"

#include <iostream>

#include <errlog.h>

#include <mrfCommon.h>

#include "evgRegMap.h"

evgSeqRamSup::evgSeqRamSup(volatile epicsUInt8* const pReg):
m_pReg(pReg) {
	for(int i = 0; i < evgNumSeqRam; i++) {
		//m_seqRam[i] = new evgSeqRam(i, pReg);
		m_seqRam.push_back(new evgSeqRam(i, pReg));
	}

	for(int i = 0; i < 3; i++) {
		//m_sequence[i] = new evgSequence(i);
		m_sequence.push_back(new evgSequence(i));
	}
}

/* Functions calling this function should always 
 *  check for valid return values
 */
evgSeqRam*
evgSeqRamSup::getSeqRam(epicsUInt32 seqId) {
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
evgSeqRamSup::getSeqRamId(epicsUInt32 seqId) {
	evgSeqRam* seqRam = getSeqRam(seqId);
	if(seqRam)
		return seqRam->getId();

	return -1;
}

/* Functions calling this function should always 
 *  check for valid return values
 */
evgSequence* 
evgSeqRamSup::getSequence(epicsUInt32 seqId) {
	if(	seqId < m_sequence.size() )
		return m_sequence[seqId];
	else 
		return 0;
}

/*
 * Load()
 * @return   :	The sequenceRam ID in which this particular sequence is loaded.
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
		printf("	Loading Seq %d in SeqRam %d\n",seqId, seqRam->getId());
		evgSequence* seq = getSequence(seqId);
		if(!seq) {
			errlogPrintf("Error: Sequence %d not created.\n",seqId);
			return ERROR;
		}
		
		seqRam->load(seq);
		commit(seqId);
		enable(seqId);
		
		printf("Leaving evgSeqRamSup::load\n");
		return OK; 	
	} else {
		errlogPrintf("ERROR: Cannot load sequence.\n");
		return ERROR;	
	}
}

/*
 * Unload()
 * @parm 	 :	The ID of the SequenceRam
 * @function : 	Unload the sequenceRam with given ID. Call reset for that sequenceRam and 
 * 				mark the sequenceRam as unloaded.
 */
epicsStatus
evgSeqRamSup::unload(epicsUInt32 seqId) {
	evgSeqRam* seqRam = getSeqRam(seqId);
	if(seqRam) 
		seqRam->unload();
	
	return OK;
}

/*
 * Commit()
 * @parm 	 : 	The sequence to be commited and sequenceRam to which the sequence is to
 *				be committed.
 * @function :  Check if the sequenceRam is loaded and disabled, if not return an error. 
 *				If loaded and disabled then copy the Event Code and Time Stamp arrays 
 *				from passed sequence to the sequenceRam along with sequence ID.
 */
epicsStatus
evgSeqRamSup::commit(epicsUInt32 seqId) {
	printf("Entering evgSeqRamSup::commit\n");
	evgSeqRam* seqRam = getSeqRam(seqId);
	if(!seqRam)
		return OK;
	
	printf("	Commiting Seq %d to SeqRam %d\n", seqId, seqRam->getId());
	if( seqRam->enabled() ) {
		//TODO:disable it
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
	enable(seqId);

	printf("Leaving evgSeqRamSup::commit\n");
	return OK;
}


epicsStatus
evgSeqRamSup::enable(epicsUInt32 seqId) {
	evgSeqRam* seqRam = getSeqRam(seqId);
	if( (!seqRam) || seqRam->enabled() )
		return OK;
	else
		seqRam->enable();

	return OK;
}


epicsStatus
evgSeqRamSup::disable(epicsUInt32 seqId) {
	evgSeqRam* seqRam = getSeqRam(seqId);
	if( (!seqRam) || (!seqRam->enabled()) )
		return OK;
	else
		seqRam->disable();

	return OK;
}