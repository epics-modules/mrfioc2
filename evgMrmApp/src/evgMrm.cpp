#include "evgMrm.h"

#include <iostream>
#include <stdexcept>
#include <math.h>

#include <errlog.h> 

#include <dbAccess.h>
#include <devSup.h>
#include <recSup.h>
#include <recGbl.h>
#include <epicsInterrupt.h>
#include <epicsTime.h>
#include <generalTimeSup.h>

#include <longoutRecord.h>

#include <mrfCommonIO.h> 
#include <mrfCommon.h> 

#ifdef __rtems__
#include <rtems/bspIo.h>
#endif //__rtems__

#include "evgRegMap.h"

evgMrm::evgMrm(const epicsUInt32 id, volatile epicsUInt8* const pReg):
m_id(id),
m_pReg(pReg),
m_evtClk(pReg),
m_softEvt(pReg),
m_seqRamMgr(this),
m_softSeqMgr(this) {

	try{
		m_ppsSrc.type = None_Input;
		m_ppsSrc.num = 0;

		for(int i = 0; i < evgNumEvtTrig; i++)
			m_trigEvt.push_back(new evgTrigEvt(i, pReg));

		for(int i = 0; i < evgNumMxc; i++) 
			m_muxCounter.push_back(new evgMxc(i, this));

		for(int i = 0; i < evgNumDbusBit; i++)
			m_dbus.push_back(new evgDbus(i, pReg));

		for(int i = 0; i < evgNumFpInp; i++) {
			m_input[ std::pair<epicsUInt32, InputType>(i, FP_Input) ] = 
					new evgInput(i, FP_Input, pReg + U32_FPInMap(i));
		}

		for(int i = 0; i < evgNumUnivInp; i++) {
			m_input[ std::pair<epicsUInt32, InputType>(i, Univ_Input) ] = 
					new evgInput(i, Univ_Input, pReg + U32_UnivInMap(i));
		}

		for(int i = 0; i < evgNumTbInp; i++) {
			m_input[ std::pair<epicsUInt32, InputType>(i, TB_Input) ] = 
 					new evgInput(i, TB_Input, pReg + U32_TBInMap(i));
		}

		for(int i = 0; i < evgNumFpOut; i++) {
			m_output[std::pair<epicsUInt32, OutputType>(i, FP_Output)] = 
										new evgOutput(i, pReg, FP_Output);
		}

		for(int i = 0; i < evgNumUnivOut; i++) {
			m_output[std::pair<epicsUInt32, OutputType>(i, Univ_Output)] = 
										new evgOutput(i, pReg, Univ_Output);
		}	

		init_cb(&irqStop0_cb, priorityHigh, &evgMrm::process_cb, &irqStop0);
		init_cb(&irqStop1_cb, priorityHigh, &evgMrm::process_cb, &irqStop1);
		init_cb(&irqExtInp_cb, priorityHigh, &evgMrm::sendTS, this);
	
		scanIoInit(&ioscanpvt);
	} catch(std::exception& e) {
		errlogPrintf("%s\n", e.what());
	}
}

evgMrm::~evgMrm() {
	for(int i = 0; i < evgNumEvtTrig; i++)
		delete m_trigEvt[i];

	for(int i = 0; i < evgNumMxc; i++)
		delete m_muxCounter[i];

	for(int i = 0; i < evgNumDbusBit; i++)
		delete m_dbus[i];

	for(int i = 0; i < evgNumFpInp; i++)
		delete m_input[std::pair<epicsUInt32, InputType>(i, FP_Input)]; 

	for(int i = 0; i < evgNumUnivInp; i++)
		delete m_input[std::pair<epicsUInt32, InputType>(i, Univ_Input)];

	for(int i = 0; i < evgNumTbInp; i++)
		delete m_input[std::pair<epicsUInt32, InputType>(i, TB_Input)];

	for(int i = 0; i < evgNumFpOut; i++)
		delete m_output[std::pair<epicsUInt32, OutputType>(i, FP_Output)]; 

	for(int i = 0; i < evgNumUnivOut; i++)
		delete m_output[std::pair<epicsUInt32, OutputType>(i, Univ_Output)];
}

