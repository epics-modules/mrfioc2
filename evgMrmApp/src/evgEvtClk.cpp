#include "evgMrm.h"

#include <stdio.h>
#include <errlog.h> 
#include <stdexcept>

#include <mrfCommonIO.h> 
#include <mrfCommon.h> 
#include <mrfFracSynth.h>

#include "evgRegMap.h"

epicsFloat64
evgMrm::getFrequency() const {
    if(getSource() == ClkSrcInternal)
        return m_fracSynFreq;
    else
        return getRFFreq()/getRFDiv();
}

void
evgMrm::setRFFreq (epicsFloat64 RFref) {
    if(RFref < 50.0f || RFref > 1600.0f) {
        char err[80];
        sprintf(err, "Cannot set RF frequency to %f MHz. Valid range is 50 - 1600.", RFref);
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }

    m_RFref = RFref;
}

epicsFloat64
evgMrm::getRFFreq() const {
    return m_RFref;    
}

void
evgMrm::setRFDiv(epicsUInt32 rfDiv) {
    if(rfDiv < 1    || rfDiv > 32) {
        char err[80];
        sprintf(err, "Invalid RF Divider %d. Valid range is 1 - 32", rfDiv);
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }
    
    epicsUInt32 temp = READ32(m_pReg, ClockControl);
    temp &= ~ClockControl_Div_MASK;
    temp |= (rfDiv-1)<<ClockControl_Div_SHIFT;

    WRITE32(m_pReg, ClockControl, temp);
}

epicsUInt32
evgMrm::getRFDiv() const {
    // read 0 -> divide by 1
    return 1+((READ32(m_pReg, ClockControl)&ClockControl_Div_MASK)>>ClockControl_Div_SHIFT);
}

void
evgMrm::setFracSynFreq(epicsFloat64 freq) {
    epicsUInt32 controlWord, oldControlWord;
    epicsFloat64 error;

    controlWord = FracSynthControlWord (freq, MRF_FRAC_SYNTH_REF, 0, &error);
    if ((!controlWord) || (error > 100.0)) {
        char err[80];
        sprintf(err, "Cannot set event clock speed to %f MHz.\n", freq);            
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }

    oldControlWord=READ32(m_pReg, FracSynthWord);

    /* Changing the control word disturbes the phase of the synthesiser
     which will cause a glitch. Don't change the control word unless needed.*/
    if(controlWord != oldControlWord){
        WRITE32(m_pReg, FracSynthWord, controlWord);
        epicsUInt32 uSecDivider = (epicsUInt16)freq;
        WRITE32(m_pReg, uSecDiv, uSecDivider);
    }

    m_fracSynFreq = FracSynthAnalyze(READ32(m_pReg, FracSynthWord), 24.0, 0);
}

epicsFloat64
evgMrm::getFracSynFreq() const {
    return FracSynthAnalyze(READ32(m_pReg, FracSynthWord), 24.0, 0);
}

void
evgMrm::setSource (epicsUInt16 clkSrc) {
    epicsUInt32 cur = READ32(m_pReg, ClockControl);
    cur &= ~ClockControl_Sel_MASK;
    cur |= (epicsUInt32(clkSrc)<<ClockControl_Sel_SHIFT)&ClockControl_Sel_MASK;
    WRITE32(m_pReg, ClockControl, cur);
}

epicsUInt16 evgMrm::getSource() const {
    epicsUInt32 cur = READ32(m_pReg, ClockControl);
    cur &= ClockControl_Sel_MASK;
    return cur >> ClockControl_Sel_SHIFT;
}

bool evgMrm::pllLocked() const
{
    epicsUInt32 cur = READ32(m_pReg, ClockControl);
    epicsUInt32 mask = 0;
    if(version()>=MRFVersion(2, 7, 0))
        mask |= ClockControl_plllock|ClockControl_cglock;
    return (cur&mask)==mask;
}
