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

evgSoftSeq::evgSoftSeq(const epicsUInt32 id, evgMrm* const owner):
m_lock(),
m_id(id),
m_owner(owner),
m_pReg(owner->getRegAddr()),
m_timestampInpMode(EGU),
m_trigSrc(None),
m_runMode(Single),
m_trigSrcCt(None),
m_runModeCt(Single),
m_seqRam(0),
m_seqRamMgr(owner->getSeqRamMgr()),
m_isEnabled(0),
m_isCommited(0),
m_isSynced(0),
m_numOfRuns(0) {
    m_eventCodeCt.push_back(0x7f);
    m_timestampCt.push_back(evgEndOfSeqBuf);

    scanIoInit(&ioscanpvt);
    scanIoInit(&ioScanPvtErr);
    scanIoInit(&iorunscan);
    scanIoInit(&iostartscan);
}

const epicsUInt32
evgSoftSeq::getId() const {
    return m_id;
}

void
evgSoftSeq::setDescription(const char* desc) {
    m_desc.assign(desc);
}

const char* 
evgSoftSeq::getDescription() {
    return m_desc.c_str();
}

void
evgSoftSeq::setErr(std::string err) {
    m_err = err;
    scanIoRequest(ioScanPvtErr);
}

std::string
evgSoftSeq::getErr() {
    return m_err;	
}

void
evgSoftSeq::setTimestampInpMode(TimestampInpMode mode) {
        m_timestampInpMode = mode;
        scanIoRequest(ioscanpvt);
}

TimestampInpMode
evgSoftSeq::getTimestampInpMode() {
    return m_timestampInpMode;
}

void
evgSoftSeq::setTimestampResolution(TimestampResolution res) {
        m_timestampResolution = res;
        scanIoRequest(ioscanpvt);
}

TimestampResolution
evgSoftSeq::getTimestampResolution() {
    return m_timestampResolution;
}

void
evgSoftSeq::setTimestamp(epicsUInt64* timestamp, epicsUInt32 size) {
    if(size > 2047)
        throw std::runtime_error("Too many Timestamps.");

    m_timestamp.clear();
    m_timestamp.assign(timestamp, timestamp + size);

    m_isCommited = false;
    if(mrmEVGSeqDebug>1)
        fprintf(stderr, "SS%u: Update TS\n",m_id);
    scanIoRequest(ioscanpvt);
}

void
evgSoftSeq::setEventCode(epicsUInt8* eventCode, epicsUInt32 size) {	
    if(size > 2047)
        throw std::runtime_error("Too many EventCodes.");
	
    m_eventCode.clear();
    m_eventCode.assign(eventCode, eventCode + size);

    m_isCommited = false;
    if(mrmEVGSeqDebug>1)
        fprintf(stderr, "SS%u: Update Evt\n",m_id);
    scanIoRequest(ioscanpvt);
}

void
evgSoftSeq::setTrigSrc(SeqTrigSrc trigSrc) {
    if(trigSrc != m_trigSrc) {
        m_trigSrc = trigSrc;
        m_isCommited = false;
        if(mrmEVGSeqDebug>1)
            fprintf(stderr, "SS%u: Update Trig\n",m_id);
        scanIoRequest(ioscanpvt);
    }
}

void
evgSoftSeq::setRunMode(SeqRunMode runMode) {
    if(runMode != m_runMode) {
        m_runMode = runMode;
        m_isCommited = false;
        if(mrmEVGSeqDebug>1)
            fprintf(stderr, "SS%u: Update Mode\n",m_id);
        scanIoRequest(ioscanpvt);
    }
}

/*
 * Retrive commited(Ct) data.
 */

const std::vector<epicsUInt8>&
evgSoftSeq::getEventCodeCt() {
    return m_eventCodeCt;
}

const std::vector<epicsUInt64>&
evgSoftSeq::getTimestampCt() {
    return m_timestampCt;
}

SeqTrigSrc 
evgSoftSeq::getTrigSrcCt() {
    return m_trigSrcCt;
}

