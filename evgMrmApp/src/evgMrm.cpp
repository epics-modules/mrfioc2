#include "evgMrm.h"

#include <iostream>
#include <sstream>
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

#include "mrf/version.h"
#include <mrfCommonIO.h> 
#include <mrfCommon.h> 

#ifdef __rtems__
#include <rtems/bspIo.h>
#endif //__rtems__

#include "evgRegMap.h"

#include <epicsExport.h>


evgMrm::evgMrm(const std::string& id, volatile epicsUInt8* const pReg):
mrf::ObjectInst<evgMrm>(id),
irqStop0_queued(0),
irqStop1_queued(0),
irqStart0_queued(0),
irqStart1_queued(0),
irqExtInp_queued(0),
m_syncTimestamp(false),
m_buftx(id+":BUFTX",pReg+U32_DataBufferControl, pReg+U8_DataBuffer_base),
m_id(id),
m_pReg(pReg),
m_acTrig(id+":AcTrig", pReg),
m_evtClk(id+":EvtClk", pReg),
m_softEvt(id+":SoftEvt", pReg),
m_seqRamMgr(this),
m_softSeqMgr(this) {

    try{
        for(int i = 0; i < evgNumEvtTrig; i++) {
            std::ostringstream name;
            name<<id<<":TrigEvt"<<i;
            m_trigEvt.push_back(new evgTrigEvt(name.str(), i, pReg));
        }

        for(int i = 0; i < evgNumMxc; i++) {
            std::ostringstream name;
            name<<id<<":Mxc"<<i;
            m_muxCounter.push_back(new evgMxc(name.str(), i, this));
        }

        for(int i = 0; i < evgNumDbusBit; i++) {
            std::ostringstream name;
            name<<id<<":Dbus"<<i;
            m_dbus.push_back(new evgDbus(name.str(), i, pReg));
        }

        for(int i = 0; i < evgNumFrontInp; i++) {
            std::ostringstream name;
            name<<id<<":FrontInp"<<i;
            m_input[ std::pair<epicsUInt32, InputType>(i, FrontInp) ] =
                new evgInput(name.str(), i, FrontInp, pReg + U32_FrontInMap(i));
        }

        for(int i = 0; i < evgNumUnivInp; i++) {
            std::ostringstream name;
            name<<id<<":UnivInp"<<i;
            m_input[ std::pair<epicsUInt32, InputType>(i, UnivInp) ] =
                new evgInput(name.str(), i, UnivInp, pReg + U32_UnivInMap(i));
        }

        for(int i = 0; i < evgNumRearInp; i++) {
            std::ostringstream name;
            name<<id<<":RearInp"<<i;
            m_input[ std::pair<epicsUInt32, InputType>(i, RearInp) ] =
                new evgInput(name.str(), i, RearInp, pReg + U32_RearInMap(i));
        }

        for(int i = 0; i < evgNumFrontOut; i++) {
            std::ostringstream name;
            name<<id<<":FrontOut"<<i;
            m_output[std::pair<epicsUInt32, evgOutputType>(i, FrontOut)] =
                new evgOutput(name.str(), i, FrontOut, pReg + U16_FrontOutMap(i));
        }

        for(int i = 0; i < evgNumUnivOut; i++) {
            std::ostringstream name;
            name<<id<<":UnivOut"<<i;
            m_output[std::pair<epicsUInt32, evgOutputType>(i, UnivOut)] =
                new evgOutput(name.str(), i, UnivOut, pReg + U16_UnivOutMap(i));
        }
    
        /*
         * Swtiched order of creation for m_timerEvent and m_wdTimer.
         *
         * Reason:
         * 		wdTimer thread that is started within wdTimer constructor
         * 		access m_timerEvent. In certian configurations (PSI IFC1210 SBC +RT Linux)
         * 		the wdTimer thread is executed sooner than m_timerEvent is created which
         * 		leads to segementation fault.
         *
         * 	Changed by: tslejko
         * 	Reason: Bug fix
         *
         */

        m_timerEvent = new epicsEvent();
        m_wdTimer = new wdTimer("Watch Dog Timer", this);

        init_cb(&irqStart0_cb, priorityHigh, &evgMrm::process_sos0_cb,
                                            m_seqRamMgr.getSeqRam(0));
        init_cb(&irqStart1_cb, priorityHigh, &evgMrm::process_sos1_cb,
                                            m_seqRamMgr.getSeqRam(1));
        init_cb(&irqStop0_cb, priorityHigh, &evgMrm::process_eos0_cb,
                                            m_seqRamMgr.getSeqRam(0));
        init_cb(&irqStop1_cb, priorityHigh, &evgMrm::process_eos1_cb,
                                            m_seqRamMgr.getSeqRam(1));
        init_cb(&irqExtInp_cb, priorityHigh, &evgMrm::process_inp_cb, this);
    
        scanIoInit(&ioScanTimestamp);
    } catch(std::exception& e) {
        errlogPrintf("Error: %s\n", e.what());
    }
}