void 
evgMrm::init_cb(CALLBACK *ptr, int priority, void(*fn)(CALLBACK*), void* valptr) { 
  	callbackSetPriority(priority, ptr); 
  	callbackSetCallback(fn, ptr);   
  	callbackSetUser(valptr, ptr);   
  	(ptr)->timer=NULL;              
}

const epicsUInt32 
evgMrm::getId() {
	return m_id;
}

volatile epicsUInt8*
evgMrm::getRegAddr() {
	return m_pReg;
}

epicsUInt32 
evgMrm::getFwVersion() {
	return READ32(m_pReg, FPGAVersion);
}

epicsUInt32
evgMrm::getStatus() {
	return READ32( m_pReg, Status);
}

epicsStatus
evgMrm::enable(bool ena) {
	if(ena)
		BITSET32(m_pReg, Control, EVG_MASTER_ENA);
	else
		BITCLR32(m_pReg, Control, EVG_MASTER_ENA);

	BITSET32(m_pReg, Control, EVG_DIS_EVT_REC);
	BITSET32(m_pReg, Control, EVG_REV_PWD_DOWN);

	return OK;
}

void
evgMrm::isr(void* arg) {
	evgMrm *evg = (evgMrm*)(arg);

	epicsUInt32 flags = READ32(evg->getRegAddr(), IrqFlag);
	epicsUInt32 enable = READ32(evg->getRegAddr(), IrqEnable);
	epicsUInt32 active = flags & enable;
	
	if(!active)
		return;
	
	if(active & EVG_IRQ_STOP_RAM(0)) {
		callbackRequest(&evg->irqStop0_cb);
		BITCLR32(evg->getRegAddr(), IrqEnable, EVG_IRQ_STOP_RAM(0));
	}

	if(active & EVG_IRQ_STOP_RAM(1)) {
		callbackRequest(&evg->irqStop1_cb);
		BITCLR32(evg->getRegAddr(), IrqEnable, EVG_IRQ_STOP_RAM(1));
	}

	if(active & EVG_IRQ_EXT_INP) {
		callbackRequest(&evg->irqExtInp_cb);
	}

	WRITE32(evg->m_pReg, IrqFlag, flags);
	return;
}

void
evgMrm::process_cb(CALLBACK *pCallback) {
	void* pVoid;
	struct irqPvt *pIrqPvt;
	dbCommon *pRec;
	struct rset *prset;
	
	callbackGetUser(pVoid, pCallback);
	pIrqPvt = (struct irqPvt *)pVoid;

	SCOPED_LOCK2(pIrqPvt->mutex, guard);
	for(unsigned int i = 0; i < pIrqPvt->recList.size(); i++) {
		pRec = pIrqPvt->recList[i];
		prset = (struct rset*)pRec->rset;

		dbScanLock(pRec);
		(*(long (*)(dbCommon*))prset->process)(pRec);
		dbScanUnlock(pRec);
	}
	pIrqPvt->recList.clear();
}

/** TimeStamps	**/
void
evgMrm::sendTS(CALLBACK *pCallback) {
	void* pVoid;
	callbackGetUser(pVoid, pCallback);
	evgMrm* evg = (evgMrm*)pVoid;
   
	evg->incrementTS();
	scanIoRequest(evg->ioscanpvt);

	struct epicsTimeStamp ts;
	epicsTime ntpTime, storedTime;

	if(epicsTimeOK == generalTimeGetExceptPriority(&ts, 0, 50)) {
		ntpTime = ts;
		storedTime = (epicsTime)evg->getTS();

		double errorTime = ntpTime - storedTime;
		char timeBuf[80];

		if(fabs(errorTime) > evgAllowedTsErr) {
			ntpTime.strftime(timeBuf, sizeof(timeBuf), "%a, %d %b %Y %H:%M:%S"); 
			errlogPrintf("NTP time : %s\n", timeBuf);
			storedTime.strftime(timeBuf, sizeof(timeBuf), "%a, %d %b %Y %H:%M:%S"); 
			errlogPrintf("Expected time : %s\n", timeBuf);
			errlogPrintf("----Timestamping Error of %f Secs----\n", errorTime);
    	}
 	}

	epicsUInt32 sec = evg->getTSsec() + 1 + POSIX_TIME_AT_EPICS_EPOCH;
	for(int i = 0; i < 32; sec <<= 1, i++) {
		if( sec & 0x80000000 ) {
			evg->getSoftEvt()->setEvtCode(0x71);
		} else {
			evg->getSoftEvt()->setEvtCode(0x70);
		}
	}
}

