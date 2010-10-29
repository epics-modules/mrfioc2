#ifndef EVG_MRM_H
#define EVG_MRM_H

#include <vector>
#include <map>
#include <string>
#include <sys/time.h>

#include <epicsTypes.h>
#include <dbScan.h>
#include <callback.h>
#include <epicsMutex.h>
#include <epicsTimer.h>
#include <errlog.h>

#include "evgEvtClk.h"
#include "evgSoftEvt.h"
#include "evgTrigEvt.h"
#include "evgMxc.h"
#include "evgDbus.h"
#include "evgInput.h"
#include "evgOutput.h"
#include "evgSequencer/evgSeqRamManager.h"  
#include "evgSequencer/evgSoftSeqManager.h"
#include "mrmDataBufTx.h"

/*********
 * Each EVG will be represented by the instance of class 'evgMrm'. Each evg 
 * object maintains a list to all the evg sub-componets i.e. Event clock,
 * Software Events, Trigger Events, Distributed bus, Multiplex Counters, 
 * Input, Output etc.
 */
class wdTimer;
enum ALARM_TS {TS_ALARM_NONE, TS_ALARM_MINOR, TS_ALARM_MAJOR};

class evgMrm {
public:
	evgMrm(const epicsUInt32 CardNum, volatile epicsUInt8* const pReg);
	~evgMrm();

	/** EVG	**/
	const epicsUInt32 getId();	
	volatile epicsUInt8* getRegAddr(); 
	epicsUInt32 getFwVersion();
	epicsStatus enable(bool ena);
	epicsUInt32 getStatus();

	/**	Interrupt and Callback	**/
	static void isr(void*);
	static void init_cb(CALLBACK*, int, void(*)(CALLBACK*), void*);
	static void process_cb(CALLBACK*);

	/** TimeStamp	**/
	static void sendTS(CALLBACK*);
	epicsTimeStamp getTS();
	epicsStatus syncTS();
	epicsStatus syncTsRequest();
	epicsStatus setTsInpType(InputType);
	epicsStatus setTsInpNum(epicsUInt32);
	InputType getTsInpType();
	epicsUInt32 getTsInpNum();
	epicsStatus setupTsIrq(bool);
	epicsStatus incrementTSsec();
	epicsUInt32 getTSsec();
	
	/**	Access	functions 	**/
	evgEvtClk* 		getEvtClk		();
	evgSoftEvt*		getSoftEvt		();
	evgTrigEvt* 	getTrigEvt		(epicsUInt32);
	evgMxc* 		getMuxCounter	(epicsUInt32);
	evgDbus* 		getDbus			(epicsUInt32);
	evgInput*  		getInput		(epicsUInt32, InputType);
	evgOutput* 		getOutput		(epicsUInt32, OutputType);
	evgSeqRamMgr* 	getSeqRamMgr	();	
	evgSoftSeqMgr* 	getSoftSeqMgr	();

	struct ppsSrc{
		InputType type;
		epicsUInt32	num;	
	};

	CALLBACK                        irqStop0_cb;
	CALLBACK                        irqStop1_cb;
	CALLBACK                        irqExtInp_cb;

	IOSCANPVT                       ioScanTS;
	epicsUInt32                     m_pilotCountTS;
	bool                            m_syncTS;
 	ALARM_TS                        m_alarmTS;

	mrmDataBufTx                    m_buftx;

private:
	const epicsUInt32               m_id;       
	volatile epicsUInt8* const      m_pReg;

	evgEvtClk                       m_evtClk;
	evgSoftEvt                      m_softEvt;

	typedef std::vector<evgTrigEvt*> TrigEvt_t;
  	TrigEvt_t                       m_trigEvt;

	typedef std::vector<evgMxc*>    MuxCounter_t;
  	MuxCounter_t                    m_muxCounter;

	typedef std::vector<evgDbus*>   Dbus_t;
  	Dbus_t                          m_dbus;

 	typedef std::map< std::pair<epicsUInt32, InputType>, evgInput*> Input_t;
 	Input_t                         m_input;

 	typedef std::map< std::pair<epicsUInt32, OutputType>, evgOutput*> Output_t;
 	Output_t                        m_output;

	evgSeqRamMgr                    m_seqRamMgr;
	evgSoftSeqMgr                   m_softSeqMgr;

	epicsTimeStamp                  m_ts;
	struct ppsSrc                   m_ppsSrc; 

	epicsTimerQueueActive &	        m_queue; 
	wdTimer*                        m_wdTimer;
};

class wdTimer : public epicsTimerNotify {
public:
	wdTimer(const char* name,epicsTimerQueueActive &queue, evgMrm* const evg): 
	m_name(name),
	m_timer(queue.createTimer()),
	m_owner(evg),
	m_timeout(true) {
	}

	virtual ~wdTimer() { 
		m_timer.destroy();
	}

	void start(double delay) {
		m_timer.start(*this,delay);
	}

	virtual expireStatus expire(const epicsTime & currentTime) {
		printf("Timestamp watchdog timer expired\n");
		currentTime.show(1);
		m_timeout = true;
		m_owner->m_alarmTS = TS_ALARM_MAJOR;
		scanIoRequest(m_owner->ioScanTS);
		return expireStatus(noRestart);
	}

	void clearTimeoutFlag() {
		m_timeout = false;
		return;
	}

	bool getTimeoutFlag() {
		return m_timeout;
	}

private:
	const char* m_name;
	epicsTimer& m_timer;
	evgMrm* const m_owner;
	bool m_timeout;
};

#endif //EVG_MRM_H
