/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef EVG_SEQUENCE_H
#define EVG_SEQUENCE_H

#include <vector>
#include <string>

#include <epicsTypes.h>
#include <epicsMutex.h>
#include <dbAccess.h>
#include <dbScan.h>

#include <mrfCommon.h>
#include "mrf/object.h"

class evgMrm;
class evgSeqRam;
class evgSeqRamMgr;

enum TimestampInpMode {
    EGU = 0,
    TICKS
};

enum TimestampResolution {
    Seconds = 0,
    MilliSeconds = 3,
    MicroSeconds = 6,
    NanoSeconds = 9
};

enum SeqRunMode {
    Normal = 0,
    Auto,
    Single
};

/* special "magic" trigger sources which are different
 * in EVG vs. EVR
 */
#define SEQ_SRC_DISABLE 256
#define SEQ_SRC_SW 257

#define IrqStop(id) irqStop##id

class evgSoftSeq : public mrf::ObjectInst<evgSoftSeq> {
    typedef mrf::ObjectInst<evgSoftSeq> base_t;
public:
    evgSoftSeq(const std::string&, evgMrm* const);
    ~evgSoftSeq();

    static Object* build(const std::string& name, const std::string& klass, const create_args_t& args);

    epicsUInt32 getId() const;

    std::string getErr() const;
    IOSCANPVT  getErrScan() const { return ioScanPvtErr; }

    void setTimestampInpMode(epicsUInt32 mode);
    epicsUInt32 getTimestampInpMode() const;

    void setTimestampResolution(epicsUInt32 res);
    epicsUInt32 getTimestampResolution() const;

    void setTimestamp(const double *, epicsUInt32);
    epicsUInt32 getTimestamp(double*, epicsUInt32) const;

    void setEventCode(const epicsUInt8*, epicsUInt32);
    epicsUInt32 getEventCode(epicsUInt8*, epicsUInt32) const;

    void setTrigSrc(epicsUInt32);
    epicsUInt32 getTrigSrcCt() const;

    void setRunMode(epicsUInt32);
    epicsUInt32 getRunModeCt() const;

    void setSeqRam(evgSeqRam*);
    evgSeqRam* getSeqRam();

    bool isLoaded() const;
    bool isEnabled() const;
    bool isCommited() const;
    IOSCANPVT stateChange() const { return ioscanpvt; }

    void load();
    void unload();
    void commit();
    void enable();
    void disable();
    void softTrig();

    void sync();
    void finishSync();
    void commitSoftSeq();

    void process_sos();
    void process_eos();

    epicsUInt32 getNumOfRuns() const { return m_numOfRuns; }
    epicsUInt32 getNumOfStarts() const { return m_numOfStarts; }
    IOSCANPVT getNumOfRunsScan() const { return iorunscan; }
    IOSCANPVT getNumOfStartsScan() const { return iostartscan; }

    void show(int);

    IOSCANPVT                  ioscanpvt;
    IOSCANPVT                  ioScanPvtErr;
    IOSCANPVT                  iostartscan;
    IOSCANPVT                  iorunscan;
    mutable epicsMutex         m_lock;

    virtual void lock() const {m_lock.lock();}
    virtual void unlock() const {m_lock.unlock();}

private:
    const epicsUInt32          m_id;
    evgMrm* const              m_owner;
    volatile epicsUInt8* const m_pReg;
    std::string                m_err;

    TimestampInpMode           m_timestampInpMode;
    TimestampResolution        m_timestampResolution;

    // scratch copy
    std::vector<epicsUInt64>   m_timestamp; //In Event Clock Ticks
    std::vector<epicsUInt8>    m_eventCode;
    epicsUInt32                m_trigSrc;
    SeqRunMode                 m_runMode;   

    // commited copy
    std::vector<epicsUInt64>   m_timestampCt;
    std::vector<epicsUInt8>    m_eventCodeCt;
    epicsUInt32                m_trigSrcCt;
    SeqRunMode                 m_runModeCt;

    evgSeqRam*                 m_seqRam;
    evgSeqRamMgr*              m_seqRamMgr; 

    bool                       m_isEnabled;
    bool                       m_isCommited;
    bool                       m_isSynced;

    epicsUInt32                m_numOfRuns;
    epicsUInt32                m_numOfStarts;
};

extern int mrmEVGSeqDebug;

#endif //EVG_SEQUENCE_H

