#ifndef EVG_OUTPUT_H
#define EVG_OUTPUT_H

#include <epicsTypes.h>
#include "mrf/object.h"

enum OutputType {
    NoneOut = 0,
    FrontOut,
    UnivOut
};

class evgOutput : public mrf::ObjectInst<evgOutput> {
public:
    evgOutput(const std::string&, const epicsUInt32, const OutputType,
              volatile epicsUInt8* const);
    ~evgOutput();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};
    
    void setSource(epicsUInt16);
    epicsUInt16 getSource() const;

private:
    const epicsUInt32          m_num;
    const OutputType           m_type;
    volatile epicsUInt8* const m_pOutReg;
};

#endif //EVG_OUTPUT_H
