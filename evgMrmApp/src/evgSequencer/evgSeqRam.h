#ifndef EVGSEQRAM_H
#define EVGSEQRAM_H

#include <vector>

#include <epicsTypes.h>
#include <dbCommon.h>

#include "evgSoftSeq.h"

class evgSeqRam {

public:
	evgSeqRam(const epicsUInt32, volatile epicsUInt8* const);
	~evgSeqRam();

	const epicsUInt32 getId();

	epicsStatus setEventCode(std::vector<epicsUInt8>);
	epicsStatus setTimeStamp(std::vector<epicsUInt32>);

	epicsStatus setSoftTrig();
	epicsStatus setTrigSrc(SeqTrigSrc);
	SeqTrigSrc getTrigSrc();
	epicsStatus setRunMode(SeqRunMode);
	SeqRunMode getRunMode();

	epicsStatus enable();
	epicsStatus disable();
	epicsStatus reset();

	bool enabled() const;
	bool running() const;

	epicsStatus alloc(evgSoftSeq* seq);
	epicsStatus dealloc();
	bool IsAlloc() const;

	evgSoftSeq* getSoftSeq();

private:
	const epicsUInt32			m_id;
	volatile epicsUInt8* const	m_pReg;
	bool 						m_allocated;
	evgSoftSeq*	 				m_softSeq;
};

#endif //EVGSEQRAM_H
