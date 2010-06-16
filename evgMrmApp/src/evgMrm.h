#ifndef EVGMRM_H
#define EVGMRM_H

#include <vector>
#include <map>
#include <string>

#include <epicsTypes.h>
#include <dbScan.h>
#include <callback.h>
#include <epicsMutex.h>
  
#include "evgMxc.h"
#include "evgTrigEvt.h"
#include "evgDbus.h"
#include "evgFPio.h"
#include "evgSeqRamSup.h"

const epicsUInt16 evgClkSrcInternal = 0;	// Event clock is generated internally
const epicsUInt16 evgClkSrcRF 	    = 1;    // Event clock is derived from the RF input

class evgMrm {

public:

	/** EVG	**/	
	evgMrm(const epicsUInt32 CardNum, volatile epicsUInt8* const pReg);
	~evgMrm();

	const epicsUInt32 getId();	
	
	volatile epicsUInt8* const getRegAddr(); 

	epicsUInt32 getFwVersion();

	epicsStatus enable(bool ena);

	static void isr(void*);
	static void process_cb(CALLBACK*);
	void init_cb(CALLBACK*, int, void(*)(CALLBACK*), void*);

	/**	Event Clock Speed	**/
	epicsStatus setClockSpeed(epicsFloat64);
	epicsFloat64 getClockSpeed();

	/**	Event Clock Source	**/
	epicsStatus setClockSource(epicsUInt8);
	epicsUInt8 getClockSource();	

	/**	Software Event	**/
	epicsStatus softEvtEnable(bool);
 	bool softEvtEnabled();

	bool softEvtPend();
	
	epicsStatus setSoftEvtCode(epicsUInt8);
	epicsUInt8 getSoftEvtCode();

	/**	Access	function 	**/
	evgMxc* getMuxCounter(epicsUInt32); 
	evgTrigEvt* getTrigEvt(epicsUInt32);
	evgDbus* getDbus(epicsUInt32 dbusBit);
	evgFPio* getFPio(epicsUInt32, std::string);
	evgSeqRamSup* getSeqRamSup();	

	struct irq {
		epicsMutexId mutex;
		std::vector<dbCommon*> recList;
	};
	
	struct irq irqStop0;
	struct irq irqStop1;
	struct irq irqStart0;
	struct irq irqStart1;

private:
	const epicsUInt32            	m_id;         // Logical number for this card
	volatile epicsUInt8* const		m_pReg;      	// CPU Address of the card's register map

    epicsFloat64          			m_clkSpeed;	// In MHz
	epicsUInt32						m_clkSrc;
	
	typedef std::vector<evgMxc*> 	MuxCounter_t;
  	MuxCounter_t m_muxCounter;

	typedef std::vector<evgTrigEvt*> TrigEvt_t;
  	TrigEvt_t m_trigEvt;

	typedef std::vector<evgDbus*> 	Dbus_t;
  	Dbus_t m_dbus;

	typedef std::map< std::pair<epicsUInt32, std::string >, evgFPio*> FPio_t;
	FPio_t m_FPio;

	evgSeqRamSup* 					m_seqRamSup;

	CALLBACK 						irqStop0_cb;
	CALLBACK						irqStop1_cb;
	CALLBACK						irqStart0_cb;
	CALLBACK						irqStart1_cb;
};

#endif //EVGMRM_H