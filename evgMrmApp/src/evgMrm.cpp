#include "evgMrm.h"

#include <iostream>
#include <stdexcept>

#include <errlog.h> 

#include <dbAccess.h>
#include <devSup.h>
#include <recSup.h>
#include <recGbl.h>
#include <epicsInterrupt.h>

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
m_evtClk(evgEvtClk(pReg)),
m_softEvt(evgSoftEvt(pReg)),
m_seqRamMgr(evgSeqRamMgr(this)),
m_seqMgr(evgSeqMgr(this)) {
	try {
		for(int i = 0; i < evgNumEvtTrig; i++)
			m_trigEvt.push_back(new evgTrigEvt(i, pReg));

		for(int i = 0; i < evgNumMxc; i++) 
			m_muxCounter.push_back(new evgMxc(i, this));

		for(int i = 0; i < evgNumDbusBit; i++)
			m_dbus.push_back(new evgDbus(i, pReg));

		for(int i = 0; i < evgNumFpInp; i++) {
			m_input[ std::pair<epicsUInt32, std::string>(i,"FP_Input") ] = 
											new evgInput(i, pReg, FP_Input);
		}
		for(int i = 0; i < evgNumUnivInp; i++) {
			m_input[ std::pair<epicsUInt32, std::string>(i,"Univ_Input") ] = 
											new evgInput(i, pReg, Univ_Input);
		}
		for(int i = 0; i < evgNumFpOut; i++) {
			m_output[std::pair<epicsUInt32, std::string>(i,"FP_Output")] = 
											new evgOutput(i, pReg, FP_Output);
		}

		for(int i = 0; i < evgNumUnivOut; i++) {
			m_output[std::pair<epicsUInt32, std::string>(i,"Univ_Output")] = 
											new evgOutput(i, pReg, Univ_Output);
		}	
	} catch(std::exception& e) {
		errlogPrintf("ERROR: EVG %d failed to initialise proprtly\n%s\n", id, e.what());
	} 	

	irqStop0.mutex = epicsMutexMustCreate();
	irqStop1.mutex = epicsMutexMustCreate();

	init_cb(&irqStop0_cb, priorityLow, &evgMrm::process_cb, &irqStop0);
	init_cb(&irqStop1_cb, priorityLow, &evgMrm::process_cb, &irqStop1);
}

evgMrm::~evgMrm() {
	for(int i = 0; i < evgNumEvtTrig; i++)
		delete m_trigEvt[i];

	for(int i = 0; i < evgNumMxc; i++)
		delete m_muxCounter[i];

	for(int i = 0; i < evgNumDbusBit; i++)
		delete m_dbus[i];

	for(int i = 0; i < evgNumFpInp; i++)
		delete m_input[std::pair<epicsUInt32, std::string>(i,"FP_Input")]; 

	for(int i = 0; i < evgNumUnivInp; i++)
		delete m_input[std::pair<epicsUInt32, std::string>(i,"Univ_Input")];

	for(int i = 0; i < evgNumFpOut; i++)
		delete m_output[std::pair<epicsUInt32, std::string>(i,"FP_Output")]; 

	for(int i = 0; i < evgNumUnivOut; i++)
		delete m_output[std::pair<epicsUInt32, std::string>(i,"Univ_Output")];
}

const epicsUInt32 
evgMrm::getId() {
	return m_id;
}

volatile epicsUInt8* const 
evgMrm::getRegAddr() {
	return m_pReg;
}

epicsUInt32 
evgMrm::getFwVersion() {
	return READ32(m_pReg, FPGAVersion);
}

inline void 
evgMrm::init_cb(CALLBACK *ptr, int priority, void(*fn)(CALLBACK*), void* valptr) { 
  	callbackSetPriority(priority, ptr); 
  	callbackSetCallback(fn, ptr);   
  	callbackSetUser(valptr, ptr);   
  	(ptr)->timer=NULL;              
}