evgMrm::~evgMrm() {
    for(int i = 0; i < evgNumEvtTrig; i++)
        delete m_trigEvt[i];

    for(int i = 0; i < evgNumMxc; i++)
        delete m_muxCounter[i];

    for(int i = 0; i < evgNumDbusBit; i++)
        delete m_dbus[i];

    for(int i = 0; i < evgNumFrontInp; i++)
        delete m_input[std::pair<epicsUInt32, InputType>(i, FrontInp)];

    for(int i = 0; i < evgNumUnivInp; i++)
        delete m_input[std::pair<epicsUInt32, InputType>(i, UnivInp)];

    for(int i = 0; i < evgNumRearInp; i++)
        delete m_input[std::pair<epicsUInt32, InputType>(i, RearInp)];

    for(int i = 0; i < evgNumFrontOut; i++)
        delete m_output[std::pair<epicsUInt32, evgOutputType>(i, FrontOut)];

    for(int i = 0; i < evgNumUnivOut; i++)
        delete m_output[std::pair<epicsUInt32, evgOutputType>(i, UnivOut)];
}

void 
evgMrm::init_cb(CALLBACK *ptr, int priority, void(*fn)(CALLBACK*), void* valptr) { 
    callbackSetPriority(priority, ptr); 
    callbackSetCallback(fn, ptr);     
    callbackSetUser(valptr, ptr);     
    (ptr)->timer=NULL;                            
}

const std::string
evgMrm::getId() const {
    return m_id;
}

volatile epicsUInt8*
evgMrm::getRegAddr() const {
    return m_pReg;
}

epicsUInt32 
evgMrm::getFwVersion() const {
    return READ32(m_pReg, FPGAVersion);
}

std::string
evgMrm::getSwVersion() const {
    return MRF_VERSION;
}

epicsUInt32
evgMrm::getDbusStatus() const {
    return READ32(m_pReg, Status);
}

void
evgMrm::enable(bool ena) {
    if(ena)
        BITSET32(m_pReg, Control, EVG_MASTER_ENA);
    else
        BITCLR32(m_pReg, Control, EVG_MASTER_ENA);

    BITSET32(m_pReg, Control, EVG_DIS_EVT_REC);
    BITSET32(m_pReg, Control, EVG_REV_PWD_DOWN);
    BITSET32(m_pReg, Control, EVG_MXC_RESET);
}

bool
evgMrm::enabled() const {
    return (READ32(m_pReg, Control) & EVG_MASTER_ENA) != 0;
}

void
evgMrm::resetMxc(bool reset) {
    if(reset)
        BITSET32(m_pReg, Control, EVG_MXC_RESET);
}

void
evgMrm::isr_pci(void* arg) {
    evgMrm *evg = (evgMrm*)(arg);

    // Call to the generic implementation
    evg->isr(arg);

    /**
     * On PCI veriant of EVG the interrupts get disabled in kernel (by uio_mrf module) since IRQ task is completed here (in userspace).
     * Interrupts must therfore be renabled here.
     *
     * Change by: tslejko
     * Reason: cPCI support
     */
    BITSET32(evg->m_pReg,IrqEnable,EVG_IRQ_ENABLE); //Renable EVG interrupt
}

