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
    evgOutput(const epicsUInt32, volatile epicsUInt8* const, const OutputType);
    ~evgOutput();
    
    epicsStatus setOutMap(epicsUInt16);

private:
    const epicsUInt32          m_id;
    volatile epicsUInt8* const m_pReg;
    const OutputType           m_type;
    
};

#endif //EVG_OUTPUT_H
