#include "evgSoftSeqManager.h"

#include <mrfCommon.h>

evgSoftSeqMgr::evgSoftSeqMgr(evgMrm* const owner):
m_owner(owner) {
}

evgSoftSeq* 
evgSoftSeqMgr::getSoftSeq(epicsUInt32 seqId) {
	evgSoftSeq* seq = m_softSeq[seqId];

	if(!seq) {
		seq = new evgSoftSeq(seqId, m_owner);
		m_softSeq[seqId] = seq;
	}

	return seq;
}