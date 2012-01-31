#ifndef EVGSEQRAM_H
#define EVGSEQRAM_H

#include <vector>
#include <epicsTypes.h>
#include <dbCommon.h>

#include <mrfCommon.h>
#include "evgSoftSeq.h"
class evgMrm;
class evgSeqRam {

public:
    evgSeqRam(const epicsUInt32, evgMrm* const);
    ~evgSeqRam();

    const epicsUInt32 getId() const;

    void setEventCode(std::vector<epicsUInt8>);
    std::vector<epicsUInt8> getEventCode();

    void setTimestamp(std::vector<epicsUInt64>);
    std::vector<epicsUInt64> getTimestamp();

    void setTrigSrc(SeqTrigSrc);
    SeqTrigSrc getTrigSrc() const;

    void setRunMode(SeqRunMode);
    SeqRunMode getRunMode() const;

    void alloc(evgSoftSeq* seq);
    void dealloc();

    void softTrig();

    void enable();
    void disable();
    void reset();

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
