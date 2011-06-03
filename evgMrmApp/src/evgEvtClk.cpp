#include "evgEvtClk.h"

#include <stdio.h>
#include <errlog.h> 
#include <stdexcept>

#include <mrfCommonIO.h> 
#include <mrfCommon.h> 
#include <mrfFracSynth.h>

#include "evgRegMap.h"

evgEvtClk::evgEvtClk(volatile epicsUInt8* const pReg):
m_pReg(pReg),
m_RFref(0.0f),
m_fracSynFreq(0.0f) {
}

/**        RF Input Frequency    **/
epicsStatus
evgEvtClk::setRFref (epicsFloat64 RFref) {    
    if(RFref < 50.0f || RFref > 1600.0f) {
        char err[80];
        sprintf(err, "Cannot set RF frequency to %f MHz.", RFref);
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }

    m_RFref = RFref;
    return OK;
}

epicsFloat64
evgEvtClk::getRFref() {
    return m_RFref;    
}

epicsStatus 
evgEvtClk::setRFdiv(epicsUInt32 rfDiv) {
    if(rfDiv < 1    || rfDiv > 32) {
        char err[80];
        sprintf(err, "Invalid RF Divider %d.", rfDiv);
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }
    
    WRITE8(m_pReg, RfDiv, rfDiv-1);
    return OK;
}

epicsUInt32
evgEvtClk::getRFdiv() {
    return READ8(m_pReg, RfDiv) + 1;
}

/**    Fractional Synthesizer Frequency    **/
epicsStatus
evgEvtClk::setFracSynFreq(epicsFloat64 freq) {
    epicsUInt32        controlWord, oldControlWord;
    epicsFloat64     error;

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
        epicsUInt16 uSecDivider = (epicsUInt16)freq;
        WRITE16(m_pReg, uSecDiv, uSecDivider);
    }

    m_fracSynFreq = FracSynthAnalyze(READ32(m_pReg, FracSynthWord), 24.0, 0);
    return OK;
}

epicsFloat64
evgEvtClk::getEvtClkSpeed() {
    if(getEvtClkSrc() == ClkSrcInternal)
        return m_fracSynFreq;
    else
        return getRFref()/getRFdiv();
}

epicsStatus
evgEvtClk::setEvtClkSrc (bool clkSrc) {
    if(clkSrc)
        BITSET8 (m_pReg, ClockSource, EVG_CLK_SRC_EXTRF);
    else 
        BITCLR8 (m_pReg, ClockSource, EVG_CLK_SRC_EXTRF);

    return OK;
}

bool
evgEvtClk::getEvtClkSrc() {
    epicsUInt8 clkReg = READ8(m_pReg, ClockSource);
    return clkReg & EVG_CLK_SRC_EXTRF;
}

