#include "evgSoftSeq.h"

#include <math.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include <vector>

#include <errlog.h>

#include <mrfCommonIO.h>
#include <mrfCommon.h>
#include "evgRegMap.h"

#include "evgMrm.h"

evgSoftSeq::evgSoftSeq(const epicsUInt32 id, evgMrm* const owner):
m_lock(),
m_id(id),
m_owner(owner),
m_pReg(owner->getRegAddr()),
m_trigSrc(Mxc0),
m_runMode(Normal),
m_trigSrcCt(Mxc0),
m_runModeCt(Normal),
m_seqRam(0),
m_seqRamMgr(owner->getSeqRamMgr()),
m_isEnabled(0),
m_isCommited(0) {
    scanIoInit(&ioscanpvt);
    scanIoInit(&ioScanPvtErr);
}

const epicsUInt32
evgSoftSeq::getId() const {
    return m_id;
}

epicsStatus 
evgSoftSeq::setDescription(const char* desc) {
    m_desc.assign(desc);
    return OK; 
}

const char* 
evgSoftSeq::getDescription() {
    return m_desc.c_str();
}

epicsStatus 
evgSoftSeq::setErr(std::string err) {
    m_err = err;
    scanIoRequest(ioScanPvtErr);
    return OK;
}

std::string
evgSoftSeq::getErr() {
    return m_err;	
}

epicsStatus
evgSoftSeq::setEventCode(epicsUInt8* eventCode, epicsUInt32 size) {	
    if(size > 2047)
        throw std::runtime_error("Too many EventCodes.");
	
    m_eventCode.clear();
    m_eventCode.assign(eventCode, eventCode + size);

    m_isCommited = false;
    scanIoRequest(ioscanpvt);
    return OK;
}

epicsStatus
evgSoftSeq::setTimestamp(epicsFloat64* timestamp, epicsUInt32 size) {
    std::vector<uint64_t> timestamp_bk = m_timestamp;
    m_timestamp.clear();
    m_timestamp.resize(size);

    uint64_t preValue = 0;
    uint64_t curValue = 0;

    for(unsigned int i = 0; i < size; i++) {
        curValue = (uint64_t)(floor(timestamp[i] * 
                   m_owner->getEvtClk()->getEvtClkSpeed() * pow(10,6) + 0.5));

        m_timestamp[i] = curValue - preValue;

        if(m_timestamp[i] <= 0) {
            m_timestamp = timestamp_bk;
            throw std::runtime_error("Timestamp values are not Sorted/Unique");
        }

        preValue = curValue;
    }

    m_timestampRaw = false; 
    m_isCommited = false;
    scanIoRequest(ioscanpvt);
	
    return OK;
}

epicsStatus
evgSoftSeq::setTimestampRaw(epicsUInt32* timestamp, epicsUInt32 size) {
    if(size > 2047)
        throw std::runtime_error("Too many Timestamps.");

    m_timestamp.clear();
    m_timestamp.assign(timestamp, timestamp + size);

    m_timestampRaw = true; 
    m_isCommited = false;
    scanIoRequest(ioscanpvt);
	
    return OK;
}

epicsStatus 
evgSoftSeq::setTrigSrc(SeqTrigSrc trigSrc) {
    if(trigSrc != m_trigSrc) {
        m_trigSrc = trigSrc;
        m_isCommited = false;
        scanIoRequest(ioscanpvt);
    }
    return OK;
}

epicsStatus 
evgSoftSeq::setRunMode(SeqRunMode runMode) {
    if(runMode != m_runMode) {
        m_runMode = runMode;
        m_isCommited = false;
        scanIoRequest(ioscanpvt);
    }
    return OK;
}

void
evgSoftSeq::commitSoftSeq() {
    m_timestampCt = m_timestamp;
    m_eventCodeCt = m_eventCode;
    m_trigSrcCt = m_trigSrc;
    m_runModeCt = m_runMode;

    /*Appending 'End of Sequence' EventCode and Timestamp. */
    m_eventCodeCt.push_back(0x7f);
    if(m_timestampCt.size() == 0)
        m_timestampCt.push_back(1);
    else
        m_timestampCt.push_back(m_timestampCt[m_timestampCt.size()-1] + 1);
}

std::vector<epicsUInt8>
evgSoftSeq::getEventCodeCt() {
    return m_eventCodeCt;
}

std::vector<uint64_t>
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

epicsStatus 
evgSoftSeq::setSeqRam(evgSeqRam* seqRam) {
    m_seqRam = seqRam;
    return OK;
}

evgSeqRam* 
evgSoftSeq::getSeqRam() {
    return m_seqRam;
}

bool
evgSoftSeq::isLoaded() {
    if(m_seqRam)
        return true;
    else
        return false;
}

bool
evgSoftSeq::isEnabled() {
    return m_isEnabled;
}

bool
evgSoftSeq::isCommited() {
    return m_isCommited;
}

epicsStatus
evgSoftSeq::load() {
    evgSeqRam* seqRam = 0;
    evgSeqRam* seqRamIter = 0;

    /*Initialize the seqRam if hardware is available(un-allocated)*/
    for(unsigned int i = 0; i < m_seqRamMgr->numOfRams(); i++) {
        seqRamIter = m_seqRamMgr->getSeqRam(i);
        if( seqRamIter->isAllocated() ) {
            if(this == seqRamIter->getSoftSeq()) {
                char err[80];
                sprintf(err, "SoftSeq %d is already loaded", getId());
                std::string strErr(err);
                throw std::runtime_error(strErr);
            }
        } else {
            if( !seqRam )
                seqRam = seqRamIter;
        }
    }

    /*If a seqRam is avialable*/
    if(seqRam != 0) {
        setSeqRam(seqRam);
        seqRam->alloc(this);
        scanIoRequest(ioscanpvt);
        if(m_isCommited)
            sync(); 
		
        return OK;	
    } else {
        char err[80];
        sprintf(err, "No SeqRam to load SoftSeq %d", getId());
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }
}
	
