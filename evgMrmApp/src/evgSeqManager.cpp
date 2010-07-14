#include "evgSeqManager.h"

#include <mrfCommon.h>

epicsStatus
evgSeqMgr::createSeq(epicsUInt32 seqID) {
	m_sequence.push_back(new evgSequence(seqID));
	printf("Created Seq %d\n", seqID);
	return OK;
}

evgSequence* 
evgSeqMgr::getSequence(epicsUInt32 seqId) {
	if(seqId < m_sequence.size() )
		return m_sequence[seqId];
	else 
		return OK;
}