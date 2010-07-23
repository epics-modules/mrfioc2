#include "evgEvtClk.h"

#include <errlog.h> 

#include <mrfCommonIO.h> 
#include <mrfCommon.h> 
#include <mrfFracSynth.h>

#include "evgRegMap.h"

evgEvtClk::evgEvtClk(volatile epicsUInt8* const pReg):
m_pReg(pReg),
m_RFref(0.0f),
m_clkSpeed(0.0f),
m_clkSrc(0) {
}

/**		RF Input Frequency	in MHz **/
epicsStatus
evgEvtClk::setRFref (epicsFloat64 RFref) {	
	if(RFref < 50.0f || RFref > 1600.0f) {
		errlogPrintf ("Cannot set RF frequency to %f MHz.\n", RFref);
        return ERROR;
	}

	printf("RF: %f MHz.\n", RFref);
	m_RFref = RFref;
	return OK;
}

epicsFloat64
evgEvtClk::getRFref() {
	return m_RFref;	
}

/**		Event Clock Speed	**/
epicsStatus
evgEvtClk::setClkSpeed (epicsFloat64 clkSpeed) {
	epicsStatus   status = OK;
	if(m_clkSrc == evgClkSrcInternal) {
		// Use internal fractional synthesizer to generate the clock
		epicsUInt32    controlWord, oldControlWord;
    	epicsFloat64   error;

    	controlWord = FracSynthControlWord (clkSpeed, MRF_FRAC_SYNTH_REF, 0, &error);
    	if ((!controlWord) || (error > 100.0)) {
        	errlogPrintf ("Cannot set event clock speed to %f MHz.\n", clkSpeed);
        	return ERROR;
    	}
	
		oldControlWord=READ32(m_pReg, FracSynthWord);

		/* Changing the control word disturbes the phase of the synthesiser
 		which will cause a glitch. Don't change the control word unless needed.*/

		if(controlWord != oldControlWord){
        	WRITE32(m_pReg, FracSynthWord, controlWord);
    	
			epicsUInt16 uSecDivider = (epicsUInt16)m_clkSpeed;
			printf("Rounded Interger value: %d\n", uSecDivider);
			WRITE16(m_pReg, uSecDiv, uSecDivider);
		}

		status = OK;
	}

	m_clkSpeed = clkSpeed;
	return status;
}

epicsFloat64
evgEvtClk::getClkSpeed() {
	if(m_clkSrc == evgClkSrcInternal)
		return m_clkSpeed;
	else
		return m_RFref/m_clkSrc;
}

/** 	Event Clock Source 	**/
//TODO: Set uSecDiv for RF source.
epicsStatus
evgEvtClk::setClkSource (epicsUInt8 clkSrc) {
	if(clkSrc == m_clkSrc)
		return OK;
		
	if (clkSrc == evgClkSrcInternal) {
		/* If you want to use the event clock from Frctional synthesizer then set
 	  	the clkSpeed first. */
		if(!m_clkSpeed) {
			errlogPrintf("ERROR: Desired Event clock speed not set.\n");
			return ERROR;
		}

		m_clkSrc = clkSrc;
		BITCLR8 (m_pReg, ClockSource, EVG_CLK_SRC_EXTRF);
		setClkSpeed(m_clkSpeed);

	} else if(clkSrc >= 1  && clkSrc <= 32) {     
		/* It is necessary to set RFref first if you want to use the event clock
 		derived from RF clock. */
		if(!m_RFref) {
			errlogPrintf("ERROR: RF Input frequency not set\n");
			return ERROR;
		}

		m_clkSrc = clkSrc;
    	BITSET8 (m_pReg, ClockSource, EVG_CLK_SRC_EXTRF);
		WRITE8 (m_pReg, RfDiv, clkSrc-1);
          
	} else {
		errlogPrintf("ERROR: Invalid Clock Source.\n");
		return ERROR;	
	}

	return OK;
}

epicsUInt8
evgEvtClk::getClkSource() {
	epicsUInt8 clkReg = READ8(m_pReg, ClockSource);
	return clkReg & EVG_CLK_SRC_EXTRF;
}
