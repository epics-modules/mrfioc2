#ifndef EVG_OUTPUT_H
#define EVG_OUTPUT_H

#include <epicsTypes.h>

enum OutputType {
    None_Output = 0,
    FP_Output,
    Univ_Output
};

class evgOutput {
public:
    evgOutput(const epicsUInt32, const OutputType, volatile epicsUInt8* const);
    ~evgOutput();
    
    epicsStatus setOutMap(epicsUInt16);

private:
    const epicsUInt32          m_num;
    const OutputType           m_type;
    volatile epicsUInt8* const m_pReg;
};

#endif //EVG_OUTPUT_H
