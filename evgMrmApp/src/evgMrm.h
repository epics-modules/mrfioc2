#ifndef EVG_MRM_H
#define EVG_MRM_H

#include <vector>
#include <map>
#include <string>

#include <stdio.h>
#include <epicsTypes.h>
#include <dbScan.h>
#include <callback.h>
#include <epicsMutex.h>
#include <epicsTimer.h>
#include <errlog.h>
#include <epicsTime.h>
#include <generalTimeSup.h>
#include <epicsThread.h>
#include <epicsEvent.h>
#include <epicsMutex.h>

#include "evgAcTrig.h"
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
#include "evgRegMap.h"

/*********
 * Each EVG will be represented by the instance of class 'evgMrm'. Each evg 
 * object maintains a list to all the evg sub-componets i.e. Event clock,
 * Software Events, Trigger Events, Distributed bus, Multiplex Counters, 
 * Input, Output etc.
 */
class wdTimer;
class wdTimer1;

enum ALARM_TS {TS_ALARM_NONE, TS_ALARM_MINOR, TS_ALARM_MAJOR};

class evgMrm {
public:
    evgMrm(const epicsUInt32 CardNum, volatile epicsUInt8* const pReg);
    ~evgMrm();

    /** EVG    **/
    const epicsUInt32 getId();    
    volatile epicsUInt8* getRegAddr(); 
    epicsUInt32 getFwVersion();
    epicsStatus enable(bool ena);
    epicsStatus resetMxc(bool reset);
    epicsUInt32 getDbusStatus();

    /**    Interrupt and Callback    **/
    static void isr(void*);
    static void init_cb(CALLBACK*, int, void(*)(CALLBACK*), void*);
    static void process_cb(CALLBACK*);
    static void process_inp_cb(CALLBACK*);

    /** TimeStamp    **/
    epicsUInt32 sendTS();
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
    
    /**    Access    functions     **/
    evgAcTrig* getAcTrig();
    evgEvtClk* getEvtClk();
    evgSoftEvt* getSoftEvt();
    evgTrigEvt* getTrigEvt(epicsUInt32);
    evgMxc* getMuxCounter(epicsUInt32);
    evgDbus* getDbus(epicsUInt32);
    evgInput* getInput(epicsUInt32, InputType);
    evgOutput* getOutput(epicsUInt32, OutputType);
    evgSeqRamMgr* getSeqRamMgr();    
    evgSoftSeqMgr* getSoftSeqMgr();
    epicsEvent* getTimerEvent();

    struct ppsSrc{
        InputType type;
        epicsUInt32 num;    
    };

    CALLBACK                      irqStop0_cb;
    CALLBACK                      irqStop1_cb;
    CALLBACK                      irqExtInp_cb;

    IOSCANPVT                     ioScanTS;
    bool                          m_syncTS;
    ALARM_TS                      m_alarmTS;

    mrmDataBufTx                  m_buftx;

private:
    const epicsUInt32             m_id;             
    volatile epicsUInt8* const    m_pReg;

    evgAcTrig                     m_acTrig;
    evgEvtClk                     m_evtClk;
    evgSoftEvt                    m_softEvt;

    typedef std::vector<evgTrigEvt*> TrigEvt_t;
    TrigEvt_t                     m_trigEvt;

    typedef std::vector<evgMxc*> MuxCounter_t;
    MuxCounter_t                  m_muxCounter;

    typedef std::vector<evgDbus*> Dbus_t;
    Dbus_t                        m_dbus;

    typedef std::map< std::pair<epicsUInt32, InputType>, evgInput*> Input_t;
    Input_t                       m_input;

    typedef std::map< std::pair<epicsUInt32, OutputType>, evgOutput*> Output_t;
    Output_t                      m_output;

    evgSeqRamMgr                  m_seqRamMgr;
    evgSoftSeqMgr                 m_softSeqMgr;

    epicsTimeStamp                m_ts;
    struct ppsSrc                 m_ppsSrc; 

    wdTimer*                      m_wdTimer;
    epicsEvent*                   m_timerEvent;
};

/*Creating a timer thread bcz epicsTimer uses epicsGeneralTime and when
  generalTime stops working even the timer stop working*/
class wdTimer : public epicsThreadRunable {
public:
    wdTimer(const char *name, evgMrm* evg):
    m_lock(),
    m_thread(*this,name,epicsThreadGetStackSize(epicsThreadStackSmall),50),
    m_evg(evg),
    m_pilotCount(4) {
        m_thread.start();
    }

    virtual void run() {
        struct epicsTimeStamp ts;
        bool timeout;

         while(1) {
             m_lock.lock();
             m_pilotCount = 4;
             m_lock.unlock();
             timeout = false;
             m_evg->getTimerEvent()->wait();

             /*Start of timer. If timeout == true then the timer expired. 
              If timeout == false then received the signal before the timeout period*/
             while(!timeout)
                 timeout = !m_evg->getTimerEvent()->wait(1 + evgAllowedTsGitter);
    
             if(epicsTimeOK == generalTimeGetExceptPriority(&ts, 0, 50)) {
                 printf("Timestamping timeout\n");
                 ((epicsTime)ts).show(1);
             }

             m_evg->m_alarmTS = TS_ALARM_MAJOR;
             scanIoRequest(m_evg->ioScanTS);
         }     
    }

    void decrPilotCount() {
        SCOPED_LOCK(m_lock);
        m_pilotCount--;
        return;
    }

    bool getPilotCount() {
        SCOPED_LOCK(m_lock);
        return m_pilotCount;
    }

    epicsMutex  m_lock;
private:
    epicsThread m_thread;
    evgMrm*     m_evg;
    epicsInt32  m_pilotCount;
};

#endif //EVG_MRM_H
