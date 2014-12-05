#ifndef EVGSEQRAM_H
#define EVGSEQRAM_H

#include <vector>
#include <epicsTypes.h>
#include <dbCommon.h>

#include <mrfCommon.h>
#include "evgSoftSeq.h"

#include <evgInput.h>

class evgMrm;
class evgSeqRam {

public:
    evgSeqRam(const epicsUInt32, evgMrm* const);
    ~evgSeqRam();

    const epicsUInt32 getId() const{return m_id;}

    void setEventCode(const std::vector<epicsUInt8>&);
    std::vector<epicsUInt8> getEventCode();

    void setTimestamp(const std::vector<epicsUInt64>&);
    std::vector<epicsUInt64> getTimestamp();

    void setTrigSrc(SeqTrigSrc);
    SeqTrigSrc getTrigSrc() const;

    void setRunMode(SeqRunMode);
    SeqRunMode getRunMode() const;

    void disableSeqExtTrig(evgInput*);
    evgInput* findSeqExtTrig(evgInput*) const;

    bool alloc(evgSoftSeq* seq);
    void dealloc();

    void softTrig();

    void enable();
    void disable();
    void reset();

    bool isEnabled() const;
    bool isRunning() const;
    bool isAllocated() const;

    void process_sos();
    void process_eos();

    evgSoftSeq* getSoftSeq();

    const epicsUInt32          m_id;
    evgMrm* const              m_owner;
private:
    volatile epicsUInt8* const m_pReg;

    // Guarded with epicsInterruptLock
    evgSoftSeq*                m_softSeq;
};

#endif //EVGSEQRAM_H
