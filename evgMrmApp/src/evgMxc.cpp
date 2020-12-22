/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#include "evgMxc.h"

#include <iostream>
#include <stdexcept>
#include <math.h>

#include <mrfCommonIO.h> 
#include <errlog.h>     
#include <mrfCommon.h>

#include "evgMrm.h"
#include "evgRegMap.h"

evgMxc::evgMxc(const std::string& name, const epicsUInt32 id,
               evgMrm* const owner)
    :mrf::ObjectInst<evgMxc>(name)
    ,m_id(id)
    ,m_owner(owner)
    ,m_pReg(owner->getRegAddr())
{
    OBJECT_INIT;
}

evgMxc::~evgMxc() {
}

bool 
evgMxc::getStatus() const {
    return (READ32(m_pReg, MuxControl(m_id)) & MuxControl_Sts) != 0;
}

void
evgMxc::setPolarity(bool polarity) {
    if(polarity)
        BITSET32(m_pReg, MuxControl(m_id), MuxControl_Pol);
    else
        BITCLR32(m_pReg, MuxControl(m_id), MuxControl_Pol);
}

bool
evgMxc::getPolarity() const {
    return (READ32(m_pReg, MuxControl(m_id)) & MuxControl_Pol) != 0;
}

void
evgMxc::setPrescaler(epicsUInt32 preScaler) {
    if(preScaler == 0 || preScaler == 1)
        throw std::runtime_error("Invalid preScaler value in Multiplexed Counter. Value should not be 0 or 1.");

    WRITE32(m_pReg, MuxPrescaler(m_id), preScaler);
}

epicsUInt32
evgMxc::getPrescaler() const {
    return READ32(m_pReg, MuxPrescaler(m_id));
}


void
evgMxc::setFrequency(epicsFloat64 freq) {
    epicsUInt32 clkSpeed = (epicsUInt32)(getFrequency() *
                            pow(10.0, 6));
    epicsUInt32 preScaler = (epicsUInt32)((epicsFloat64)clkSpeed / freq);
    
    setPrescaler(preScaler);
}

epicsFloat64 
evgMxc::getFrequency() const {
    epicsFloat64 clkSpeed = (epicsFloat64)m_owner->getFrequency()
                             * pow(10.0, 6);
    epicsFloat64 preScaler = (epicsFloat64)getPrescaler();
    return clkSpeed/preScaler;    
}

void
evgMxc::setTrigEvtMap(epicsUInt16 trigEvt, bool ena) {
    if(trigEvt > 7)
        throw std::runtime_error("EVG Mxc Trig Event ID too large. Max: 7");

    epicsUInt32    mask = 1 << (trigEvt+MuxControl_TrigMap_SHIFT);
    if(ena)
        BITSET32(m_pReg, MuxControl(m_id), mask);
    else
        BITCLR32(m_pReg, MuxControl(m_id), mask);
}

bool
evgMxc::getTrigEvtMap(epicsUInt16 trigEvt) const {
    if(trigEvt > 7)
        throw std::runtime_error("EVG Mxc Trig Event ID too large. Max: 7");

    epicsUInt32    mask = 1 << (trigEvt+MuxControl_TrigMap_SHIFT);
    return READ32(m_pReg, MuxControl(m_id))&mask;
}

