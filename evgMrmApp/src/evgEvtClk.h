#ifndef EVG_EVTCLK_H
#define EVG_EVTCLK_H

#include <epicsTypes.h>

const epicsUInt16 ClkSrcInternal = 0;	// Event clock is generated internally
const epicsUInt16 ClkSrcRF 	     = 1;    // Event clock is derived from the RF input

class evgEvtClk {
public:
	evgEvtClk(volatile epicsUInt8* const);
	
	/** RF Input Frequency	**/
	epicsStatus setRFref (epicsFloat64 RFref);
	epicsFloat64 getRFref();

	epicsStatus setRFdiv(epicsUInt32);
	epicsUInt32 getRFdiv();

	/**	Fractional Synthesizer Frequency	**/
	epicsStatus setFracSynFreq(epicsFloat64);
	epicsFloat64 getEvtClkSpeed();

	/**	Event Clock Source	**/
	epicsStatus setEvtClkSrc(bool);
	bool getEvtClkSrc();	

private:
	volatile epicsUInt8* const		m_pReg;
	epicsFloat64					m_RFref;		//In MHz
	epicsFloat64          			m_fracSynFreq;	// In MHz
};

#endif //EVG_EVTCLK_H
