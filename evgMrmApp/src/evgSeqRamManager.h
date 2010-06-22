#ifndef EVG_SEQ_RAM_SUP_H
#define EVG_SEQ_RAM_SUP_H

#include <vector>

#include <epicsTypes.h>

#include "evgSeqRam.h"
#include "evgSequence.h"

class evgMrm;

class evgSeqRamMgr {
public:
	evgSeqRamMgr(volatile epicsUInt8* const, evgMrm*);
	
	epicsUInt32 findSeqRamId(evgSequence *);
	evgSeqRam* findSeqRam(evgSequence *);
	evgSeqRam* getSeqRam(epicsUInt32);
	evgSequence* getSequence(epicsUInt32);
	
	epicsStatus load(evgSequence *);
	epicsStatus unload(evgSequence *);
	epicsStatus commit(evgSequence *, dbCommon*);
	epicsStatus enable(evgSequence *);
	epicsStatus disable(evgSequence *);

private:
	evgMrm*							m_owner;
	volatile epicsUInt8* const 		m_pReg;
	std::vector<evgSeqRam*> 		m_seqRam;
	std::vector<evgSequence*> 		m_sequence;
};

#endif //EVG_SEQ_RAM_SUP_H

