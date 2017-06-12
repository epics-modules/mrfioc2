/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#include "evgSeqRam.h"

#include <iostream>
#include <stdexcept>
#include <stdlib.h>

#include <errlog.h>
#include <epicsInterrupt.h>

#include <mrfCommonIO.h>
#include <mrfCommon.h>
#include <epicsGuard.h>

#include "evgMrm.h"
#include "evgRegMap.h"

evgSeqRam::evgSeqRam(const epicsUInt32 id, evgMrm* const owner):
m_id(id),
m_owner(owner),
m_pReg(owner->getRegAddr()),
m_softSeq(0) {
}

void
evgSeqRam::setEventCode(const std::vector<epicsUInt8>& eventCode) {
    for(unsigned int i = 0; i < eventCode.size(); i++)
        WRITE32(m_pReg, SeqRamEvent(m_id,i), (epicsUInt32)eventCode[i]);
}

std::vector<epicsUInt8>
evgSeqRam::getEventCode() {
     std::vector<epicsUInt8> eventCode(2048, 0);

    for(unsigned int i = 0; i < eventCode.size(); i++)
        eventCode[i] = READ32(m_pReg, SeqRamEvent(m_id,i));

    return eventCode;
}

void
evgSeqRam::setTimestamp(const std::vector<epicsUInt64>& timestamp){
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
    interruptLock L;
    BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_SW_TRIG);
}

void
evgSeqRam::disableSeqExtTrig(evgInput* inp) {
    epicsUInt32 map = inp->getSeqTrigMap();
    map = map & ~(m_id+1);
    inp->setSeqTrigMap(map);
}

void
evgSeqRam::setTrigSrc(epicsUInt32 trigSrc) {
    // special handling of trigger sources which are different
    // between EVG and EVR sequencer implementations
    if(trigSrc == SEQ_SRC_DISABLE) {
        trigSrc = 31; // trigger disabled
    } else if(trigSrc == SEQ_SRC_SW || trigSrc==19){
        if(!m_id)
            trigSrc = 17; // use SEQ0 soft trigger
        else
            trigSrc = 18; // use SEQ1 soft trigger
    }

    /*Here we allow only one external input at a time to act as a trigger source
     *for the Sequencer. In theory mutliple external input can act as a trigger
     *source simultaneously
     */
    if(trigSrc > 40) {
       /*
        *When we set the trigger source to a new external input we should disable
        *the previous external input trigger. Since we dont keep track of the
        *previous trigger source we disable the trigger from all the
        *external inputs.
        */
        for(int i = 0; i < evgNumFrontInp; i++)
            disableSeqExtTrig(m_owner->getInput(i, FrontInp));

        for(int i = 0; i < evgNumUnivInp; i++)
            disableSeqExtTrig(m_owner->getInput(i, UnivInp));

        for(int i = 0; i < evgNumRearInp; i++)
            disableSeqExtTrig(m_owner->getInput(i, RearInp));
       /*
        *Now enable the triggering only on the appropraite external input of EVG.
        *Each external input is identified by its number and its type. The
        *SeqTrigSrc value for each input is chosen in such a way that when you
        *divide it by 4 the quotient will give you the input number and
        *remainder will give you the input type.
        */
        div_t divResult = div((epicsUInt32)trigSrc-40, 4);
        evgInput* inp = m_owner->getInput(divResult.quot, (InputType)divResult.rem);

        epicsUInt32 map = inp->getSeqTrigMap();
        map = map | (m_id+1);
        inp->setSeqTrigMap(map);

        if(!m_id)
            trigSrc = 24; // use mapped external trigger for SEQ0
        else
            trigSrc = 25; // use mapped external trigger for SEQ1
    }

    interruptLock L;
    epicsUInt32 temp = READ32(m_pReg, SeqControl(m_id));
    temp &= ~SeqControl_TrigSrc_MASK;
    temp |= (trigSrc&SeqControl_TrigSrc_MASK)<<SeqControl_TrigSrc_SHIFT;
    WRITE32(m_pReg, SeqControl(m_id), temp);
}

evgInput*
evgSeqRam::findSeqExtTrig(evgInput* inp) const {
    epicsUInt32 map = inp->getSeqTrigMap();
    if(map & (m_id+1))
        return inp;
    else
        return 0;
}

void
evgSeqRam::setRunMode(SeqRunMode mode) {
    interruptLock L;
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
    epicsUInt32 ctrl = READ32(m_pReg, SeqControl(m_id));
    if(ctrl & EVG_SEQ_RAM_SINGLE)
        return Single;
    if(ctrl & EVG_SEQ_RAM_RECYCLE)
        return Auto;
    else
        return Normal;
}

void
evgSeqRam::enable() {
    if(isAllocated()) {
        interruptLock L;
        BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_ARM);
    } else 
        throw std::runtime_error("Trying to enable Unallocated seqRam");
}

void
evgSeqRam::disable() {
    interruptLock L;
    BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_DISABLE);
}

void
evgSeqRam::reset() {
    interruptLock L;
    BITSET32(m_pReg, SeqControl(m_id), EVG_SEQ_RAM_RESET);
}

bool
evgSeqRam::alloc(evgSoftSeq* softSeq) {
    assert(softSeq);
    interruptLock ig;
    if(!isAllocated()) {
        softSeq->setSeqRam(this);
        m_softSeq = softSeq;
        return true;
    } else {
        return false;
    }
}

void
evgSeqRam::dealloc() {
    m_softSeq = 0;

    //clear interrupt flags
    BITSET32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM(m_id));	
    BITSET32(m_pReg, IrqFlag, EVG_IRQ_START_RAM(m_id));
}

bool 
evgSeqRam::isEnabled() const {
    return (READ32(m_pReg, SeqControl(m_id)) & EVG_SEQ_RAM_ENABLED) != 0;
}

bool 
evgSeqRam::isRunning() const {
    return (READ32(m_pReg, SeqControl(m_id)) & EVG_SEQ_RAM_RUNNING) != 0;
}

bool
evgSeqRam::isAllocated() const {
    return m_softSeq != NULL;
}

void
evgSeqRam::process_sos() {
    evgSoftSeq* softSeq = getSoftSeq();
    if(!softSeq)
        return;
    epicsGuard<epicsMutex> g(softSeq->m_lock);
    if(softSeq->getSeqRam()!=this)
        return;

    softSeq->process_sos();
}

void
evgSeqRam::process_eos() {
    evgSoftSeq* softSeq = getSoftSeq();
    if(!softSeq)
        return;
    epicsGuard<epicsMutex> g(softSeq->m_lock);
    if(softSeq->getSeqRam()!=this)
        return;

    softSeq->process_eos();
}

evgSoftSeq*
evgSeqRam::getSoftSeq() {
    interruptLock ig;
    return m_softSeq;
}
