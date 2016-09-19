#include "evgEvtClk.h"

#include <stdio.h>
#include <errlog.h> 
#include <stdexcept>

#include <mrfCommonIO.h> 
#include <mrfCommon.h> 
#include <mrfFracSynth.h>

#include "evgRegMap.h"

evgEvtClk::evgEvtClk(const std::string& name, volatile epicsUInt8* const pReg):
mrf::ObjectInst<evgEvtClk>(name),
m_pReg(pReg),
m_RFref(0.0f),
m_fracSynFreq(0.0f) {
}

evgEvtClk::~evgEvtClk() {
}

epicsFloat64
evgEvtClk::getFrequency() const {
    if(getSource() == ClkSrcInternal)
        return m_fracSynFreq;
    else
        return getRFFreq()/getRFDiv();
}

void
evgEvtClk::setRFFreq (epicsFloat64 RFref) {
    if(RFref < 50.0f || RFref > 1600.0f) {
        char err[80];
        sprintf(err, "Cannot set RF frequency to %f MHz. Valid range is 50 - 1600.", RFref);
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }

    m_RFref = RFref;
}

epicsFloat64
evgEvtClk::getRFFreq() const {
    return m_RFref;    
}

void
evgEvtClk::setRFDiv(epicsUInt32 rfDiv) {
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
evgEvtClk::getRFDiv() const {
    // read 0 -> divide by 1
    return 1+((READ32(m_pReg, ClockControl)&ClockControl_Div_MASK)>>ClockControl_Div_SHIFT);
}

void
evgEvtClk::setFracSynFreq(epicsFloat64 freq) {
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
evgEvtClk::getFracSynFreq() const {
    return FracSynthAnalyze(READ32(m_pReg, FracSynthWord), 24.0, 0);
}

void
evgEvtClk::setSource (bool clkSrc) {
    if(clkSrc)
        BITSET32 (m_pReg, ClockControl, ClockControl_EXTRF);
    else 
        BITCLR32 (m_pReg, ClockControl, ClockControl_EXTRF);
}

bool
evgEvtClk::getSource() const {
    epicsUInt32 clkReg = READ32(m_pReg, ClockControl);
    return clkReg & ClockControl_EXTRF;
}

