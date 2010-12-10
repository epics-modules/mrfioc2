#include "evgSeqRam.h"

#include <iostream>
#include <stdexcept>

#include <errlog.h>

#include <mrfCommonIO.h>
#include <mrfCommon.h>

#include "evgRegMap.h"

evgSeqRam::evgSeqRam(const epicsUInt32 id, volatile epicsUInt8* const pReg):
m_id(id),
m_pReg(pReg),
m_allocated(0),
m_softSeq(0) {
}

const epicsUInt32 
evgSeqRam::getId() {
    return m_id;
}

epicsStatus
evgSeqRam::setEventCode(std::vector<epicsUInt8> eventCode) {
    for(unsigned int i = 0; i < eventCode.size(); i++) {
        WRITE8(m_pReg, SeqRamEvent(m_id,i), eventCode[i]);
    }

    return OK;
}

epicsStatus
evgSeqRam::setTimestamp(std::vector<uint64_t> timestamp){
    for(unsigned int i = 0; i < timestamp.size(); i++) {
        WRITE32(m_pReg, SeqRamTS(m_id,i), (epicsUInt32)timestamp[i]);
    }
	
    return OK;
}

epicsStatus
evgSeqRam::setSoftTrig() {
    BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SW_TRIG);
    return OK;
}

epicsStatus
evgSeqRam::setTrigSrc(SeqTrigSrc trigSrc) {
    WRITE8(m_pReg, SeqTrigSrc(m_id), trigSrc);
    return OK;
}

SeqTrigSrc
evgSeqRam::getTrigSrc() {
    return (SeqTrigSrc)READ8(m_pReg, SeqTrigSrc(m_id));
}

epicsStatus
evgSeqRam::setRunMode(SeqRunMode mode) {
    switch (mode) {
        case(Normal):
            BITCLR32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SINGLE);
            BITCLR32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_RECYCLE);
            break;
        case(Auto):
            BITCLR32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SINGLE);
            BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_RECYCLE);
            break;	
        case(Single):
            BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SINGLE);
            break;	
        default:
            throw std::runtime_error("Unknown SeqRam RunMode");
    }
	
    return OK;
}

SeqRunMode
evgSeqRam::getRunMode() {
    if(READ32(m_pReg, SeqControl(m_id)) & EVG_SEQ_RAM_SINGLE)
        return Single;

    if(READ32(m_pReg, SeqControl(m_id)) & EVG_SEQ_RAM_RECYCLE)
        return Auto;
    else
        return Normal;
}

epicsStatus 
evgSeqRam::enable() {
    if(isAllocated()) {
        BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_ARM);
        return OK;
    } else 
        throw std::runtime_error("Trying to enable Unallocated seqRam");
}

epicsStatus 
evgSeqRam::disable() {
    BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_DISABLE);
    return OK;
}

epicsStatus 
evgSeqRam::reset() {
    BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_RESET);	
    return OK;
}

epicsStatus
evgSeqRam::alloc(evgSoftSeq* softSeq) {
    m_softSeq = softSeq;
    m_allocated = true;
    return OK;
}

epicsStatus
evgSeqRam::dealloc() {
    m_softSeq = 0;
    m_allocated = false;

    //clear interrupt flags
    BITSET32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM(m_id));	
    BITSET32(m_pReg, IrqFlag, EVG_IRQ_START_RAM(m_id));
    return OK;
}

bool 
evgSeqRam::isEnabled() const {
    return READ32(m_pReg, SeqControl(m_id)) & EVG_SEQ_RAM_ENABLED; 
}

bool 
evgSeqRam::isRunning() const {
    return    READ32(m_pReg, SeqControl(m_id)) & EVG_SEQ_RAM_RUNNING; 
}

bool
evgSeqRam::isAllocated() const {
    return m_allocated && m_softSeq;
}

evgSoftSeq* 
evgSeqRam::getSoftSeq() {
    return m_softSeq;
}
