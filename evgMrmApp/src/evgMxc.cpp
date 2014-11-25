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
               evgMrm* const owner):
mrf::ObjectInst<evgMxc>(name),
m_id(id),
m_owner(owner),
m_pReg(owner->getRegAddr()) {    
}

evgMxc::~evgMxc() {
}

bool 
evgMxc::getStatus() const {
    return READ32(m_pReg, MuxControl(m_id)) & EVG_MUX_STATUS;
}

void
evgMxc::setPolarity(bool polarity) {
    if(polarity)
        BITSET32(m_pReg, MuxControl(m_id), EVG_MUX_POLARITY);
    else
        BITCLR32(m_pReg, MuxControl(m_id), EVG_MUX_POLARITY);
}

bool
evgMxc::getPolarity() const {
    return READ32(m_pReg, MuxControl(m_id)) & EVG_MUX_POLARITY;
}

void
evgMxc::setPrescaler(epicsUInt32 preScaler) {
    if(preScaler == 0 || preScaler == 1)
        throw std::runtime_error("Invalid preScaler value in Multiplexed Counter");

    WRITE32(m_pReg, MuxPrescaler(m_id), preScaler);
}

epicsUInt32
evgMxc::getPrescaler() const {
    return READ32(m_pReg, MuxPrescaler(m_id));
}


void
evgMxc::setFrequency(epicsFloat64 freq) {
    epicsUInt32 clkSpeed = (epicsUInt32)(m_owner->getEvtClk()->getFrequency() *
                            pow(10, 6));
    epicsUInt32 preScaler = (epicsUInt32)((epicsFloat64)clkSpeed / freq);
    
    setPrescaler(preScaler);
}

epicsFloat64 
evgMxc::getFrequency() const {
    epicsFloat64 clkSpeed = (epicsFloat64)m_owner->getEvtClk()->getFrequency()
                             * pow(10, 6);
    epicsFloat64 preScaler = (epicsFloat64)getPrescaler();
    return clkSpeed/preScaler;    
}

void
evgMxc::setTrigEvtMap(epicsUInt16 trigEvt, bool ena) {
    if(trigEvt > 7)
        throw std::runtime_error("EVG Mxc Trig Event ID too large.");

    epicsUInt8    mask = 1 << trigEvt;
    //Read-Modify-Write
    epicsUInt8 map = READ8(m_pReg, MuxTrigMap(m_id));

    if(ena)
        map = map | mask;
    else
        map = map & ~mask;

    WRITE8(m_pReg, MuxTrigMap(m_id), map);
}

bool
evgMxc::getTrigEvtMap(epicsUInt16 trigEvt) const {
    if(trigEvt > 7)
        throw std::runtime_error("EVG Mxc Trig Event ID too large.");

    epicsUInt8 mask = 1 << trigEvt;
    epicsUInt8 map = READ8(m_pReg, MuxTrigMap(m_id));
    return mask & map;
}

