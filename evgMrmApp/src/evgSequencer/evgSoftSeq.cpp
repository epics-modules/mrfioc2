/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#include "evgSoftSeq.h"

#include <math.h>
#include <stdio.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>

#include <errlog.h>

#include <mrfCommonIO.h>
#include <mrfCommon.h>
#include "evgRegMap.h"

#include "evgMrm.h"

int mrmEVGSeqDebug = 0;

static epicsUInt32 next_seq_id;

// capture error message, then rethrow so that underlying dset will alarm
#define RETHROW() catch(std::exception& e) { m_err = e.what(); scanIoRequest(ioScanPvtErr); throw; }

evgSoftSeq::evgSoftSeq(const std::string &id, evgMrm* const owner)
    :base_t(id),
m_lock(),
m_id(next_seq_id++),
m_owner(owner),
m_pReg(owner->getRegAddr()),
m_timestampInpMode(EGU),
m_runMode(Single),
m_runModeCt(Single),
m_seqRam(0),
m_seqRamMgr(owner->getSeqRamMgr()),
m_isEnabled(0),
m_isCommited(0),
m_isSynced(0),
m_numOfRuns(0) {
    m_eventCodeCt.push_back(0x7f);
    m_timestampCt.push_back(evgEndOfSeqBuf);

    setTrigSrc(SEQ_SRC_DISABLE);
    m_trigSrcCt = m_trigSrc;

    scanIoInit(&ioscanpvt);
    scanIoInit(&ioScanPvtErr);
    scanIoInit(&iorunscan);
    scanIoInit(&iostartscan);
}

evgSoftSeq::~evgSoftSeq() {}

epicsUInt32
evgSoftSeq::getId() const {
    return m_id;
}

std::string evgSoftSeq::getErr() const {
    return m_err;	
}

void
evgSoftSeq::setTimestampInpMode(epicsUInt32 mode) {
    switch(mode) {
    case EGU:
    case TICKS:
        break;
    default:
        throw std::invalid_argument("Invalid Timestamp input mode");
    }
    m_timestampInpMode = (TimestampInpMode)mode;
    scanIoRequest(ioscanpvt);
}

epicsUInt32
evgSoftSeq::getTimestampInpMode() const {
    return m_timestampInpMode;
}

void
evgSoftSeq::setTimestampResolution(epicsUInt32 res) {
    switch(res) {
    case Seconds:
    case MilliSeconds:
    case MicroSeconds:
    case NanoSeconds:
        break;
    default:
        throw std::invalid_argument("Invalid Timestamp input resolution");
    }
    m_timestampResolution = (TimestampResolution)res;
    scanIoRequest(ioscanpvt);
}

epicsUInt32
evgSoftSeq::getTimestampResolution() const {
    return m_timestampResolution;
}

void
evgSoftSeq::setTimestamp(const double* vals, epicsUInt32 size) {
    if(size > 2047)
        throw std::runtime_error("Too many Timestamps. Max: 2047");
    try {
        double scale = 1.0;

        if(m_timestampInpMode!=TICKS) {
            epicsFloat64 evtClk = m_owner->getEvtClk()->getFrequency() * pow(10.0,6);
            scale = pow(10.0, (int)m_timestampResolution) / evtClk;
        }

        m_timestamp.resize(size);
        for(epicsUInt32 i=0; i<size; i++)
            m_timestamp[i] = vals[i] / scale;
    } RETHROW()
}

epicsUInt32
evgSoftSeq::getTimestamp(double* vals, epicsUInt32 max) const
{
    epicsUInt32 N = std::min(size_t(max), m_timestampCt.size());
    double scale = 1.0;

    if(m_timestampInpMode!=TICKS) {
        epicsFloat64 evtClk = m_owner->getEvtClk()->getFrequency() * pow(10.0,6);
        scale = pow(10.0, (int)m_timestampResolution) / evtClk;
    }

    for(epicsUInt32 i=0; i<N; i++)
        vals[i] = m_timestampCt[i] * scale;
    return N;
}

