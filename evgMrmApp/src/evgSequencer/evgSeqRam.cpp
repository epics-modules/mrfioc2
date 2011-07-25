#include "evgSeqRam.h"

#include <iostream>
#include <stdexcept>
#include <stdlib.h>

#include <errlog.h>

#include <mrfCommonIO.h>
#include <mrfCommon.h>

#include "evgMrm.h"
#include "evgRegMap.h"

evgSeqRam::evgSeqRam(const epicsUInt32 id, evgMrm* const owner):
m_id(id),
m_owner(owner),
m_pReg(owner->getRegAddr()),
m_allocated(0),
m_softSeq(0) {
}

const epicsUInt32 
evgSeqRam::getId() const {
    return m_id;
}

void
evgSeqRam::setEventCode(std::vector<epicsUInt8> eventCode) {
    for(unsigned int i = 0; i < eventCode.size(); i++)
        WRITE8(m_pReg, SeqRamEvent(m_id,i), eventCode[i]);
}

std::vector<epicsUInt8>
evgSeqRam::getEventCode() {
     std::vector<epicsUInt8> eventCode(2048, 0);

    for(unsigned int i = 0; i < eventCode.size(); i++)
        eventCode[i] = READ8(m_pReg, SeqRamEvent(m_id,i));

    return eventCode;
}

void
evgSeqRam::setTimestamp(std::vector<epicsUInt64> timestamp){
    for(unsigned int i = 0; i < timestamp.size(); i++)
        WRITE32(m_pReg, SeqRamTS(m_id,i), (epicsUInt32)timestamp[i]);
}

std::vector<epicsUInt64>
evgSeqRam::getTimestamp() {
    std::vector<epicsUInt64> timestamp(2048, 0);

    for(unsigned int i = 0; i < timestamp.size(); i++)
        timestamp[i] = READ32(m_pReg, SeqRamTS(m_id,i));

    return timestamp;
}

void
evgSeqRam::softTrig() {
    BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SW_TRIG);
}

static void
disableSeqTrig(evgInput* inp, epicsUInt32 seqId) {
    epicsUInt32 map = inp->getSeqTrigMap();
    map = map & ~(seqId+1);
    inp->setSeqTrigMap(map);
}

void
evgSeqRam::setTrigSrc(SeqTrigSrc trigSrc) {
    if(trigSrc == Software){
        if(!m_id)
            trigSrc = SoftRam0;
        else
            trigSrc = SoftRam1;
    }

    if(trigSrc >= FrontInp0) {
       /*
        *First disable the triggering of this sequencer on all the external
        *inputs of EVG.
        */
        for(int i = 0; i < evgNumFrontInp; i++)
            disableSeqTrig(m_owner->getInput(i, FrontInp), m_id);

        for(int i = 0; i < evgNumUnivInp; i++)
            disableSeqTrig(m_owner->getInput(i, UnivInp), m_id);

        for(int i = 0; i < evgNumRearInp; i++)
            disableSeqTrig(m_owner->getInput(i, RearInp), m_id);
       /*
        *Now enable the triggering only on the appropraite external input of EVG.
        *Each external input is identified by its number and its type. The
        *SeqTrigSrc value for each input is chosen in such a way that when you
        *divide it by 4 the quotient will give you the input number and
        *remainder will give you the input type.
        */
        div_t divResult = div((epicsUInt32)trigSrc-40, 4);
        evgInput* inp = m_owner->getInput(divResult.quot, (InputType)divResult.rem);
        inp->setSeqTrigMap(m_id+1);

        if(!m_id)
            trigSrc = ExtRam0;
        else
            trigSrc = ExtRam1;
    }

    WRITE8(m_pReg, SeqTrigSrc(m_id), trigSrc);
}

static evgInput*
findSeqTrig(evgInput* inp, epicsUInt32 seqId) {
    epicsUInt32 map = inp->getSeqTrigMap();
    if(map & (seqId+1))
        return inp;
    else
        return 0;
}

SeqTrigSrc
evgSeqRam::getTrigSrc() const {
    SeqTrigSrc trigSrc = (SeqTrigSrc)READ8(m_pReg, SeqTrigSrc(m_id));

    if(trigSrc == SoftRam0 || trigSrc == SoftRam1){
        trigSrc = Software;
    }


    if(trigSrc == ExtRam0 || trigSrc == ExtRam1) {
        evgInput* inp = 0;

        for(int i = 0; i < evgNumFrontInp && inp == 0; i++)
            inp = findSeqTrig(m_owner->getInput(i, FrontInp), m_id);

        for(int i = 0; i < evgNumUnivInp && inp == 0; i++)
            inp = findSeqTrig(m_owner->getInput(i, UnivInp), m_id);

        for(int i = 0; i < evgNumRearInp && inp == 0; i++)
            inp = findSeqTrig(m_owner->getInput(i, RearInp), m_id);

        if(inp != 0)
            trigSrc = (SeqTrigSrc)
                      ((inp->getNum()*4 + (epicsUInt32)inp->getType()) + 40);
    }
    return trigSrc;
}

void
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
}

SeqRunMode
evgSeqRam::getRunMode() const {
    if(READ32(m_pReg, SeqControl(m_id)) & EVG_SEQ_RAM_SINGLE)
        return Single;
    if(READ32(m_pReg, SeqControl(m_id)) & EVG_SEQ_RAM_RECYCLE)
        return Auto;
    else
        return Normal;
}

void
evgSeqRam::enable() {
    if(isAllocated()) {
        BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_ARM);
    } else 
        throw std::runtime_error("Trying to enable Unallocated seqRam");
}

void
evgSeqRam::disable() {
    BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_DISABLE);
}

void
evgSeqRam::reset() {
    BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_RESET);	
}

void
evgSeqRam::alloc(evgSoftSeq* softSeq) {
    m_softSeq = softSeq;
    m_allocated = true;
}

void
evgSeqRam::dealloc() {
    m_softSeq = 0;
    m_allocated = false;

    //clear interrupt flags
    BITSET32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM(m_id));	
    BITSET32(m_pReg, IrqFlag, EVG_IRQ_START_RAM(m_id));
}

bool 
evgSeqRam::isEnabled() const {
    return READ32(m_pReg, SeqControl(m_id)) & EVG_SEQ_RAM_ENABLED; 
}

bool 
evgSeqRam::isRunning() const {
    return READ32(m_pReg, SeqControl(m_id)) & EVG_SEQ_RAM_RUNNING; 
}

bool
evgSeqRam::isAllocated() const {
    return m_allocated && m_softSeq;
}

evgSoftSeq* 
evgSeqRam::getSoftSeq() {
    return m_softSeq;
}
