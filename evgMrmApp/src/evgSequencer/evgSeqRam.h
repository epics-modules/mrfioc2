#ifndef EVGSEQRAM_H
#define EVGSEQRAM_H

#include <vector>
#include <stdint.h>
#include <epicsTypes.h>
#include <dbCommon.h>

#include "evgSoftSeq.h"
class evgMrm;
class evgSeqRam {

public:
    evgSeqRam(const epicsUInt32, evgMrm* const);
    ~evgSeqRam();

    const epicsUInt32 getId();

    epicsStatus setEventCode(std::vector<epicsUInt8>);
    epicsStatus setTimestamp(std::vector<epicsUInt64>);

    epicsStatus setSoftTrig();
    epicsStatus setTrigSrc(SeqTrigSrc);
    SeqTrigSrc getTrigSrc();
    epicsStatus setRunMode(SeqRunMode);
    SeqRunMode getRunMode();

    epicsStatus enable();
    epicsStatus disable();
    epicsStatus reset();

    epicsStatus alloc(evgSoftSeq* seq);
    epicsStatus dealloc();

    bool isEnabled() const;
    bool isRunning() const;
    bool isAllocated() const;

    evgSoftSeq* getSoftSeq();

private:
    const epicsUInt32          m_id;
    evgMrm*                    m_owner;
    volatile epicsUInt8* const m_pReg;
    bool                       m_allocated;
    evgSoftSeq*                m_softSeq;
};

#endif //EVGSEQRAM_H