void
evgSoftSeq::setEventCode(const epicsUInt8 *eventCode, epicsUInt32 size) {
try {
    if(size > 2047)
        throw std::runtime_error("Too many EventCodes. Max: 2047");
	
    m_eventCode.clear();
    m_eventCode.assign(eventCode, eventCode + size);

    m_isCommited = false;
    if(mrmEVGSeqDebug>1)
        fprintf(stderr, "SS%u: Update Evt\n",m_id);
    scanIoRequest(ioscanpvt);
} RETHROW()
}

epicsUInt32
evgSoftSeq::getEventCode(epicsUInt8* vals, epicsUInt32 max) const
{
    epicsUInt32 N = std::min(size_t(max), m_eventCodeCt.size());
    std::copy(m_eventCodeCt.begin(),
              m_eventCodeCt.begin()+N,
              vals);
    return N;
}

void
evgSoftSeq::setTrigSrc(epicsUInt32 trigSrc) {
    if(trigSrc>SEQ_SRC_SW)
        throw std::invalid_argument("Trigger source out of range");
try {
    if(trigSrc != m_trigSrc) {
        m_trigSrc = trigSrc;
        m_isCommited = false;
        if(mrmEVGSeqDebug>1)
            fprintf(stderr, "SS%u: Update Trig\n",m_id);
        scanIoRequest(ioscanpvt);
    }
} RETHROW()
}

void
evgSoftSeq::setRunMode(epicsUInt32 runMode) {
    switch(runMode) {
    case Normal:
    case Auto:
    case Single:
        break;
    default:
        throw std::invalid_argument("Invalid run mode");
    }

try {
    if((SeqRunMode)runMode != m_runMode) {
        m_runMode = (SeqRunMode)runMode;
        m_isCommited = false;
        if(mrmEVGSeqDebug>1)
            fprintf(stderr, "SS%u: Update Mode\n",m_id);
        scanIoRequest(ioscanpvt);
    }
} RETHROW()
}

/*
 * Retrive commited(Ct) data.
 */

epicsUInt32 evgSoftSeq::getTrigSrcCt() const {
    return m_trigSrcCt;
}

epicsUInt32
evgSoftSeq::getRunModeCt() const {
    return m_runModeCt;
}

void
evgSoftSeq::setSeqRam(evgSeqRam* seqRam) {
    interruptLock ig;
    m_seqRam = seqRam;
}

evgSeqRam* 
evgSoftSeq::getSeqRam() {
    interruptLock ig;
    return m_seqRam;
}

bool
evgSoftSeq::isLoaded() const {
    return m_seqRam != 0;
}

bool
evgSoftSeq::isEnabled() const {
    return m_isEnabled;
}

bool
evgSoftSeq::isCommited() const {
    return m_isCommited;
}

void
evgSoftSeq::load() {
try {
    if(isLoaded())
        return;

    /* 
     * Assign the first avialable(unallocated) hardware sequenceRam to this soft
     * sequence if none is currently assingned.
     */
    for(unsigned int i = 0; i < m_seqRamMgr->numOfRams(); i++) {
        evgSeqRam* seqRamIter = m_seqRamMgr->getSeqRam(i);
        if( seqRamIter->alloc(this) )
            break;
    }

    if(isLoaded()) {
        // always need sync after loading
        m_isSynced = false;
        if(mrmEVGSeqDebug)
            fprintf(stderr, "SS%u: Load\n",m_id);
        sync(); 
        scanIoRequest(ioscanpvt);
    } else {
        char err[80];
        sprintf(err, "No SeqRam to load SoftSeq %d", getId());
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }
} RETHROW()
}
	
void
evgSoftSeq::unload() {
try {
    if(!isLoaded())
        return;

    // ensure we will stop soon
    m_seqRam->setRunMode(Single);
    m_seqRam->setTrigSrc(SEQ_SRC_DISABLE);
    {
        interruptLock ig;
        m_seqRam->dealloc();
        setSeqRam(0);
    }
    // always out of sync when not loaded
    m_isSynced = false;
    if(mrmEVGSeqDebug)
        fprintf(stderr, "SS%u: Unload\n",m_id);
    scanIoRequest(ioscanpvt);
} RETHROW()
}