epicsStatus
evgMrm::incrementTS() {
	m_tv.secPastEpoch++;
	return OK;
}

epicsUInt32
evgMrm::getTSsec() {
	return m_tv.secPastEpoch;
}

epicsStatus
evgMrm::syncTS() {
	while(generalTimeGetExceptPriority(&m_tv, 0, 50));
	m_tv.nsec = 0;
	return OK;
}

epicsTimeStamp
evgMrm::getTS() {
	return m_tv;
}

epicsStatus
evgMrm::setTsInpType(InputType type) {
	if(m_ppsSrc.type == type)
		return OK;

	/*Check if such an input exists*/
	if(type != None_Input)
		getInput(m_ppsSrc.num, type);

	setupTS(0);
	m_ppsSrc.type = type;
	setupTS(1);

	return OK;
}

epicsStatus
evgMrm::setTsInpNum(epicsUInt32 num) {
	if(m_ppsSrc.num == num)
		return OK;

	/*Check if such an input exists*/
	if(m_ppsSrc.type != None_Input)
		getInput(num, m_ppsSrc.type);

	setupTS(0);
	m_ppsSrc.num = num;
	setupTS(1);
	
	return OK;
}

InputType
evgMrm::getTsInpType() {
	return m_ppsSrc.type;
}

epicsUInt32
evgMrm::getTsInpNum() {
	return m_ppsSrc.num;
}

epicsStatus
evgMrm::setupTS(bool ena) {
	if(m_ppsSrc.type == None_Input)
		return OK;

	evgInput* inp = getInput(m_ppsSrc.num, m_ppsSrc.type);
	if(ena) {
		inp->enaExtIrq(1);
		syncTS();
	} else {
		inp->enaExtIrq(0);
	}

	return OK;
}

/**	Access	functions 	**/

evgEvtClk*
evgMrm::getEvtClk() {
	return &m_evtClk;
}

evgSoftEvt*
evgMrm::getSoftEvt() {
	return &m_softEvt;
}

evgTrigEvt*
evgMrm::getTrigEvt(epicsUInt32 evtTrigNum) {
	evgTrigEvt* trigEvt = m_trigEvt[evtTrigNum];
	if(!trigEvt)
		throw std::runtime_error("Event Trigger not initialized");

	return trigEvt;
}

evgMxc* 
evgMrm::getMuxCounter(epicsUInt32 muxNum) {
	evgMxc* mxc =  m_muxCounter[muxNum];
	if(!mxc)
		throw std::runtime_error("Multiplexed Counter not initialized");

	return mxc;
}

evgDbus*
evgMrm::getDbus(epicsUInt32 dbusBit) {
	evgDbus* dbus = m_dbus[dbusBit];
	if(!dbus)
		throw std::runtime_error("Event Dbus not initialized");

	return dbus;
}

evgInput*
evgMrm::getInput(epicsUInt32 inpNum, InputType type) {
	evgInput* inp = m_input[ std::pair<epicsUInt32, InputType>(inpNum, type) ];
	if(!inp)
		throw std::runtime_error("Input not initialized");

	return inp;
}

evgOutput*
evgMrm::getOutput(epicsUInt32 outNum, OutputType type) {
	evgOutput* out = m_output[ std::pair<epicsUInt32, OutputType>(outNum, type) ];
	if(!out)
		throw std::runtime_error("Output not initialized");

	return out;
}

evgSeqRamMgr*
evgMrm::getSeqRamMgr() {
	return &m_seqRamMgr;
}

evgSoftSeqMgr*
evgMrm::getSoftSeqMgr() {
	return &m_softSeqMgr;
}


