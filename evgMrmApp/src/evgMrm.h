#ifndef EVGMRM_H
#define EVGMRM_H

#include <vector>
#include <map>
#include <string>

#include <epicsTypes.h>
#include <dbScan.h>
#include <callback.h>
#include <epicsMutex.h>

#include "evgSeqRamManager.h"  
#include "evgMxc.h"
#include "evgTrigEvt.h"
#include "evgDbus.h"
#include "evgInput.h"
#include "evgOutput.h"


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
	
	epicsStatus setSoftEvtCode(epicsUInt32);
	epicsUInt8 getSoftEvtCode();

	/**	Access	function 	**/
	evgMxc* 		getMuxCounter	(epicsUInt32); 
	evgTrigEvt* 	getTrigEvt		(epicsUInt32);
	evgDbus* 		getDbus			(epicsUInt32);
	evgInput*  		getInput		(epicsUInt32, std::string);
	evgOutput* 		getOutput		(epicsUInt32, std::string);
	evgSeqRamMgr* 	getSeqRamMgr	();	

	struct irq {
		epicsMutexId mutex;
		std::vector<dbCommon*> recList;
	};
	
	struct irq irqStop0;
	struct irq irqStop1;

	CALLBACK 						irqStop0_cb;
	CALLBACK						irqStop1_cb;

private:
	const epicsUInt32            	m_id;       
	volatile epicsUInt8* const		m_pReg;

    epicsFloat64          			m_clkSpeed;	// In MHz
	epicsUInt32						m_clkSrc;
	
	typedef std::vector<evgMxc*> 	MuxCounter_t;
  	MuxCounter_t m_muxCounter;

	typedef std::vector<evgTrigEvt*> TrigEvt_t;
  	TrigEvt_t m_trigEvt;

	typedef std::vector<evgDbus*> 	Dbus_t;
  	Dbus_t m_dbus;

 	typedef std::map< std::pair<epicsUInt32, std::string>, evgInput*> Input_t;
 	Input_t m_input;

 	typedef std::map< std::pair<epicsUInt32, std::string>, evgOutput*> Output_t;
 	Output_t m_output;

	evgSeqRamMgr* 					m_seqRamMgr;
};

#endif //EVGMRM_H