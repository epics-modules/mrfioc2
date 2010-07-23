#include "evgSeqManager.h"

#include <mrfCommon.h>

evgSeqMgr::evgSeqMgr(evgMrm* const owner):
m_owner(owner) {
}

evgSequence* 
evgSeqMgr::getSeq(epicsUInt32 seqId) {
	evgSequence* seq = m_softSeq[seqId];

	if(!seq) {
		seq = new evgSequence(seqId, m_owner);
		m_softSeq[seqId] = seq;
	}

	return seq;
}