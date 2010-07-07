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
#include "mrfFracSynth.h"

#ifdef __rtems__
#include <rtems/bspIo.h>
#endif //__rtems__

#include "evgRegMap.h"

evgMrm::evgMrm(const epicsUInt32 id, volatile epicsUInt8* const pReg):
m_id(id),
m_pReg(pReg) {
	try {
		for(int i = 0; i < evgNumMxc; i++) {
			m_muxCounter.push_back(new evgMxc(i, pReg));
		}

		for(int i = 0; i < evgNumEvtTrig; i++) {
			m_trigEvt.push_back(new evgTrigEvt(i, pReg));
		}

		for(int i = 0; i < evgNumDbusBit; i++) {
			m_dbus.push_back(new evgDbus(i, pReg));
		}

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

		m_seqRamMgr = new evgSeqRamMgr(pReg, this);
	} catch(std::exception& e) {
		errlogPrintf("ERROR: EVG %d faied to initialise proprtly\n%s\n", id, e.what());
	} 	

	irqStop0.mutex = epicsMutexMustCreate();
	irqStop1.mutex = epicsMutexMustCreate();

	init_cb(&irqStop0_cb, priorityLow, &evgMrm::process_cb, &irqStop0);
	init_cb(&irqStop1_cb, priorityLow, &evgMrm::process_cb, &irqStop1);
}

evgMrm::~evgMrm() {
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

evgMxc* 
evgMrm::getMuxCounter(epicsUInt32 muxNum) {
	evgMxc* mxc =  m_muxCounter[muxNum];
	if(!mxc)
		throw std::runtime_error("ERROR: Multiplexed Counter not initialized");

	return mxc;
}

evgTrigEvt*
evgMrm::getTrigEvt(epicsUInt32 evtTrigNum) {
	evgTrigEvt* pTrigEvt = m_trigEvt[evtTrigNum];
	if(!pTrigEvt)
		throw std::runtime_error("ERROR: Event Trigger not initialized");

	return pTrigEvt;
}

evgDbus*
evgMrm::getDbus(epicsUInt32 dbusBit) {
	evgDbus* pDbus = m_dbus[dbusBit];
	if(!pDbus)
		throw std::runtime_error("ERROR: Event Dbus not initialized");

	return pDbus;
}

evgInput*
evgMrm::getInput(epicsUInt32 inpNum, std::string type) {
	evgInput* pInp = m_input[ std::pair<epicsUInt32, std::string>(inpNum, type) ];
	if(!pInp)
		throw std::runtime_error("ERROR: Input not initialized");

	return pInp;
}

evgOutput*
evgMrm::getOutput(epicsUInt32 outNum, std::string type) {
	evgOutput* pOut = m_output[ std::pair<epicsUInt32, std::string>(outNum, type) ];
	if(!pOut)
		throw std::runtime_error("ERROR: Output not initialized");

	return pOut;
}

evgSeqRamMgr*
evgMrm::getSeqRamMgr() {
	return m_seqRamMgr;
}

/** 	Event Clock Source 	**/

//TODO: Set uSecDiv for RF source.
epicsStatus
evgMrm::setClockSource (epicsUInt8 clkSrc) {
	if(clkSrc == m_clkSrc)
		return OK;

	if (clkSrc == evgClkSrcInternal) {
		// Use internal fractional synthesizer to generate the clock
		BITCLR8 (m_pReg, ClockSource, EVG_CLK_SRC_EXTRF);
		m_clkSrc = clkSrc;
		setClockSpeed(m_clkSpeed);

	} else if(clkSrc >= 1  && clkSrc <= 32) {     
		// Use external RF source to generate the clock
		m_clkSrc = clkSrc;
		clkSrc = clkSrc - 1;
    	BITSET8 (m_pReg, ClockSource, EVG_CLK_SRC_EXTRF);
		WRITE8 (m_pReg, RfDiv, clkSrc);
	
	} else {
		errlogPrintf("ERROR: Invalid Clock Source.\n");
		return ERROR;	
	}

	return OK;
} //SetOutLinkClockSource()

epicsUInt8
evgMrm::getClockSource() {
	epicsUInt8 clkReg = READ8(m_pReg, ClockSource);
	return clkReg & EVG_CLK_SRC_EXTRF;
}


/**		Event Clock Speed	**/

epicsStatus
evgMrm::setClockSpeed (epicsFloat64 clkSpeed) {	
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

		/* Changing the control word disturbes the phase of 
		 * the synthesiser which will cause a glitch.
    	 * Don't change the control word unless needed.
    	 */
		if(controlWord != oldControlWord){
        	WRITE32(m_pReg, FracSynthWord, controlWord);
    	
			epicsUInt16 uSecDivider = (epicsUInt16)m_clkSpeed;
			printf("Rounded Interge value: %d\n", uSecDivider);
			WRITE16(m_pReg, uSecDiv, uSecDivider);
		}

		status = OK;
	}

	m_clkSpeed = clkSpeed;
	return status;

}//setClockSpeed

epicsFloat64
evgMrm::getClockSpeed() {
	return m_clkSpeed;
}


/**		Software Event		**/

epicsStatus 
evgMrm::softEvtEnable(bool ena){	
	if(ena)
		BITSET8(m_pReg, SwEventControl, SW_EVT_ENABLE);
	else
		BITCLR8(m_pReg, SwEventControl, SW_EVT_ENABLE);
	
	return OK;
}


bool 
evgMrm::softEvtEnabled() {
	epicsUInt8 swEvtCtrl = READ8(m_pReg, SwEventControl);
	return swEvtCtrl & SW_EVT_ENABLE;
}


bool 
evgMrm::softEvtPend() {
	epicsUInt8 swEvtCtrl = READ8(m_pReg, SwEventControl);
	return swEvtCtrl & SW_EVT_PEND;
}

	
epicsStatus 
evgMrm::setSoftEvtCode(epicsUInt32 evtCode) {
	if(evtCode < 0 || evtCode > 255) {
		errlogPrintf("ERROR: Event Code out of range.\n");
		return ERROR;		
	}

	//TODO: if(pend == 0), write in an atomic step to SwEvtCode register. 
	WRITE8(m_pReg, SwEventCode, evtCode);
	return OK; 
}


epicsUInt8 
evgMrm::getSoftEvtCode() {
	return READ8(m_pReg, SwEventCode);
}

