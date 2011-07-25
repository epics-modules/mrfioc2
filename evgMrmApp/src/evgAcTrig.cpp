#include "evgAcTrig.h"

#include <iostream>
#include <stdexcept>

#include <mrfCommonIO.h>    
#include <mrfCommon.h> 

#include "evgRegMap.h"

evgAcTrig::evgAcTrig(const std::string& name, volatile epicsUInt8* const pReg):
mrf::ObjectInst<evgAcTrig>(name),
m_pReg(pReg) {
}

evgAcTrig::~evgAcTrig() {
}

void
evgAcTrig::setDivider(epicsUInt32 divider) {
    if(divider > 255)
        throw std::runtime_error("EVG AC Trigger divider out of range.");

    WRITE8(m_pReg, AcTrigDivider, divider);
}

epicsUInt32
evgAcTrig::getDivider() const {
    return READ8(m_pReg, AcTrigDivider);
}

void
evgAcTrig::setPhase(epicsFloat64 phase) {
    if(phase < 0 || phase > 25.5)
        throw std::runtime_error("EVG AC Trigger phase out of range.");

    WRITE8(m_pReg, AcTrigPhase, phase);
}

epicsFloat64
evgAcTrig::getPhase() const {
    return READ8(m_pReg, AcTrigPhase);
}

void
evgAcTrig::setBypass(bool byp) {
    if(byp)
        BITSET8(m_pReg, AcTrigControl, EVG_AC_TRIG_BYP);
    else
        BITCLR8(m_pReg, AcTrigControl, EVG_AC_TRIG_BYP);
}

bool
evgAcTrig::getBypass() const {
    return READ8(m_pReg, AcTrigControl) & EVG_AC_TRIG_BYP;
}


void
evgAcTrig::setSyncSrc(bool syncSrc) {
    if(syncSrc)
        BITSET8(m_pReg, AcTrigControl, EVG_AC_TRIG_SYNC);
    else
        BITCLR8(m_pReg, AcTrigControl, EVG_AC_TRIG_SYNC);
}

bool
evgAcTrig::getSyncSrc() const {
    return READ8(m_pReg, AcTrigControl) & EVG_AC_TRIG_SYNC;
}

void
evgAcTrig::setTrigEvtMap(epicsUInt16 trigEvt, bool ena) {
    if(trigEvt > 7)
        throw std::runtime_error("EVG Trig Event ID too large.");

    epicsUInt8    mask = 1 << trigEvt;
    //Read-Modify-Write
    epicsUInt8 map = READ8(m_pReg, AcTrigEvtMap);

    if(ena)
        map = map | mask;
    else
        map = map & ~mask;

    WRITE8(m_pReg, AcTrigEvtMap, map);
}

epicsUInt32
evgAcTrig::getTrigEvtMap() const {
    return READ8(m_pReg, AcTrigEvtMap);
}
