#include "evgTrigEvt.h"
#include "evgMrm.h"

#include <iostream>
#include <stdexcept>

#include <errlog.h>

#include <mrfCommonIO.h>
#include <mrfCommon.h>

#include "evgRegMap.h"

evgTrigEvt::evgTrigEvt(const std::string& name, const epicsUInt32 id,
                       volatile epicsUInt8* const pReg, evgMrm* owner):
mrf::ObjectInst<evgTrigEvt>(name),
m_id(id),
m_pReg(pReg),
m_owner(owner) {
    scanIoInit(&changed);
}

evgTrigEvt::~evgTrigEvt() {
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

    if(evtCode!=0)
        evtCode |= TrigEventCtrl_Ena;

    WRITE32(m_pReg, TrigEventCtrl(m_id), evtCode);
}

void
evgTrigEvt::setSource(epicsUInt32 src) {
    if (!m_owner) {
        throw std::runtime_error("evgTrigEvt owner is null");
    }

    SCOPED_LOCK2(m_owner->m_lock, guard);

    // Clear MXC mappings
    for (epicsUInt32 i = 0; i < 8; i++) {
        evgMxc* mxc = m_owner->getMxc(i);
        if (mxc) {
            mxc->setTrigEvtMap(m_id, false);
        }
    }
    // Clear AC Trigger mapping
    evgAcTrig* acTrig = m_owner->getAcTrig();
    if (acTrig) {
        acTrig->setTrigEvtMap(m_id, false);
    }
    // Clear External Input mappings
    for (evgMrm::inputs_iterator it = m_owner->beginInputs(); it != m_owner->endInputs(); ++it) {
        evgInput* input = it->second;
        if (input) {
            input->setTrigEvtMap(m_id, false);
        }
    }

    // 2. Map the selected source
    if (src == 0x03000000) {
        // Off
    } else if (src < 8) {
        // MXC 0..7
        evgMxc* mxc = m_owner->getMxc(src);
        if (!mxc) {
            throw std::runtime_error("Selected MXC is not initialized");
        }
        mxc->setTrigEvtMap(m_id, true);
    } else if (src == 16) {
        // AC
        if (!acTrig) {
            throw std::runtime_error("AC Trigger is not initialized");
        }
        acTrig->setTrigEvtMap(m_id, true);
    } else if ((src & 0xff000000) == 0x02000000) {
        // External Input
        InputType type;
        switch ((src >> 16) & 0xff) {
            case 1: type = FrontInp; break;
            case 2: type = UnivInp;  break;
            case 3: type = RearInp;  break;
            case 4: type = BackInp;  break;
            default:
                throw std::runtime_error("Invalid external input type in TrigEvt source selection");
        }
        epicsUInt32 inputNum = src & 0xff;
        evgInput* input = m_owner->getInput(inputNum, type);
        if (!input) {
            throw std::runtime_error("Selected input is not initialized");
        }
        input->setTrigEvtMap(m_id, true);
    } else {
        throw std::runtime_error("Invalid TrigEvt source selection code");
    }

    // RB
    scanIoRequest(changed);
}

epicsUInt32
evgTrigEvt::getSource() const {
    if (!m_owner) {
        return 0x03000000; // Off
    }

    SCOPED_LOCK2(m_owner->m_lock, guard);

    // Check MXC
    for (epicsUInt32 i = 0; i < 8; i++) {
        evgMxc* mxc = m_owner->getMxc(i);
        if (mxc && mxc->getTrigEvtMap(m_id)) {
            return i;
        }
    }

    // 2. Check AC
    evgAcTrig* acTrig = m_owner->getAcTrig();
    if (acTrig && (acTrig->getTrigEvtMap() & (1 << m_id)) != 0) {
        return 16;
    }

    // 3. Check Inputs
    for (evgMrm::inputs_iterator it = m_owner->beginInputs(); it != m_owner->endInputs(); ++it) {
        evgInput* input = it->second;
        if (input && input->getTrigEvtMap(m_id)) {
            epicsUInt32 typeCode = 0;
            switch (input->getType()) {
                case FrontInp: typeCode = 1; break;
                case UnivInp:  typeCode = 2; break;
                case RearInp:  typeCode = 3; break;
                case BackInp:  typeCode = 4; break;
                default: break;
            }
            return 0x02000000 | (typeCode << 16) | input->getNum();
        }
    }

    return 0x03000000; // default to Off
}