/**		Enable EVG	**/
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
	
	#ifdef __rtems__
	printk("\n ----------------------------------------------- \n");
	printk("1.\nflags  : %08x\nactive : %08x\n", flags, active);
	#endif //__rtems__

    if(!active)
      return;

    if(active & EVG_IRQ_STOP_RAM(0)) {
		#ifdef __rtems__
		printk("EVG_IRQ_STOP_RAM0\n");
		#endif //__rtems__	
		callbackRequest(&evg->irqStop0_cb);
		BITCLR32(evg->getRegAddr(), IrqEnable, EVG_IRQ_STOP_RAM(0));
    }

	 if(active & EVG_IRQ_STOP_RAM(1)) {
		#ifdef __rtems__
		printk("EVG_IRQ_STOP_RAM1\n");
		#endif //__rtems__
		callbackRequest(&evg->irqStop1_cb);
		BITCLR32(evg->getRegAddr(), IrqEnable, EVG_IRQ_STOP_RAM(1));
    }
	
	if(active & EVG_IRQ_EXT_INP) {
		#ifdef __rtems__
		printk("EVG_IRQ_EXT_INP\n");
		#endif //_rtems_
	}

    WRITE32(evg->m_pReg, IrqFlag, flags);

	flags = READ32(evg->getRegAddr(), IrqFlag);
    enable = READ32(evg->getRegAddr(), IrqEnable);
    active = flags & enable;

	#ifdef __rtems__
	printk("2.\nflags  : %08x\nactive : %08x\n", flags, active);
	#endif //_rtems_
	
	return;
}

void
evgMrm::process_cb(CALLBACK *pCallback) {
	printf("In Callback..\n");
	void* pVoid;
	struct irq *irqTemp;
	dbCommon *pRec;
	struct rset *prset;
	
	callbackGetUser(pVoid, pCallback);
	irqTemp = (struct irq *)pVoid;

	epicsMutexLock(irqTemp->mutex);
	for(unsigned int i = 0; i < irqTemp->recList.size(); i++) {
		pRec = irqTemp->recList[i];
		prset = (struct rset*)pRec->rset;
		printf("Processing Record : %s \n", pRec->name);
		printf("----------------------------------------------- \n\n");
		dbScanLock(pRec);
		(*(long (*)(dbCommon*))prset->process)(pRec);
		dbScanUnlock(pRec);
	}
	irqTemp->recList.clear();
	epicsMutexUnlock(irqTemp->mutex);
}

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
		throw std::runtime_error("ERROR: Event Trigger not initialized");

	return trigEvt;
}

evgMxc* 
evgMrm::getMuxCounter(epicsUInt32 muxNum) {
	evgMxc* mxc =  m_muxCounter[muxNum];
	if(!mxc)
		throw std::runtime_error("ERROR: Multiplexed Counter not initialized");

	return mxc;
}

evgDbus*
evgMrm::getDbus(epicsUInt32 dbusBit) {
	evgDbus* dbus = m_dbus[dbusBit];
	if(!dbus)
		throw std::runtime_error("ERROR: Event Dbus not initialized");

	return dbus;
}

evgInput*
evgMrm::getInput(epicsUInt32 inpNum, std::string type) {
	evgInput* inp = m_input[ std::pair<epicsUInt32, std::string>(inpNum, type) ];
	if(!inp)
		throw std::runtime_error("ERROR: Input not initialized");

	return inp;
}

evgOutput*
evgMrm::getOutput(epicsUInt32 outNum, std::string type) {
	evgOutput* out = m_output[ std::pair<epicsUInt32, std::string>(outNum, type) ];
	if(!out)
		throw std::runtime_error("ERROR: Output not initialized");

	return out;
}

evgSeqRamMgr*
evgMrm::getSeqRamMgr() {
	return &m_seqRamMgr;
}

evgSeqMgr*
evgMrm::getSeqMgr() {
	return &m_seqMgr;
}


