#include "evgSoftEvt.h"

#include <errlog.h> 
#include <stdexcept>

#include <mrfCommonIO.h> 
#include <mrfCommon.h> 

#include "evgRegMap.h"

evgSoftEvt::evgSoftEvt(const std::string& name, volatile epicsUInt8* const pReg):
mrf::ObjectInst<evgSoftEvt>(name),
m_pReg(pReg),
m_lock() {
}

void
evgSoftEvt::setEvtCode(epicsUInt32 evtCode) {
    if(evtCode > 255)
        throw std::runtime_error("Event Code out of range. Valid range: 0 - 255.");
    
    SCOPED_LOCK(m_lock);

    while(READ32(m_pReg, SwEvent) & SwEvent_Pend) {}

    WRITE32(m_pReg, SwEvent,
            (evtCode<<SwEvent_Code_SHIFT)
            |SwEvent_Ena);
}

