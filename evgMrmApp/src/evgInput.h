#ifndef EVG_INPUT_H
#define EVG_INPUT_H

#include <iostream>
#include <string>
#include <map>

#include <epicsTypes.h>

enum InputType {
    None_Input = 0,
    FP_Input,
    Univ_Input,
    TB_Input
};

extern std::map<std::string, epicsUInt32> InpStrToEnum;

class evgInput {
public:
    evgInput(const epicsUInt32, const InputType, volatile epicsUInt8* const);
    ~evgInput();

    epicsUInt32 getNum();
    InputType getType();

    epicsStatus enaExtIrq(bool);
    epicsStatus setInpDbusMap(epicsUInt16, bool);
    epicsStatus setInpSeqTrigMap(epicsUInt32);
    epicsUInt32 getInpSeqTrigMap();
    epicsStatus setInpTrigEvtMap(epicsUInt16, bool);

private:
    const epicsUInt32          m_num;
    const InputType            m_type;
    volatile epicsUInt8* const m_pInMap;
};
#endif //EVG_INPUT_H
