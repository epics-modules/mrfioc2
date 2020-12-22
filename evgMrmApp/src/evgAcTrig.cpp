/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#include "evgAcTrig.h"

#include <iostream>
#include <stdexcept>

#include <mrfCommonIO.h>    
#include <mrfCommon.h> 

#include "evgRegMap.h"

evgAcTrig::evgAcTrig(const std::string& name, volatile epicsUInt8* const pReg)
    :mrf::ObjectInst<evgAcTrig>(name)
    ,m_pReg(pReg)
{
    OBJECT_INIT;
}

evgAcTrig::~evgAcTrig() {
}

void
evgAcTrig::setDivider(epicsUInt32 divider) {
    if(divider > 255)
        throw std::runtime_error("EVG AC Trigger divider out of range. Range: 0 - 255"); // 0: divide by 1, 1: divide by 2, ... 255: divide by 256

    epicsUInt32 temp = READ32(m_pReg, AcTrigControl)&~AcTrigControl_Divider_MASK;
    WRITE32(m_pReg, AcTrigControl, temp|(divider<<AcTrigControl_Divider_SHIFT));
}

epicsUInt32
evgAcTrig::getDivider() const {
    return (READ32(m_pReg, AcTrigControl)&AcTrigControl_Divider_MASK)>>AcTrigControl_Divider_SHIFT;
}

void
evgAcTrig::setPhase(epicsFloat64 phase) {
    if(phase < 0 || phase > 25.5)
        throw std::runtime_error("EVG AC Trigger phase out of range. Delay range 0 ms - 25.5 ms in 0.1 ms steps");
    epicsUInt32 iphase = phase;

    epicsUInt32 temp = READ32(m_pReg, AcTrigControl)&~AcTrigControl_Phase_MASK;
    WRITE32(m_pReg, AcTrigControl, temp|(iphase<<AcTrigControl_Phase_SHIFT));
}

epicsFloat64
evgAcTrig::getPhase() const {
    return (READ32(m_pReg, AcTrigControl)&AcTrigControl_Phase_MASK)>>AcTrigControl_Phase_SHIFT;
}

void
evgAcTrig::setBypass(bool byp) {
    if(byp)
        BITSET32(m_pReg, AcTrigControl, AcTrigControl_Bypass);
    else
        BITCLR32(m_pReg, AcTrigControl, AcTrigControl_Bypass);
}

bool
evgAcTrig::getBypass() const {
    return !!(READ32(m_pReg, AcTrigControl)&AcTrigControl_Bypass);
}


void
evgAcTrig::setSyncSrc(bool syncSrc) {
    if(syncSrc)
        BITSET32(m_pReg, AcTrigControl, AcTrigControl_Sync);
    else
        BITCLR32(m_pReg, AcTrigControl, AcTrigControl_Sync);
}

bool
evgAcTrig::getSyncSrc() const {
    return !!(READ32(m_pReg, AcTrigControl)&AcTrigControl_Sync);
}

void
evgAcTrig::setTrigEvtMap(epicsUInt16 trigEvt, bool ena) {
    if(trigEvt > 7)
        throw std::runtime_error("EVG Trig Event ID too large. Max : 7");

    epicsUInt32    mask = 1 << (trigEvt+AcTrigMap_EvtSHIFT);

    if(ena)
        BITSET32(m_pReg, AcTrigMap, mask);
    else
        BITCLR32(m_pReg, AcTrigMap, mask);
}

epicsUInt32
evgAcTrig::getTrigEvtMap() const {
    return READ32(m_pReg, AcTrigMap)>>AcTrigMap_EvtSHIFT;
}
