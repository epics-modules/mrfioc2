#ifndef EVG_SEQ_RAM_SUP_H
#define EVG_SEQ_RAM_SUP_H

#include <vector>

#include <epicsTypes.h>

#include "evgSeqRam.h"
#include "evgSequence.h"

class evgMrm;

class evgSeqRamSup {
public:
	evgSeqRamSup(volatile epicsUInt8* const, evgMrm*);
	
	epicsUInt32 findSeqRamId(epicsUInt32);
	evgSeqRam* findSeqRam(epicsUInt32);
	evgSeqRam* getSeqRam(epicsUInt32);
	evgSequence* getSequence(epicsUInt32);
	
	epicsStatus load(epicsUInt32);
	epicsStatus unload(epicsUInt32);
	epicsStatus commit(epicsUInt32, dbCommon*);
	epicsStatus enable(epicsUInt32);
	epicsStatus disable(epicsUInt32);

private:
	evgMrm*							m_owner;
	volatile epicsUInt8* const 		m_pReg;
	std::vector<evgSeqRam*> 		m_seqRam;
	std::vector<evgSequence*> 		m_sequence;
};

#endif //EVG_SEQ_RAM_SUP_H

