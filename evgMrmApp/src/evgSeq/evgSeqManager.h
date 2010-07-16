#ifndef EVG_SEQ_MGR_H
#define EVG_SEQ_MGR_H

#include <vector>

#include <epicsTypes.h>

#include "evgSequence.h"

class evgSeqMgr {
public:
	epicsStatus	createSeq(epicsUInt32);
	evgSequence* getSeq(epicsUInt32);

private:
	std::vector<evgSequence*> 		m_sequence;
};
#endif //EVG_SEQ_MGR_H