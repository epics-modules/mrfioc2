#ifndef EVG_SEQ_MGR_H
#define EVG_SEQ_MGR_H

#include <map>

#include <epicsTypes.h>

#include "evgSequence.h"

class evgSeqMgr {
public:
	evgSeqMgr(evgMrm* const);
	evgSequence* getSeq(epicsUInt32);

private:
	evgMrm*	const							m_owner;
	std::map<epicsUInt32, evgSequence*> 	m_softSeq;
};
#endif //EVG_SEQ_MGR_H