void
evgMrm::isr_vme(void* arg) {
    evgMrm *evg = (evgMrm*)(arg);

    epicsUInt32 flags = READ32(evg->m_pReg, IrqFlag);
    epicsUInt32 enable = READ32(evg->m_pReg, IrqEnable);
    epicsUInt32 active = flags & enable;

    // This skips extra work with a shared interrupt.
    if(!active)
      return;

    // Call to the generic implementation
    evg->isr(arg);
}

void
evgMrm::isr(void* arg) {
    evgMrm *evg = (evgMrm*)(arg);

    epicsUInt32 flags = READ32(evg->m_pReg, IrqFlag);
    epicsUInt32 enable = READ32(evg->m_pReg, IrqEnable);
    epicsUInt32 active = flags & enable;


    #ifdef vxWorks
    /* actually: if isr runs in kernel mode */

        /*
         * For IFC1210 board this is useless
         * and for SwissFEL_TIM it is dangerous, since
         * in the unlikely event of queuing more than 2 IRQs the system
         * will stop dropping them. For SwissFEL timing system this is
         * unacceptable.
         * Furthermore it is not thread safe!!!
         * A race condition has been observed where
         * x_queued is changed back to 0 without re-enabling interrupts.         *
         * For PSI, this is now handled in the IRQ thread, avg ISR time is
         * around ~10us
         */

     if(active & EVG_IRQ_STOP_RAM(0)) {
         if(evg->irqStop0_queued==0) {
             callbackRequest(&evg->irqStop0_cb);
             evg->irqStop0_queued=1;
         } else if(evg->irqStop0_queued==1) {
             WRITE32(evg->getRegAddr(), IrqEnable, enable & ~EVG_IRQ_STOP_RAM(0));
             evg->irqStop0_queued=2;
         }
     }

     if(active & EVG_IRQ_STOP_RAM(1)) {
         if(evg->irqStop1_queued==0) {
             callbackRequest(&evg->irqStop1_cb);
             evg->irqStop1_queued=1;
         } else if(evg->irqStop1_queued==1) {
             WRITE32(evg->getRegAddr(), IrqEnable, enable & ~EVG_IRQ_STOP_RAM(1));
             evg->irqStop1_queued=2;
         }
     }

    if(active & EVG_IRQ_START_RAM(0)) {
        if(evg->irqStart0_queued==0) {
            callbackRequest(&evg->irqStart0_cb);
            evg->irqStart0_queued=1;
        } else if(evg->irqStart0_queued==1) {
            WRITE32(evg->getRegAddr(), IrqEnable, enable & ~EVG_IRQ_START_RAM(0));
            evg->irqStart0_queued=2;
        }
    }

    if(active & EVG_IRQ_START_RAM(1)) {
        if(evg->irqStart1_queued==0) {
            callbackRequest(&evg->irqStart1_cb);
            evg->irqStart1_queued=1;
        } else if(evg->irqStart1_queued==1) {
            WRITE32(evg->getRegAddr(), IrqEnable, enable & ~EVG_IRQ_START_RAM(1));
            evg->irqStart1_queued=2;
        }
    }

     if(active & EVG_IRQ_EXT_INP) {
         if(evg->irqExtInp_queued==0) {
             callbackRequest(&evg->irqExtInp_cb);
             evg->irqExtInp_queued=1;
         } else if(evg->irqExtInp_queued==1) {
             WRITE32(evg->getRegAddr(), IrqEnable, enable & ~EVG_IRQ_EXT_INP);
             evg->irqExtInp_queued=2;
         }
     }

    #else
    
    /*
     * This is far far far far from proper solution
     * since it blocks IRQ thread. Luckily the whole ISR
     * Executes in ~10us so it's not a big problem.
     * A better solution would be to use epics callback like
     * original driver, but care must be taken in order to
     * avoid race conditions.
     */
    if(active & EVG_IRQ_STOP_RAM(0)) {
        evg->getSeqRamMgr()->getSeqRam(0)->process_eos();
    }

    if(active & EVG_IRQ_STOP_RAM(1)) {
        evg->getSeqRamMgr()->getSeqRam(1)->process_eos();
    }

    if(active & EVG_IRQ_START_RAM(0)) {
        evg->getSeqRamMgr()->getSeqRam(0)->process_sos();
    }

    if(active & EVG_IRQ_START_RAM(1)) {
        evg->getSeqRamMgr()->getSeqRam(1)->process_sos();
    }

    if(active & EVG_IRQ_EXT_INP) {
        if(evg->irqExtInp_queued==0) {
            callbackRequest(&evg->irqExtInp_cb);
            evg->irqExtInp_queued=1;
        } else if(evg->irqExtInp_queued==1) {
            WRITE32(evg->getRegAddr(), IrqEnable, enable & ~EVG_IRQ_EXT_INP);
            evg->irqExtInp_queued=2;
        }
    }
#endif



    WRITE32(evg->m_pReg, IrqFlag, flags);  // Clear the interrupt causes
    READ32(evg->m_pReg, IrqFlag);          // Make sure the clear completes before returning

    return;
}

