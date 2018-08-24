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
    
    // all frequencies in MHz

    epicsFloat64 getFrequency() const;

    void setRFFreq(epicsFloat64);
    epicsFloat64 getRFFreq() const;

    void setRFDiv(epicsUInt32);
    epicsUInt32 getRFDiv() const;

    void setFracSynFreq(epicsFloat64);
    epicsFloat64 getFracSynFreq() const;

    // see ClockCtrl[RFSEL]
    enum ClkSrc {
        ClkSrcInternal=0,
        ClkSrcRF=1,
        ClkSrcPXIe100=2,
        ClkSrcRecovered=4, // fanout mode
        ClkSrcSplit=5, // split, external downstream on downstream, recovered on upstream
        ClkSrcPXIe10=6,
        ClkSrcRecovered_2=7,
    };
    void setSource(epicsUInt16);
    epicsUInt16 getSource() const;

    bool pllLocked() const;

private:
    volatile epicsUInt8* const m_pReg;
    epicsFloat64               m_RFref;       // In MHz
    epicsFloat64               m_fracSynFreq; // In MHz
};

#endif //EVG_EVTCLK_H
