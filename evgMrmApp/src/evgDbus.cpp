#include "evgDbus.h"
#include "evgMrm.h"

#include <iostream>
#include <stdexcept>

#include <mrfCommonIO.h>
#include <errlog.h>
#include <mrfCommon.h>

#include "evgRegMap.h"

evgDbus::evgDbus(const std::string& name, const epicsUInt32 id,
                 volatile epicsUInt8* const pReg, evgMrm* owner):
mrf::ObjectInst<evgDbus>(name),
m_id(id),
m_pReg(pReg),
m_owner(owner) {
    scanIoInit(&changed);
}

evgDbus::~evgDbus() {
}

void
evgDbus::setSource(epicsUInt32 src) {
    if (!m_owner) {
        throw std::runtime_error("evgDbus owner is null");
    }

    SCOPED_LOCK2(m_owner->m_lock, guard);

    // Clear all external input mappings for this DBUS bit.
    for (evgMrm::inputs_iterator it = m_owner->beginInputs(); it != m_owner->endInputs(); ++it) {
        evgInput* input = it->second;
        if (input) {
            input->setDbusMap(m_id, false);
        }
    }

    epicsUInt32 hwSrc = 0;

    // External input trigger
    if ((src & 0xff000000) == 0x02000000) {
        hwSrc = 1; // ExtInp

        InputType type;
        switch ((src >> 16) & 0xff) {
            case 1: type = FrontInp; break;
            case 2: type = UnivInp;  break;
            case 3: type = RearInp;  break;
            case 4: type = BackInp;  break;
            default:
                throw std::runtime_error("Invalid external input type in DBUS source selection");
        }
        epicsUInt32 inputNum = src & 0xff;
        evgInput* input = m_owner->getInput(inputNum, type);
        if (!input) {
            throw std::runtime_error("Selected input is not initialized");
        }
        input->setDbusMap(m_id, true);
    } else {
        // Internal source (Off=0, Mxc=2, Up EVG=3)
        if (src != 0 && src != 2 && src != 3) {
            throw std::runtime_error("Invalid DBUS source selection code");
        }
        hwSrc = src;
    }

    epicsUInt32 mask = hwSrc << (4 * m_id);
    epicsUInt32 dbusSrc = READ32(m_pReg, DBusSrc);
    dbusSrc = dbusSrc & ~(0xf << (4 * m_id));
    dbusSrc = dbusSrc | mask;
    WRITE32(m_pReg, DBusSrc, dbusSrc);

    // RB
    scanIoRequest(changed);
}

epicsUInt32
evgDbus::getSource() const {
    if (!m_owner) {
        return 0;
    }

    SCOPED_LOCK2(m_owner->m_lock, guard);

    epicsUInt32 dbusSrc = READ32(m_pReg, DBusSrc);
    epicsUInt32 hwSrc = (dbusSrc >> (4 * m_id)) & 0xf;

    if (hwSrc != 1) {
        return hwSrc;
    }

    for (evgMrm::inputs_iterator it = m_owner->beginInputs(); it != m_owner->endInputs(); ++it) {
        evgInput* input = it->second;
        if (input && input->getDbusMap(m_id)) {
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

    return 0; // fallback to Off
}


