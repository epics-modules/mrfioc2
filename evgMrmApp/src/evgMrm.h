#ifndef EVG_MRM_H
#define EVG_MRM_H

#include <vector>
#include <map>
#include <string>

#include <epicsTypes.h>
#include <dbScan.h>
#include <callback.h>
#include <epicsMutex.h>

#include "evgEvtClk.h"
#include "evgSoftEvt.h"
#include "evgTrigEvt.h"
#include "evgMxc.h"
#include "evgDbus.h"
#include "evgInput.h"
#include "evgOutput.h"
#include "evgSequencer/evgSeqRamManager.h"  
#include "evgSequencer/evgSoftSeqManager.h"

/*********
 * Each EVG will be represented by the instance of class 'evgMrm'. Each evg 
 * object maintains a list to all the evg sub-componets i.e. Event clock,
 * Software Events, Trigger Events, Distributed bus, Multiplex Counters, 
 * Input, Output etc.
 */

class evgMrm {

public:
	evgMrm(const epicsUInt32 CardNum, volatile epicsUInt8* const pReg);
	~evgMrm();

	/** EVG	**/
	const epicsUInt32 getId();	
	volatile epicsUInt8* const getRegAddr(); 
	epicsUInt32 getFwVersion();
	epicsStatus enable(bool ena);

	/**	Interrupt and Callback	**/
	static void isr(void*);
	static void process_cb(CALLBACK*);
	void init_cb(CALLBACK*, int, void(*)(CALLBACK*), void*);
	
	/**	Access	function 	**/
	evgEvtClk* 		getEvtClk		();
	evgSoftEvt*		getSoftEvt		();
	evgTrigEvt* 	getTrigEvt		(epicsUInt32);
	evgMxc* 		getMuxCounter	(epicsUInt32);
	evgDbus* 		getDbus			(epicsUInt32);
	evgInput*  		getInput		(epicsUInt32, std::string);
	evgOutput* 		getOutput		(epicsUInt32, std::string);
	evgSeqRamMgr* 	getSeqRamMgr	();	
	evgSoftSeqMgr* 	getSoftSeqMgr	();

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

	evgEvtClk 						m_evtClk;
	evgSoftEvt						m_softEvt;

	typedef std::vector<evgTrigEvt*> TrigEvt_t;
  	TrigEvt_t 						m_trigEvt;

	typedef std::vector<evgMxc*> 	MuxCounter_t;
  	MuxCounter_t 					m_muxCounter;

	typedef std::vector<evgDbus*> 	Dbus_t;
  	Dbus_t	 						m_dbus;

 	typedef std::map< std::pair<epicsUInt32, std::string>, evgInput*> Input_t;
 	Input_t 						m_input;

 	typedef std::map< std::pair<epicsUInt32, std::string>, evgOutput*> Output_t;
 	Output_t 						m_output;

	evgSeqRamMgr 					m_seqRamMgr;
	evgSoftSeqMgr					m_softSeqMgr;
};

#endif //EVG_MRM_H
