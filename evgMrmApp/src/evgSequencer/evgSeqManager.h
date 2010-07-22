#ifndef EVG_SEQ_MGR_H
#define EVG_SEQ_MGR_H

#include <vector>

#include <epicsTypes.h>

#include "evgSequence.h"

class evgSeqMgr {
public:
	evgSeqMgr(evgMrm* const);
	epicsStatus	createSeq(epicsUInt32);
	evgSequence* getSeq(epicsUInt32);

private:
	evgMrm*	const					m_owner;
	std::vector<evgSequence*> 		m_sequence;
};
#endif //EVG_SEQ_MGR_H