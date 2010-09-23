#include "evgSoftSeqManager.h"

#include <mrfCommon.h>

evgSoftSeqMgr::evgSoftSeqMgr(evgMrm* const owner):
m_owner(owner),
m_lock() {
}

evgSoftSeq* 
evgSoftSeqMgr::getSoftSeq(epicsUInt32 seqId) {
	SCOPED_LOCK(m_lock);
	evgSoftSeq* seq = m_softSeq[seqId];

	if(!seq) {
		seq = new evgSoftSeq(seqId, m_owner);
		m_softSeq[seqId] = seq;
	}

	return seq;
}
