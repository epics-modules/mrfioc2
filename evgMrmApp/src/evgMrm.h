/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef EVG_MRM_H
#define EVG_MRM_H

#include <vector>
#include <map>
#include <string>

#include <stdio.h>
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

#include <devLibPCI.h>

#include "evgAcTrig.h"
#include "evgEvtClk.h"
#include "evgTrigEvt.h"
#include "evgMxc.h"
#include "evgDbus.h"
#include "evgInput.h"
#include "evgOutput.h"
#include "mrmDataBufTx.h"
#include "mrmtimesrc.h"
#include "mrmevgseq.h"
#include "mrmspi.h"
#include "configurationInfo.h"
#include "drvem.h"

/*********
 * Each EVG will be represented by the instance of class 'evgMrm'. Each evg
 * object maintains a list to all the evg sub-componets i.e. Event clock,
 * Software Events, Trigger Events, Distributed bus, Multiplex Counters,
 * Input, Output etc.
 */
class wdTimer;
class wdTimer1;

class FCT;

enum ALARM_TS {TS_ALARM_NONE, TS_ALARM_MINOR, TS_ALARM_MAJOR};

class evgMrm : public mrf::ObjectInst<evgMrm>,
               public TimeStampSource,
               public MRMSPI
{
public:
    struct Config {
        const char *model;
        unsigned numFrontInp,
                 numUnivInp,
                 numRearInp;
    };

    evgMrm(const std::string& id,
           const Config *conf,
           bus_configuration& busConfig,
           volatile epicsUInt8* const base,
           const epicsPCIDevice* pciDevice);
    ~evgMrm();

    void enableIRQ();

    /* locking done internally: TODO: not really... */
    virtual void lock() const{};
    virtual void unlock() const{};
    epicsMutex m_lock;

    /** EVG    **/
    const std::string getId() const;
    volatile epicsUInt8* getRegAddr() const;
    MRFVersion version() const;
    std::string getFwVersionStr() const;
    std::string getSwVersion() const;
    std::string getCommitHash() const;

    void enable(epicsUInt16);
    epicsUInt16 enabled() const;

    bool getResetMxc() const {return true;}
    void resetMxc(bool reset);
    epicsUInt32 getDbusStatus() const;

    IOSCANPVT timeErrorScan() const { return ioScanTimestamp; }

    virtual void postSoftSecondsSrc();

    // event clock
    epicsFloat64 getFrequency() const;

    void setRFFreq(epicsFloat64);
    epicsFloat64 getRFFreq() const;

    void setRFDiv(epicsUInt32);
    epicsUInt32 getRFDiv() const;

    void setFracSynFreq(epicsFloat64);
    epicsFloat64 getFracSynFreq() const;

    // see ClockCtrl[RFSEL]
    enum ClkSrc {
        ClkSrcInternal=0,
        ClkSrcRF=1,
        ClkSrcPXIe100=2,
        ClkSrcRecovered=4, // fanout mode
        ClkSrcSplit=5, // split, external on downstream, recovered on upstream
        ClkSrcPXIe10=6,
        ClkSrcRecovered_2=7,
    };
    void setSource(epicsUInt16);
    epicsUInt16 getSource() const;

    bool pllLocked() const;

    /**    Interrupt and Callback    **/
    static void isr(evgMrm *evg, bool pci);
    static void isr_pci(void*);
    static void isr_vme(void*);
    static void isr_poll(void*);
    static void init_cb(CALLBACK*, int, void(*)(CALLBACK*), void*);
    static void process_inp_cb(CALLBACK*);

    void setEvtCode(epicsUInt32);

    // use w/ Object properties for which no getter is necessary
    epicsUInt32 writeonly() const { return 0; }

    /**    Access    functions     **/
    evgInput* getInput(epicsUInt32, InputType);
    epicsEvent* getTimerEvent();
    const bus_configuration* getBusConfiguration();

    CALLBACK                      irqExtInp_cb;

    unsigned char irqExtInp_queued;

    IOSCANPVT                     ioScanTimestamp;

    mrmDataBufTx                  m_buftx;

    void show(int lvl);

    const epicsPCIDevice*         m_pciDevice;

private:
    const std::string             m_id;
    volatile epicsUInt8* const    m_pReg;
    const bus_configuration       busConfiguration;

    epicsFloat64               m_RFref;       // In MHz
    epicsFloat64               m_fracSynFreq; // In MHz
    unsigned                   m_RFDiv;
    ClkSrc                     m_ClkSrc;
    void recalcRFDiv();

    EvgSeqManager                 m_seq;

    evgAcTrig                     m_acTrig;

    typedef std::vector<evgTrigEvt*> TrigEvt_t;
    TrigEvt_t                     m_trigEvt;

    typedef std::vector<evgMxc*> MuxCounter_t;
    MuxCounter_t                  m_muxCounter;

    typedef std::vector<evgDbus*> Dbus_t;
    Dbus_t                        m_dbus;

    typedef std::map< std::pair<epicsUInt32, InputType>, evgInput*> Input_t;
    Input_t                       m_input;

public:
    typedef Input_t::iterator inputs_iterator;
    inputs_iterator beginInputs() { return m_input.begin(); }
    inputs_iterator endInputs() { return m_input.end(); }
private:

    typedef std::map< std::pair<epicsUInt32, evgOutputType>, evgOutput*> Output_t;
    Output_t                      m_output;

    epicsEvent                    m_timerEvent;

    epicsUInt32                   shadowIrqEnable;

    // EVM only
    mrf::auto_ptr<FCT> fct;
    mrf::auto_ptr<EVRMRM> evru, evrd;
public:
    EVRMRM* getEvruMrm() const { return evru.get(); } // EVRU MRM accessor
    EVRMRM* getEvrdMrm() const { return evrd.get(); } // EVRD MRM accessor
};

#endif //EVG_MRM_H
