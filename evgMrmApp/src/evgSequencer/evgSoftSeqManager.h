#ifndef EVG_SEQ_MGR_H
#define EVG_SEQ_MGR_H

#include <map>

#include <epicsTypes.h>

#include "evgSoftSeq.h"

class evgSoftSeqMgr {
public:
	evgSoftSeqMgr(evgMrm* const);
	evgSoftSeq* getSoftSeq(epicsUInt32);

private:
	evgMrm*	const						m_owner;
	std::map<epicsUInt32, evgSoftSeq*> 	m_softSeq;
};
#endif //EVG_SEQ_MGR_H