void
evgMrm::process_eos0_cb(CALLBACK *pCallback) {
    void* pVoid;
    evgSeqRam* seqRam;
    
    callbackGetUser(pVoid, pCallback);
    seqRam = (evgSeqRam*)pVoid;
    if(!seqRam)
        return;

    {
        interruptLock ig;
        if(seqRam->m_owner->irqStop0_queued==2)
            BITSET32(seqRam->m_owner->getRegAddr(), IrqEnable, EVG_IRQ_STOP_RAM(0));
        seqRam->m_owner->irqStop0_queued=0;
    }

    seqRam->process_eos();
}

void
evgMrm::process_eos1_cb(CALLBACK *pCallback) {
    void* pVoid;
    evgSeqRam* seqRam;

    callbackGetUser(pVoid, pCallback);
    seqRam = (evgSeqRam*)pVoid;
    if(!seqRam)
        return;

    {
        interruptLock ig;
        if(seqRam->m_owner->irqStop1_queued==2)
            BITSET32(seqRam->m_owner->getRegAddr(), IrqEnable, EVG_IRQ_STOP_RAM(1));
        seqRam->m_owner->irqStop1_queued=0;
    }

    seqRam->process_eos();
}

void
evgMrm::process_sos0_cb(CALLBACK *pCallback) {
    void* pVoid;
    evgSeqRam* seqRam;

    callbackGetUser(pVoid, pCallback);
    seqRam = (evgSeqRam*)pVoid;
    if(!seqRam)
        return;

    {
        interruptLock ig;
        if(seqRam->m_owner->irqStart0_queued==2)
            BITSET32(seqRam->m_owner->getRegAddr(), IrqEnable, EVG_IRQ_START_RAM(0));
        seqRam->m_owner->irqStart0_queued=0;
    }

    seqRam->process_sos();
}

void
evgMrm::process_sos1_cb(CALLBACK *pCallback) {
    void* pVoid;
    evgSeqRam* seqRam;

    callbackGetUser(pVoid, pCallback);
    seqRam = (evgSeqRam*)pVoid;
    if(!seqRam)
        return;

    {
        interruptLock ig;
        if(seqRam->m_owner->irqStart1_queued==2)
            BITSET32(seqRam->m_owner->getRegAddr(), IrqEnable, EVG_IRQ_START_RAM(1));
        seqRam->m_owner->irqStart1_queued=0;
    }

    seqRam->process_sos();
}

