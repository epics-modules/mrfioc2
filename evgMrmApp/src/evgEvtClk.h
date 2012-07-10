#ifndef EVG_EVTCLK_H
#define EVG_EVTCLK_H

#include <epicsTypes.h>
#include "mrf/object.h"

const epicsUInt16 ClkSrcInternal = 0; // Event clock is generated internally
const epicsUInt16 ClkSrcRF = 1;  // Event clock is derived from the RF input

class evgEvtClk : public mrf::ObjectInst<evgEvtClk> {
public:
    evgEvtClk(const std::string&, volatile epicsUInt8* const);
    ~evgEvtClk();

    /* locking done internally */
    virtual void lock() const{};
    virtual void unlock() const{};
    
    epicsFloat64 getFrequency() const;

    void setRFFreq(epicsFloat64);
    epicsFloat64 getRFFreq() const;

    void setRFDiv(epicsUInt32);
    epicsUInt32 getRFDiv() const;

    void setFracSynFreq(epicsFloat64);
    epicsFloat64 getFracSynFreq() const;

    void setSource(bool);
    bool getSource() const;

private:
    volatile epicsUInt8* const m_pReg;
    epicsFloat64               m_RFref;       // In MHz
    epicsFloat64               m_fracSynFreq; // In MHz
};

#endif //EVG_EVTCLK_H
