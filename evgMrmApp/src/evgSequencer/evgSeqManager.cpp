#include "evgSeqManager.h"

#include <mrfCommon.h>

evgSeqMgr::evgSeqMgr(evgMrm* const owner):
m_owner(owner) {
}

epicsStatus
evgSeqMgr::createSeq(epicsUInt32 seqID) {
	m_sequence.push_back(new evgSequence(seqID, m_owner));
	return OK;
}

evgSequence* 
evgSeqMgr::getSeq(epicsUInt32 seqId) {
	if(seqId < m_sequence.size() )
		return m_sequence[seqId];
	else 
		return OK;
}