#ifndef EVG_SEQ_RAM_MGR_H
#define EVG_SEQ_RAM_MGR_H

#include <vector>

#include <epicsTypes.h>

#include "evgSequence.h"
#include "evgSeqRam.h"

class evgMrm;

class evgSeqRamMgr {
public:
	evgSeqRamMgr(evgMrm*);
	
	evgSeqRam* getSeqRam(epicsUInt32);
	
	epicsStatus load	(evgSequence *);
	epicsStatus unload	(evgSequence *, dbCommon *);
	epicsStatus commit	(evgSequence *, dbCommon *);
	epicsStatus enable	(evgSequence *);
	epicsStatus disable	(evgSequence *);
	epicsStatus halt	(evgSequence *);

private:
	evgMrm*							m_owner;
	volatile epicsUInt8* const 		m_pReg;
	std::vector<evgSeqRam*> 		m_seqRam;
};

#endif //EVG_SEQ_RAM_MGR_H