void
evgSoftSeq::commit() {
try {
    if(isCommited())
        return;
    if(mrmEVGSeqDebug>1)
        fprintf(stderr, "SS%u: Request Commit\n",m_id);
    commitSoftSeq();

    if(isLoaded()) {
        sync();
        if(mrmEVGSeqDebug)
            fprintf(stderr, "SS%u: Committed\n",m_id);
    } else {
        if(mrmEVGSeqDebug)
            fprintf(stderr, "SS%u: Committed (Not loaded)\n",m_id);
    }

    scanIoRequest(ioscanpvt);
} RETHROW()
}

void
evgSoftSeq::enable() {
try {
    if(isEnabled())
        return;

    if(isLoaded()) {
        /*
         * RunMode and TrigSrc could be modified in the hardware. So it is 
         * necessary to sync them before enabling the sequence.
         */
        m_seqRam->setTrigSrc(m_trigSrcCt);
        m_seqRam->setRunMode(m_runModeCt);
        m_seqRam->enable();
    }

    m_isEnabled = true;
    if(mrmEVGSeqDebug)
        fprintf(stderr, "SS%u: Enable\n",m_id);
    scanIoRequest(ioscanpvt);
} RETHROW()
}

void
evgSoftSeq::disable() {
try {
    if(!isEnabled())
        return;

    if(m_seqRam) {
        m_seqRam->setRunMode(Single);
        m_seqRam->setTrigSrc(SEQ_SRC_DISABLE);
    }

    m_isEnabled = false;
    if(mrmEVGSeqDebug)
        fprintf(stderr, "SS%u: Disable\n",m_id);
    scanIoRequest(ioscanpvt);
} RETHROW()
}

void
evgSoftSeq::softTrig()
{
    evgSeqRam *ram = getSeqRam();
    if(!ram) {
        if(mrmEVGSeqDebug>1)
            fprintf(stderr, "SS%u: No SW Trig (Not loaded)\n",m_id);
        throw alarm_exception(INVALID_ALARM);
    }
    try {
        if(mrmEVGSeqDebug>1)
            fprintf(stderr, "SS%u: SW Trig\n",m_id);
        ram->softTrig();
    }RETHROW()
}

void
evgSoftSeq::sync() {
    if(!isLoaded() || m_isSynced)
        return;

    // Ensure the sequencer will stop at some point
    m_seqRam->setRunMode(Single);
    // Ensure the sequencer won't start if it hasn't already
    m_seqRam->setTrigSrc(SEQ_SRC_DISABLE);

    // At this point, if the sequencer is not running
    // then we know that it will never run.
    // If it is running, then we must wait for the
    // EOS interrupt.
    // It is possible that the sequencer has stopped,
    // but the callback for the EOS interrupt has not been
    // delivered.  This would result in finishSync()
    // being called twice, however this is prevented
    // by the m_isSynced flag.

    if(!m_seqRam->isRunning()) {
        m_seqRam->disable();
        finishSync(); // or finish immediately
    } else {
        if(mrmEVGSeqDebug>1)
            fprintf(stderr, "SS%u: Start sync\n",m_id);
        return; // wait for EOS
    }
}

void
evgSoftSeq::finishSync()
{
    if(mrmEVGSeqDebug>1) {
        fprintf(stderr, "Syncing...\n Src: %d\n Mode: %d\n",
                (int)getTrigSrcCt(), (int)getRunModeCt());
    }
    m_seqRam->setEventCode(m_eventCodeCt);
    m_seqRam->setTimestamp(m_timestampCt);
    m_seqRam->setTrigSrc(m_trigSrcCt);
    m_seqRam->setRunMode(m_runModeCt);
    if(m_isEnabled) {
        m_seqRam->enable();
        if(mrmEVGSeqDebug>1)
            fprintf(stderr, "SS%u: Enabling...\n",m_id);
    }

    m_isSynced = true;
    if(mrmEVGSeqDebug>1)
        fprintf(stderr, "SS%u: Finish sync\n",m_id);
    scanIoRequest(ioscanpvt);
}

