
#include <stdio.h>
#include <errlog.h> 
#include <stdexcept>
#include <sstream>

#include <mrfCommonIO.h> 
#include <mrfCommon.h> 
#include <mrfFracSynth.h>

#include "evgMrm.h"
#include "evgRegMap.h"

epicsFloat64
evgMrm::getFrequency() const {
    epicsUInt16 cur = getSource();
    switch(cur) {
    case ClkSrcPXIe100:
        return 100;
        // case ClkSrcPXIe10: ??
    case ClkSrcRF:
    case ClkSrcSplit:
        return getRFFreq()/getRFDiv();
    default:
        return m_fracSynFreq;
    }
}

void
evgMrm::setRFFreq (epicsFloat64 RFref) {
    if(RFref < 50.0f || RFref > 1600.0f) {
        std::ostringstream strm;
        strm<<"Cannot set RF frequency to "<<RFref<<" MHz. Valid range is 50 - 1600.";
        throw std::runtime_error(strm.str());
    }

    m_RFref = RFref;
}

epicsFloat64
evgMrm::getRFFreq() const {
    return m_RFref;    
}

void
evgMrm::setRFDiv(epicsUInt32 rfDiv) {
    if(rfDiv < 1 || rfDiv == 13 || rfDiv > 32) {
        char err[80];
        sprintf(err, "Invalid RF Divider %d. Valid range is 1 - 12, 14 - 32", rfDiv);
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }
    m_RFDiv = rfDiv;

    recalcRFDiv();
}

epicsUInt32
evgMrm::getRFDiv() const {
    return m_RFDiv;
}

void
evgMrm::setFracSynFreq(epicsFloat64 freq) {
    epicsFloat64 error;

    epicsUInt32 controlWord = FracSynthControlWord (freq, MRF_FRAC_SYNTH_REF, 0, &error);
    if ((!controlWord) || (error > 100.0)) {
        char err[80];
        sprintf(err, "Cannot set event clock speed to %f MHz.\n", freq);            
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }
    epicsUInt32 uSecDivider = (epicsUInt16)freq;

    epicsUInt32 oldControlWord=READ32(m_pReg, FracSynthWord),
                olduSecDivider=READ32(m_pReg, uSecDiv);

    /* Changing the control word disturbes the phase of the synthesiser
     which will cause a glitch. Don't change the control word unless needed.*/
    if(controlWord != oldControlWord || uSecDivider!=olduSecDivider){
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
    switch(clkSrc) {
    case ClkSrcInternal:
    case ClkSrcRF:
    case ClkSrcPXIe100:
    case ClkSrcRecovered:
    case ClkSrcSplit:
    case ClkSrcPXIe10:
    case ClkSrcRecovered_2:
        m_ClkSrc = (ClkSrc)clkSrc;

        recalcRFDiv();
        return;
    }

    throw std::invalid_argument("Invalid clock source");
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

void evgMrm::setPLLBandwidth(epicsUInt16 pllBandwidth)
{
    if(pllBandwidth > PLLBandwidth_MAX) {
        throw std::out_of_range("PLL bandwidth you selected is not available.");
    }

    epicsUInt32 temp = READ32(m_pReg, ClockControl);
    temp &= ~ClockControl_pllbw;
    temp |= (epicsUInt8)pllBandwidth << ClockControl_pllbw_SHIFT;
    WRITE32(m_pReg, ClockControl, temp);
}

epicsUInt16 evgMrm::getPLLBandwidth() const
{
    epicsUInt32 bw = READ32(m_pReg, ClockControl);
    bw &= ClockControl_pllbw;
    bw = bw >> ClockControl_pllbw_SHIFT;

    return (epicsUInt16) bw;
}

void evgMrm::recalcRFDiv()
{
    epicsUInt32 cur = READ32(m_pReg, ClockControl);

    cur &= ~ClockControl_Sel_MASK;
    cur &= ~ClockControl_Div_MASK;

    epicsUInt32 div = m_RFDiv-1; // /1 -> 0, /2 -> 1, etc.

    switch(m_ClkSrc) {
    case ClkSrcInternal:
    case ClkSrcRecovered:
    case ClkSrcRecovered_2:
        // disable RF input
        div = 0xc; // /13 is magic for OFF
        break;
    default:
        break;
    }

    cur |= (div<<ClockControl_Div_SHIFT)&ClockControl_Div_MASK;
    cur |= (epicsUInt32(m_ClkSrc)<<ClockControl_Sel_SHIFT)&ClockControl_Sel_MASK;
    WRITE32(m_pReg, ClockControl, cur);
}
