#ifndef EVG_INPUT_H
#define EVG_INPUT_H

#include <iostream>
#include <string>
#include <map>

#include <epicsTypes.h>
#include "mrf/object.h"

enum InputType {
    NoneInp = 0,
    FrontInp,
    UnivInp,
    RearInp
};

class evgInput : public mrf::ObjectInst<evgInput> {
    OBJECT_DECL(evgInput);
public:
    evgInput(const std::string&, const epicsUInt32, const InputType,
             volatile epicsUInt8* const);
    ~evgInput();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};

    epicsUInt32 getNum() const;
    InputType getType() const;

    void setExtIrq(bool);
    bool getExtIrq() const;

    void setDbusMap(epicsUInt16, bool);
    bool getDbusMap(epicsUInt16) const;

    void setSeqTrigMap(epicsUInt32);
    epicsUInt32 getSeqTrigMap() const;

    void setTrigEvtMap(epicsUInt16, bool);
    bool getTrigEvtMap(epicsUInt16) const;

private:
    const epicsUInt32          m_num;
    const InputType            m_type;
    volatile epicsUInt8* const m_pInReg;
};
#endif //EVG_INPUT_H
