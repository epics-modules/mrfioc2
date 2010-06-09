#ifndef EVGSEQRAM_H
#define EVGSEQRAM_H

#include <vector>

#include <epicsTypes.h>
#include <dbCommon.h>

#include "evgSequence.h"

class evgSeqRam {

public:
	evgSeqRam(const epicsUInt32, volatile epicsUInt8* const);
	~evgSeqRam();

	const epicsUInt32 getId();

	epicsStatus setEventCode(std::vector<epicsUInt8>);
	epicsStatus setTimeStamp(std::vector<epicsUInt32>);

	epicsStatus setSoftTrig(bool);
	epicsStatus setTrigSrc(TrigSrc);
	epicsStatus setRunMode(RunMode);

	epicsStatus enable();
	epicsStatus disable();
	epicsStatus reset();

	bool enabled() const;
	bool running() const;

	epicsStatus load(evgSequence* seq);
	epicsStatus unload();
	bool loaded() const;

	evgSequence* getSequence();

private:
	const epicsUInt32			m_id;
	volatile epicsUInt8* const	m_pReg;
	bool 						m_allocated;
	evgSequence*	 			m_seq;
};

#endif //EVGSEQRAM_H