void
evgMrm::process_inp_cb(CALLBACK *pCallback) {
    void* pVoid;
    callbackGetUser(pVoid, pCallback);
    evgMrm* evg = (evgMrm*)pVoid;

    {
        interruptLock ig;
        if(evg->irqExtInp_queued==2)
            BITSET32(evg->getRegAddr(), IrqEnable, EVG_IRQ_EXT_INP);
        evg->irqExtInp_queued=0;
    }
     
    epicsUInt32 data = evg->sendTimestamp();
    if(!data)
        return;

    for(int i = 0; i < 32; data <<= 1, i++) {
        if( data & 0x80000000 )
            evg->getSoftEvt()->setEvtCode(MRF_EVENT_TS_SHIFT_1);
        else
            evg->getSoftEvt()->setEvtCode(MRF_EVENT_TS_SHIFT_0);
    }
}

epicsUInt32
evgMrm::sendTimestamp() {
    /*Start the timer*/
    m_timerEvent->signal();

    /*If the time since last update is more than 1.5 secs(i.e. if wdTimer expires) 
    then we need to resync the time after 5 good pulses*/
    if(m_wdTimer->getPilotCount()) {
        m_wdTimer->decrPilotCount();
        if(m_wdTimer->getPilotCount() == 0) {
            syncTimestamp();
            printf("Starting timestamping\n");
            ((epicsTime)getTimestamp()).show(1);
        }
        return 0;
    }

    m_alarmTimestamp = TS_ALARM_NONE;

    incrTimestamp();
    scanIoRequest(ioScanTimestamp);

    if(m_syncTimestamp) {
        syncTimestamp();
        m_syncTimestamp = false;
    }

    struct epicsTimeStamp ts;
    epicsTime ntpTime, storedTime;
    if(epicsTimeOK == generalTimeGetExceptPriority(&ts, 0, 50)) {
        ntpTime = ts;
        storedTime = (epicsTime)getTimestamp();

        double errorTime = ntpTime - storedTime;

        /*If there is an error between storedTime and ntpTime then we just print
            the relevant information but we send out storedTime*/
        if(fabs(errorTime) > evgAllowedTsGitter) {
            m_alarmTimestamp = TS_ALARM_MINOR;
            printf("NTP time:\n");
            ntpTime.show(1);
            printf("EVG time:\n");
            storedTime.show(1);
            printf("----Timestamping Error of %f Secs----\n", errorTime);
        } 
    }

    return getTimestamp().secPastEpoch + 1 + POSIX_TIME_AT_EPICS_EPOCH;
}

epicsTimeStamp
evgMrm::getTimestamp() const {
    return m_timestamp;
}

void
evgMrm::incrTimestamp() {
    m_timestamp.secPastEpoch++;
}

void
evgMrm::syncTimestamp() {
    if(epicsTimeOK != generalTimeGetExceptPriority(&m_timestamp, 0, 50))
        return;
    /*
     * Generally nano seconds should be close to zero.
     *  So the seconds value should be rounded to the nearest interger
     *  e.g. 26.000001000 should be rounded to 26 and
     *       26.996234643 should be rounded to 27.
     *  Also the nano second value can be assumed to be zero.
     */
    if(m_timestamp.nsec > 500*pow(10.0,6))
        incrTimestamp();
    
    m_timestamp.nsec = 0;
}

void
evgMrm::syncTsRequest() {
    m_syncTimestamp = true;
}

/**    Access    functions     **/

evgEvtClk*
evgMrm::getEvtClk() {
    return &m_evtClk;
}

evgAcTrig*
evgMrm::getAcTrig() {
    return &m_acTrig;
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
    evgMxc* mxc =    m_muxCounter[muxNum];
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
evgMrm::getOutput(epicsUInt32 outNum, evgOutputType type) {
    evgOutput* out = m_output[ std::pair<epicsUInt32, evgOutputType>(outNum, type) ];
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

epicsEvent* 
evgMrm::getTimerEvent() {
    return m_timerEvent;
}

namespace {
    struct showSoftSeq {int lvl; void operator()(evgSoftSeq* seq){seq->show(lvl);}};
}

void evgMrm::show(int lvl)
{
    showSoftSeq ss;
    ss.lvl = lvl;
    m_softSeqMgr.visit(ss);
}
