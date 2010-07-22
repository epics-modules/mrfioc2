#ifndef EVG_EVTCLK_H
#define EVG_EVTCLK_H

#include <epicsTypes.h>

const epicsUInt16 evgClkSrcInternal = 0;	// Event clock is generated internally
const epicsUInt16 evgClkSrcRF 	    = 1;    // Event clock is derived from the RF input

class evgEvtClk {
public:
	evgEvtClk(volatile epicsUInt8* const);
	
	/** RF Input Frequency	**/
	epicsStatus setRFref (epicsFloat64 RFref);
	epicsFloat64 getRFref();

	/**	Event Clock Speed	**/
	epicsStatus setClkSpeed(epicsFloat64);
	epicsFloat64 getClkSpeed();

	/**	Event Clock Source	**/
	epicsStatus setClkSource(epicsUInt8);
	epicsUInt8 getClkSource();	

private:
	volatile epicsUInt8* const		m_pReg;
	epicsFloat64					m_RFref;
    epicsFloat64          			m_clkSpeed;	// In MHz
	epicsUInt32						m_clkSrc;
};

#endif //EVG_EVTCLK_H