epicsStatus
evgSoftSeq::unload() {
    if(m_seqRam) {
        m_seqRam->setRunMode(Single);
        m_seqRam->dealloc();
        setSeqRam(0);
        scanIoRequest(ioscanpvt);
    }

    return OK;
}


epicsStatus
evgSoftSeq::commit() {	
    inspectSoftSeq();	
    commitSoftSeq();

    if(m_seqRam != 0)
        sync();

    m_isCommited = true;
    scanIoRequest(ioscanpvt);
    return OK;
}

epicsStatus
evgSoftSeq::enable() {
    if(m_seqRam) {
        /*RunMode and TrigSrc could be modified in the hardware. So it is 
          necessary to sync them before enabling the sequence.*/
        m_seqRam->setTrigSrc(getTrigSrcCt());
        m_seqRam->setRunMode(getRunModeCt());
        m_seqRam->enable();
    }

    m_isEnabled = true;
    scanIoRequest(ioscanpvt);
    return OK;
}

epicsStatus
evgSoftSeq::disable() {
    if(m_seqRam)
        m_seqRam->setRunMode(Single);

    m_isEnabled = false;
    scanIoRequest(ioscanpvt);
    return OK;
}

epicsStatus
evgSoftSeq::abort(bool callBack) {
    if( m_seqRam && m_seqRam->isEnabled() ) {
        m_seqRam->reset();
        m_isEnabled = false;
        scanIoRequest(ioscanpvt);

        if(callBack) {
            /*Satisfy any callback request pending on irqStop0 or irqStop1 recList.
            As no 'End of sequence' Intrrupt will be generated. */
            if(m_seqRam->getId() == 0)
                callbackRequest(&m_owner->irqStop0_cb);
            else 
                callbackRequest(&m_owner->irqStop1_cb);
        }
    }

    return OK;
}

epicsStatus
evgSoftSeq::pause() {
    if( m_seqRam && m_seqRam->isEnabled() ) {
        m_seqRam->disable();
        m_isEnabled = false;
        scanIoRequest(ioscanpvt);
    }

    return OK;
}

epicsStatus
evgSoftSeq::sync() {
    if(!m_seqRam) {
        char err[80];
        sprintf(err, "SeqRam is not allocated for SoftSeq %d", getId());
        std::string strErr(err);
        throw std::runtime_error(strErr);
    }

    if(isRunning())
        return OK;

    m_seqRam->setEventCode(getEventCodeCt());
    m_seqRam->setTimestamp(getTimestampCt());
    m_seqRam->setTrigSrc(getTrigSrcCt());
    m_seqRam->setRunMode(getRunModeCt());
    if(m_isEnabled) m_seqRam->enable();

    scanIoRequest(ioscanpvt);

    return OK;
}

/* This function makes sure that this sequence does not run again.*/
epicsStatus
evgSoftSeq::isRunning() {
    m_seqRam->setRunMode(Single);
    m_seqRam->setTrigSrc(SW_Ram0);
	
    /*Clear the sequencer stop interrupt flag*/
    WRITE32(m_pReg, IrqFlag, EVG_IRQ_STOP_RAM(m_seqRam->getId()));

    if(!m_seqRam->isRunning()) {
        m_seqRam->disable();
        return 0;
    } else {	
        /*Enable the sequencer stop interrupt*/	
        BITSET32(m_pReg, IrqEnable, EVG_IRQ_STOP_RAM(m_seqRam->getId()));
        return 1;
    }		
}

epicsStatus
evgSoftSeq::inspectSoftSeq() {
    int64_t tsUInt64;
    epicsUInt8 ecUInt8;
    int64_t pre = 0;

    std::vector<uint64_t> timestamp;
    std::vector<epicsUInt8> eventCode;

    /*Make EventCode and Timestamp vector of same size(Smaller of the two vectors).
      Also take care of rollover if timestamp input was in units of seconds*/
    std::vector<uint64_t>::iterator itTS = m_timestamp.begin();
    std::vector<epicsUInt8>::iterator itEC = m_eventCode.begin();
    for(; itTS < m_timestamp.end() && itEC < m_eventCode.end(); itTS++, itEC++) {
        ecUInt8 = *itEC;

        if(!m_timestampRaw) {
            tsUInt64 = *itTS + pre;
            for(;tsUInt64 > 0xffffffff; tsUInt64 -= 0xffffffff) {
                timestamp.push_back(0xffffffff);
                eventCode.push_back(0);
            }
        } else 
            tsUInt64 = *itTS;

            timestamp.push_back(tsUInt64);
            eventCode.push_back(ecUInt8);
            pre = tsUInt64;
        }

    m_timestamp = timestamp;
    m_eventCode = eventCode;

    /*Check if the timestamps are sorted and Unique */
    if(m_timestamp.size() > 1) {
        for(unsigned int i = 0; i < m_timestamp.size()-1; i++) {
            if( m_timestamp[i] >= m_timestamp[i+1] ) {
                if( (m_timestamp[i] == 0xffffffff) && (m_eventCode[i] == 0))
                    continue;
                else
                    throw std::runtime_error("Sequencer timestamps are not \
					      Sorted/Unique");
            } 
        }
    }

    return OK;
}

