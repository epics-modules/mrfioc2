#ifndef EVG_SEQ_RAM_MGR_H
#define EVG_SEQ_RAM_MGR_H

#include <vector>

#include <epicsTypes.h>

#include "evgSeqRam.h"

class evgMrm;

class evgSeqRamMgr {
public:
    evgSeqRamMgr(evgMrm*);	

    evgSeqRam* getSeqRam(epicsUInt32);	
    epicsUInt32 numOfRams();

private:
    evgMrm*                 m_owner;
    std::vector<evgSeqRam*> m_seqRam;
};

#endif //EVG_SEQ_RAM_MGR_H

