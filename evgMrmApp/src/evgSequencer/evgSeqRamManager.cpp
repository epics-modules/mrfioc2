#include "evgSeqRamManager.h"

#include <iostream>

#include <errlog.h>
#include <longoutRecord.h>

#include <mrfCommonIO.h>
#include <mrfCommon.h>

#include "evgRegMap.h"
#include "evgMrm.h"

evgSeqRamMgr::evgSeqRamMgr(evgMrm* owner):
m_owner(owner),
m_pReg(owner->getRegAddr()) {
	for(int i = 0; i < evgNumSeqRam; i++) {
		m_seqRam.push_back(new evgSeqRam(i, m_pReg));
	}
}

evgSeqRam* 
evgSeqRamMgr::getSeqRam(epicsUInt32 seqRamId) {
	if(	seqRamId < m_seqRam.size() )
		return m_seqRam[seqRamId];
	else 
		return 0;
}

epicsUInt32
evgSeqRamMgr::numOfRams() {
	return m_seqRam.size();
}