void
evgSoftSeq::commitSoftSeq() {
    epicsUInt64 preTs = 0;

    std::vector<epicsUInt64> timestamp;
    std::vector<epicsUInt8> eventCode;

    // reserve (allocate) for worst case
    timestamp.reserve(2048);
    eventCode.reserve(2048);

    /*
     * Make EventCode and Timestamp vector of same size
     *(Smaller of the two vectors sizes).
     */
    std::vector<epicsUInt64>::iterator itTS = m_timestamp.begin();
    std::vector<epicsUInt8>::iterator itEC = m_eventCode.begin();
    for(; itTS < m_timestamp.end() && itEC < m_eventCode.end(); itTS++, itEC++) {
        const epicsUInt8 ecUInt8 = *itEC;

        const epicsUInt64 curTs = *itTS;
        int64_t tsUInt64 = curTs - preTs; /* relative ticks since last input event */
        if(timestamp.size())
            tsUInt64 += timestamp.back(); /* abs. output ticks wrt. start or last continuation */

        /* inject continuation event(s) when output time would overflow */
        for(;tsUInt64 > 0xffffffff; tsUInt64 -= 0xffffffff) {
            timestamp.push_back(0xffffffff);
            eventCode.push_back(0);
        }
        preTs = curTs;

        timestamp.push_back(tsUInt64);
        eventCode.push_back(ecUInt8);
        if(ecUInt8==0x7f)
            break; /* User provided end of sequence event */
    }

    /*
     * Check if the timestamps are sorted and unique 
     */
    if(timestamp.size() > 1) {
        for(unsigned int i = 0; i < timestamp.size()-1; i++) {
            if( timestamp[i] >= timestamp[i+1] ) {
                if( (timestamp[i] == 0xffffffff) && (eventCode[i] == 0))
                    continue;
                else
                    throw std::runtime_error("Sequencer timestamps are not Sorted/Unique");
            } 
        }
    }

    if(eventCode.size()==0 && timestamp.size()==0) {
        /* empty sequence.  Not very useful, but not an error */
        eventCode.push_back(0x7f);
        timestamp.push_back(evgEndOfSeqBuf);

    } if(timestamp.size()!=eventCode.size()) {
        throw std::logic_error("SoftSeq, length of timestamp and eventCode don't match");

    } else if(timestamp.size()>2047) {
        throw std::runtime_error("Sequence too long (>2047)");

    } else if(eventCode.back()!=0x7f) {
        /*
         * If not already present append 'End of Sequence' event code(0x7f) and
         * timestamp.
         */
        if(timestamp.back()+evgEndOfSeqBuf>=0xffffffff) {
            eventCode.push_back(0);
            timestamp.push_back(0xffffffff);
            eventCode.push_back(0x7f);
            timestamp.push_back(evgEndOfSeqBuf);
        } else {
            eventCode.push_back(0x7f);
            timestamp.push_back(timestamp.back() + evgEndOfSeqBuf);
        }
    }



    m_timestampCt = timestamp;
    m_eventCodeCt = eventCode;
    m_trigSrcCt = m_trigSrc;
    m_runModeCt = m_runMode;

    m_isCommited = true;
    m_isSynced = false;

    if(mrmEVGSeqDebug>1)
        fprintf(stderr, "SS%u: Commit complete, need sync\n",m_id);
}

void
evgSoftSeq::process_sos()
{
    scanIoRequest(iostartscan);
}

void
evgSoftSeq::process_eos()
{
    m_numOfRuns++;

    // In single shot mode, auto-disable after
    // each run.
    if(m_runModeCt==Single && m_isEnabled)
        disable();

    if(isLoaded() && !m_isSynced)
        finishSync();

    scanIoRequest(iorunscan);
}

void evgSoftSeq::show(int lvl)
{
    if(lvl<1)
        return;

    fprintf(stderr, "SoftSeq %d\n", m_id);
    epicsUInt32 rid = (epicsUInt32)-1;
    evgSeqRam *ram;
    {
        interruptLock ig;
        ram=getSeqRam();
        if(ram)
            rid = ram->getId();
    }
    fprintf(stderr, " Loaded: %s (%u)\n", ram ? "Yes": "No", rid);
    fprintf(stderr, " Enabled: %d\n Committed: %d\n Synced: %d\n",
            isEnabled(), isCommited(), m_isSynced);
}

