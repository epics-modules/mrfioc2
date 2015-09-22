#include "evgTrigEvt.h"

#include <iostream>
#include <stdexcept>

#include <errlog.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h>

#include "evgRegMap.h"

evgTrigEvt::evgTrigEvt(const std::string& name, const epicsUInt32 id,
                       volatile epicsUInt8* const pReg):
mrf::ObjectInst<evgTrigEvt>(name),
m_id(id),
m_pReg(pReg) {
}

evgTrigEvt::~evgTrigEvt() {
} 

void
evgTrigEvt::enable(bool ena) {
    if(ena)
        BITSET32(m_pReg, TrigEventCtrl(m_id), EVG_TRIG_EVT_ENA);
    else
        BITCLR32(m_pReg, TrigEventCtrl(m_id), EVG_TRIG_EVT_ENA);
}

bool
evgTrigEvt::enabled() const {
    return (READ32(m_pReg, TrigEventCtrl(m_id)) & EVG_TRIG_EVT_ENA) != 0;
}

epicsUInt32
evgTrigEvt::getEvtCode() const {
    epicsUInt32 temp = READ32(m_pReg, TrigEventCtrl(m_id));
    temp &= TrigEventCtrl_Code_MASK;
    return temp>>TrigEventCtrl_Code_SHIFT;
}

void
evgTrigEvt::setEvtCode(epicsUInt32 evtCode) {
    if(evtCode > 255)
        throw std::runtime_error("Event Code out of range. Valid range: 0 - 255");

    epicsUInt32 temp = READ32(m_pReg, TrigEventCtrl(m_id));
    temp &= ~TrigEventCtrl_Code_MASK;
    temp |= evtCode<<TrigEventCtrl_Code_SHIFT;
    WRITE32(m_pReg, TrigEventCtrl(m_id), evtCode);
}