SeqRunMode 
evgSoftSeq::getRunModeCt() {
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
evgSoftSeq::isLoaded() {
    return m_seqRam;
}

bool
evgSoftSeq::isEnabled() {
    return m_isEnabled;
}

bool
evgSoftSeq::isCommited() {
    return m_isCommited;
}

void
evgSoftSeq::incNumOfRuns() {
    m_numOfRuns++;
}

void
evgSoftSeq::resetNumOfRuns() {
    m_numOfRuns = 0;
}

epicsUInt32
evgSoftSeq::getNumOfRuns() const {
    return m_numOfRuns;
}

void
evgSoftSeq::load() {
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
}
	
void
evgSoftSeq::unload() {
    if(!isLoaded())
        return;

    // ensure we will stop soon
    m_seqRam->setRunMode(Single);
    m_seqRam->setTrigSrc(None);
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
}


void
evgSoftSeq::commit() {
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
}

void
evgSoftSeq::enable() {
    if(isEnabled())
        return;

    if(isLoaded()) {
        /*
         * RunMode and TrigSrc could be modified in the hardware. So it is 
         * necessary to sync them before enabling the sequence.
         */
        m_seqRam->setTrigSrc(getTrigSrcCt());
        m_seqRam->setRunMode(getRunModeCt());
        m_seqRam->enable();
    }

    m_isEnabled = true;
    if(mrmEVGSeqDebug)
        fprintf(stderr, "SS%u: Enable\n",m_id);
    scanIoRequest(ioscanpvt);
}

void
evgSoftSeq::disable() {
    if(!isEnabled())
        return;

    if(m_seqRam) {
        m_seqRam->setRunMode(Single);
        m_seqRam->setTrigSrc(None);
    }

    m_isEnabled = false;
    if(mrmEVGSeqDebug)
        fprintf(stderr, "SS%u: Disable\n",m_id);
    scanIoRequest(ioscanpvt);
}

void
evgSoftSeq::abort(bool callBack) {
    if(!isLoaded() && !isEnabled())
        return;

    // Prevent re-trigger
    m_seqRam->setRunMode(Single);
    m_seqRam->setTrigSrc(None);

    m_seqRam->reset();
    m_isEnabled = false;
    if(mrmEVGSeqDebug)
        fprintf(stderr, "SS%u: Abort!\n",m_id);
    scanIoRequest(ioscanpvt);

    if(callBack) {
        /*
             * Satisfy any callback request pending on irqStop0 or irqStop1
             * recList. As no 'End of sequence' Intrrupt will be generated. 
             */
        if(m_seqRam->getId() == 0) {
            callbackRequest(&m_owner->irqStop0_cb);
            callbackRequest(&m_owner->irqStart0_cb);
        } else {
            callbackRequest(&m_owner->irqStop1_cb);
            callbackRequest(&m_owner->irqStart1_cb);
        }
    }
}

void
evgSoftSeq::pause() {
    if( m_seqRam && m_seqRam->isEnabled() ) {
        m_seqRam->disable();
        m_isEnabled = false;
        if(mrmEVGSeqDebug)
            fprintf(stderr, "SS%u: Pause\n",m_id);
        scanIoRequest(ioscanpvt);
    }
}

void
evgSoftSeq::sync() {
    if(!isLoaded() || m_isSynced)
        return;

    // Ensure the sequencer will stop at some point
    m_seqRam->setRunMode(Single);
    // Ensure the sequencer won't start if it hasn't already
    m_seqRam->setTrigSrc(None);

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
    m_seqRam->setEventCode(getEventCodeCt());
    m_seqRam->setTimestamp(getTimestampCt());
    m_seqRam->setTrigSrc(getTrigSrcCt());
    m_seqRam->setRunMode(getRunModeCt());
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
    epicsInt64 tsUInt64;
    epicsUInt8 ecUInt8;
    epicsUInt64 preTs = 0;
    epicsUInt64 curTs = 0;

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
        ecUInt8 = *itEC;

        curTs = *itTS;
        tsUInt64 = curTs - preTs;
        if(timestamp.size())
            tsUInt64 += timestamp.back();

        for(;tsUInt64 > 0xffffffff; tsUInt64 -= 0xffffffff) {
            timestamp.push_back(0xffffffff);
            eventCode.push_back(0);
        }
        preTs = curTs;

        timestamp.push_back(tsUInt64);
        eventCode.push_back(ecUInt8);
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

    /*
     * If not already present append 'End of Sequence' event code(0x7f) and
     * timestamp.
     */
    if(eventCode[eventCode.size()-1] != 0x7f) {
        eventCode.push_back(0x7f);
        if(timestamp.size() == 0)
            timestamp.push_back(evgEndOfSeqBuf);
        else
            timestamp.push_back(timestamp[timestamp.size()-1] + evgEndOfSeqBuf);
    }

    if(timestamp.size()!=eventCode.size())
        throw std::logic_error("SoftSeq, length of timestamp and eventCode doesn't match");
    else if(timestamp.size()>2047)
        throw std::runtime_error("Sequence too long");

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
    incNumOfRuns();

    if(isLoaded() && !m_isSynced)
        finishSync();

    // In single shot mode, auto-disable after
    // each run.
    if(m_runModeCt==Single && m_isEnabled)
        disable();

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

#include <epicsExport.h>
extern "C"{
 epicsExportAddress(int,mrmEVGSeqDebug);
}
