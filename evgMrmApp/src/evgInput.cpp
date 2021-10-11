#include "evgInput.h"

#include <stdexcept>

#include <errlog.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h>
 
#include "evgRegMap.h"
std::map<std::string, epicsUInt32> InpStrToEnum;

evgInput::evgInput(const std::string& name, const epicsUInt32 num,
                   const InputType type, volatile epicsUInt8* const pInReg)
    :mrf::ObjectInst<evgInput>(name)
    ,m_num(num)
    ,m_type(type)
    ,m_pInReg(pInReg)
{}

evgInput::~evgInput() {
}

epicsUInt32
evgInput::getNum() const {
    return m_num;
}

InputType
evgInput::getType() const {
    return m_type;
}

void
evgInput::setExtIrq(bool ena) {
    if(ena)
        nat_iowrite32(m_pInReg, nat_ioread32(m_pInReg) |
                                 (epicsUInt32)EVG_EXT_INP_IRQ_ENA);
    else
        nat_iowrite32(m_pInReg, nat_ioread32(m_pInReg) &
                                 (epicsUInt32)~(EVG_EXT_INP_IRQ_ENA));
}

bool
evgInput::getExtIrq() const {
    return  (nat_ioread32(m_pInReg) & (epicsUInt32)EVG_EXT_INP_IRQ_ENA) != 0;
}

void
evgInput::setDbusMap(epicsUInt16 dbus, bool ena) {
    if(dbus > 7)
        throw std::runtime_error("EVG DBUS num out of range. Max: 7");

    epicsUInt32    mask = 0x10000 << dbus;

    //Read-Modify-Write
    epicsUInt32 map = nat_ioread32(m_pInReg);

    if(ena)
        map = map | mask;
    else
        map = map & ~mask;

    nat_iowrite32(m_pInReg, map);
}

bool
evgInput::getDbusMap(epicsUInt16 dbus) const {
    if(dbus > 7)
        throw std::runtime_error("EVG DBUS num out of range. Max: 7");

    epicsUInt32 mask = 0x10000 << dbus;
    epicsUInt32 map = nat_ioread32(m_pInReg);
    return (map & mask) != 0;
}

void
evgInput::setSeqTrigMap(epicsUInt32 seqTrigMap) {
    if(seqTrigMap > 3)
        throw std::runtime_error("Seq Trig Map out of range. Max: 3");

    //Read-Modify-Write
    epicsUInt32 map = nat_ioread32(m_pInReg);

    map = map & (~InMap_SEQ_MASK);
    map = map | (seqTrigMap << InMap_SEQ_SHIFT);

    nat_iowrite32(m_pInReg, map);
}

epicsUInt32
evgInput::getSeqTrigMap() const {
    epicsUInt32 map = nat_ioread32(m_pInReg);
    map = map & InMap_SEQ_MASK;
    map = map >> InMap_SEQ_SHIFT;
    return map;
}

void
evgInput::setTrigEvtMap(epicsUInt16 trigEvt, bool ena) {
    if(trigEvt > 7)
        throw std::runtime_error("Trig Event num out of range. Max: 7");

    epicsUInt32    mask = 1 << trigEvt;
    //Read-Modify-Write
    epicsUInt32 map = nat_ioread32(m_pInReg);

    if(ena)
        map = map | mask;
    else
        map = map & ~mask;

    nat_iowrite32(m_pInReg, map);
}

bool
evgInput::getTrigEvtMap(epicsUInt16 trigEvt) const {
    if(trigEvt > 7)
        throw std::runtime_error("EVG Trig Event num out of range. Max: 7");

    epicsUInt32 mask = 0x1 << trigEvt;
    epicsUInt32 map = nat_ioread32(m_pInReg);
    return (map & mask) != 0;
}