mrf::Object*
evgSoftSeq::build(const std::string& name, const std::string& klass, const create_args_t& args)
{
    create_args_t::const_iterator it=args.find("PARENT");
    if(it==args.end())
        throw std::runtime_error("No PARENT= (EVG) specified");

    mrf::Object *evgobj = mrf::Object::getObject(it->second);
    if(!evgobj)
        throw std::runtime_error("No such PARENT object");

    evgMrm *evg = dynamic_cast<evgMrm*>(evgobj);
    if(!evg)
        throw std::runtime_error("PARENT is not an EVG");

    return new evgSoftSeq(name, evg);
}

OBJECT_BEGIN(evgSoftSeq)
  OBJECT_FACTORY(evgSoftSeq::build);
  OBJECT_PROP1("ID", &evgSoftSeq::getId);
  OBJECT_PROP1("ERROR", &evgSoftSeq::getErr);
  OBJECT_PROP1("ERROR", &evgSoftSeq::getErrScan);
  OBJECT_PROP1("LOADED", &evgSoftSeq::isLoaded);
  OBJECT_PROP1("LOADED", &evgSoftSeq::stateChange);
  OBJECT_PROP1("LOAD", &evgSoftSeq::load);
  OBJECT_PROP1("UNLOAD", &evgSoftSeq::unload);
  OBJECT_PROP1("ENABLED", &evgSoftSeq::isEnabled);
  OBJECT_PROP1("ENABLED", &evgSoftSeq::stateChange);
  OBJECT_PROP1("ENABLE", &evgSoftSeq::enable);
  OBJECT_PROP1("DISABLE", &evgSoftSeq::disable);
  OBJECT_PROP1("COMMITTED", &evgSoftSeq::isCommited);
  OBJECT_PROP1("COMMITTED", &evgSoftSeq::stateChange);
  OBJECT_PROP1("COMMIT", &evgSoftSeq::commit);
  OBJECT_PROP1("SOFT_TRIG", &evgSoftSeq::softTrig);
  OBJECT_PROP2("TIMES", &evgSoftSeq::getTimestamp, &evgSoftSeq::setTimestamp);
  OBJECT_PROP1("TIMES", &evgSoftSeq::stateChange);
  OBJECT_PROP2("CODES", &evgSoftSeq::getEventCode, &evgSoftSeq::setEventCode);
  OBJECT_PROP1("CODES", &evgSoftSeq::stateChange);
  OBJECT_PROP1("NUM_RUNS", &evgSoftSeq::getNumOfRuns);
  OBJECT_PROP1("NUM_RUNS", &evgSoftSeq::getNumOfRunsScan);
  OBJECT_PROP1("NUM_STARTS", &evgSoftSeq::getNumOfStarts);
  OBJECT_PROP1("NUM_STARTS", &evgSoftSeq::getNumOfStartsScan);
  OBJECT_PROP2("TIMEMODE", &evgSoftSeq::getTimestampInpMode, &evgSoftSeq::setTimestampInpMode);
  OBJECT_PROP2("TIMEUNITS", &evgSoftSeq::getTimestampResolution, &evgSoftSeq::setTimestampResolution);
  OBJECT_PROP2("TRIG_SRC", &evgSoftSeq::getTrigSrcCt, &evgSoftSeq::setTrigSrc);
  OBJECT_PROP1("TRIG_SRC", &evgSoftSeq::stateChange);
  OBJECT_PROP2("RUN_MODE", &evgSoftSeq::getRunModeCt, &evgSoftSeq::setRunMode);
  OBJECT_PROP1("RUN_MODE", &evgSoftSeq::stateChange);
OBJECT_END(evgSoftSeq)

#include <epicsExport.h>
extern "C"{
 epicsExportAddress(int,mrmEVGSeqDebug);
}
