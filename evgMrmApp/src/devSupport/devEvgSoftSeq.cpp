#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string.h>
#include <math.h>

#include <stringinRecord.h>
#include <waveformRecord.h>
#include <mbboRecord.h>
#include <mbbiRecord.h>
#include <boRecord.h>
#include <biRecord.h>
#include <longinRecord.h>

#include <devSup.h>
#include <dbAccess.h>
#include <errlog.h>
#include <epicsExport.h>
#include <errlog.h>

#include "devObj.h"

#include "evgInit.h"
#include "evgRegMap.h"

struct Pvt {
    evgMrm* evg;
    evgSoftSeq* seq;
};

/**     Initialization    **/
static long 
init_wf_pvt (dbCommon *pRec) {
    long ret = 0;
    waveformRecord* pwf = (waveformRecord*)pRec;

    if(pwf->inp.type != VME_IO) {
        errlogPrintf("ERROR: Hardware link not VME_IO : %s\n", pwf->name);
        return S_db_badField;
    }
    
    try {
        std::string parm(pwf->inp.value.vmeio.parm);
        evgMrm* evg = dynamic_cast<evgMrm*>(mrf::Object::getObject(parm));
        if(!evg)
            throw std::runtime_error("Failed to lookup EVG");

        evgSoftSeqMgr* seqMgr = evg->getSoftSeqMgr();
        if(!seqMgr)
            throw std::runtime_error("Failed to lookup EVG Seq Manager");

        evgSoftSeq* seq = seqMgr->getSoftSeq(pwf->inp.value.vmeio.signal);
        if(!seq)
            throw std::runtime_error("Failed to lookup EVG Sequence");

        Pvt* pvt = new Pvt;
        pvt->evg = evg;
        pvt->seq = seq;
        pwf->dpvt = pvt;
        ret = 0;
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_db_noMemory;
    }

    return ret;
}

static long 
init_record(dbCommon *pRec, DBLINK* lnk) {
    long ret = 0;

    if(lnk->type != VME_IO) {
        errlogPrintf("ERROR: Hardware link not VME_IO : %s\n", pRec->name);
        return S_db_badField;
    }
    
    try {
        std::string parm(lnk->value.vmeio.parm);
        evgMrm* evg = dynamic_cast<evgMrm*>(mrf::Object::getObject(parm));
        if(!evg)
            throw std::runtime_error("Failed to lookup EVG");

        evgSoftSeqMgr* seqMgr = evg->getSoftSeqMgr();
        if(!seqMgr)
            throw std::runtime_error("Failed to lookup EVG Seq Manager");

        evgSoftSeq* seq = seqMgr->getSoftSeq(lnk->value.vmeio.signal);
        if(!seq)
            throw std::runtime_error("Failed to lookup EVG Sequence");

        pRec->dpvt = seq;
        ret = 0;
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pRec->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pRec->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
init_si(stringinRecord* psi) {
    return init_record((dbCommon*)psi, &psi->inp);
}

/*returns: (-1,0)=>(failure,success)*/
static long
init_wf(waveformRecord* pwf) {
    return init_record((dbCommon*)pwf, &pwf->inp);
}

/*returns: (0,2)=>(success,success no convert)*/
static long
init_mbbo(mbboRecord* pmbbo) {
    epicsStatus ret = init_record((dbCommon*)pmbbo, &pmbbo->out);
    if(ret == 0)
        ret = 2;
    
    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long
init_mbbi(mbbiRecord* pmbbi) {
    return init_record((dbCommon*)pmbbi, &pmbbi->inp);
}

/*returns:(0,2)=>(success,success no convert*/
static long 
init_bo(boRecord* pbo) {
    epicsStatus ret = init_record((dbCommon*)pbo, &pbo->out);
    if(ret == 0)
        ret = 2;
    
    return ret;
}

static long 
init_bi(biRecord* pbi) {
    return init_record((dbCommon*)pbi, &pbi->inp);
}

static long
init_li(longinRecord* pli) {
    return init_record((dbCommon*)pli, &pli->inp);
}

/**        Read/Write Function        **/
static long
get_ioint_info_pvt(int cmd, dbCommon *pwf, IOSCANPVT *ppvt) {
    Pvt* dpvt = (Pvt*)pwf->dpvt;
    if(!dpvt)
        throw std::runtime_error("Initialization failed");

    evgMrm* evg = (evgMrm*)dpvt->evg;
    evgSoftSeq* seq = (evgSoftSeq*)dpvt->seq;
    if(! (evg || seq))
        throw std::runtime_error("Device pvt field not initialized correctly");
    *ppvt = seq->ioscanpvt;

    return 0;
}

static long 
get_ioint_info(int cmd, dbCommon *pRec, IOSCANPVT *ppvt) {
    evgSoftSeq* seq = (evgSoftSeq*)pRec->dpvt;
    if(!seq) {
        errlogPrintf("Device pvt field not initialized\n");
        return -1;
    }

    *ppvt = seq->ioscanpvt;
    return 0;
}

static long 
get_ioint_info_err(int cmd, dbCommon *pRec, IOSCANPVT *ppvt) {
    evgSoftSeq* seq = (evgSoftSeq*)pRec->dpvt;
    if(!seq) {
        errlogPrintf("Device pvt field not initialized\n");
        return -1;
    }

    *ppvt = seq->ioScanPvtErr;
    return 0;
}

/*returns: (-1,0)=>(failure,success)*/
static long
write_bo_timestampInpMode(boRecord* pbo) {
    long ret = OK;
    evgSoftSeq* seq = 0;

    try {
        seq = (evgSoftSeq*)pbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->setTimestampInpMode((TimestampInpMode)pbo->val);
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long
read_bi_timestampInpMode(biRecord* pbi) {
    long ret = 0;
    evgSoftSeq* seq = 0;

    try {
        seq = (evgSoftSeq*)pbi->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        pbi->rval = seq->getTimestampInpMode();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbi->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbi->name);
        ret = S_db_noMemory;
    }

    return ret;
}

static long
write_mbbo_timestampResolution(mbboRecord* pmbbo) {
    long ret = OK;
    evgSoftSeq* seq = 0;

    try {
        seq = (evgSoftSeq*)pmbbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->setTimestampResolution((TimestampResolution)pmbbo->rval);
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*(0,2)=>(success, success no convert)*/
static long
read_mbbi_timestampResolution(mbbiRecord* pmbbi) {
    long ret = OK;
    evgSoftSeq* seq = 0;

    try {
        seq = (evgSoftSeq*)pmbbi->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        pmbbi->rval = seq->getTimestampResolution();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbi->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbi->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long
write_wf_timestamp(waveformRecord* pwf) {
    epicsStatus ret = OK;

    try {
        Pvt* pvt = (Pvt*)pwf->dpvt;
        if(!pvt)
            throw std::runtime_error("Device pvt field not initialized");

        evgMrm* evg = (evgMrm*)pvt->evg;
        evgSoftSeq* seq = (evgSoftSeq*)pvt->seq;
        if(! (evg || seq))
            throw std::runtime_error("Device pvt field not initialized correctly");

        epicsUInt32 size = pwf->nord;
        epicsUInt64 ts[size];

        SCOPED_LOCK2(seq->m_lock, guard);
        if(seq->getTimestampInpMode() == TICKS) {
            for(unsigned int i = 0; i < size; i++)
                ts[i] = (epicsUInt64)floor(((epicsFloat64*)pwf->bptr)[i] + 0.5);
        } else {
            /*
             * When timestamp Input mode is EGU; Scale the time to seconds
             * and then converting seconds to clock ticks
             */
            epicsFloat64 seconds;
            epicsUInt32 timeScaler = seq->getTimestampResolution() ;
            for(unsigned int i = 0; i < size; i++) {
               seconds  = ((epicsFloat64*)pwf->bptr)[i] / pow(10, timeScaler);
               ts[i] = (epicsUInt64)floor(seconds *
                       evg->getEvtClk()->getFrequency() * pow(10,6) + 0.5);
            }
        }
        seq->setTimestamp(ts, size);
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long
read_wf_timestamp(waveformRecord* pwf) {
    long ret = OK;

    try {
        Pvt* dpvt = (Pvt*)pwf->dpvt;
        if(!dpvt)
            throw std::runtime_error("Initialization failed");

        evgMrm* evg = (evgMrm*)dpvt->evg;
        evgSoftSeq* seq = (evgSoftSeq*)dpvt->seq;
        if(! (evg || seq))
            throw std::runtime_error("Device pvt field not initialized correctly");

        SCOPED_LOCK2(seq->m_lock, guard);
        std::vector<uint64_t> timestamp = seq->getTimestampCt();
        epicsFloat64 evtClk = evg->getEvtClk()->getFrequency() * pow(10,6);
        epicsUInt32 timeScaler = seq->getTimestampResolution();

        epicsFloat64* bptr = (epicsFloat64*)pwf->bptr;
        for(unsigned int i = 0; i < timestamp.size(); i++) {
            if(seq->getTimestampInpMode() == TICKS)
                 bptr[i] = (epicsFloat64)timestamp[i];
            else
                bptr[i] = timestamp[i] * pow(10, timeScaler) / evtClk;
        }

        pwf->nord = timestamp.size();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_eventCode(waveformRecord* pwf) {
    long ret = OK;

    try {
        evgSoftSeq* seq = (evgSoftSeq*)pwf->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->setEventCode((epicsUInt8*)pwf->bptr, pwf->nord);
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
read_wf_eventCode(waveformRecord* pwf) {
    long ret = 0;

    try {
        evgSoftSeq* seq = (evgSoftSeq*)pwf->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        std::vector<epicsUInt8> eventCode = seq->getEventCodeCt();
        epicsUInt8* bptr = (epicsUInt8*)pwf->bptr;
        for(unsigned int i = 0; i < eventCode.size(); i++)
            bptr[i] = eventCode[i];

        pwf->nord = eventCode.size();

    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_db_noMemory;
    }
    
    return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long
write_mbbo_runMode(mbboRecord* pmbbo) {
    long ret = 2;

    try {
        evgSoftSeq* seq = (evgSoftSeq*)pmbbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->setRunMode((SeqRunMode)pmbbo->val);
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
        ret = S_db_noMemory;
    }
    
    return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long
read_mbbi_runMode(mbbiRecord* pmbbi) {
    long ret = 2;

    try {
        evgSoftSeq* seq = (evgSoftSeq*)pmbbi->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        pmbbi->val = seq->getRunModeCt();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbi->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbi->name);
        ret = S_db_noMemory;
    }
    
    return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long
write_mbbo_trigSrc(mbboRecord* pmbbo) {
    long ret = OK;

    try {
        evgSoftSeq* seq = (evgSoftSeq*)pmbbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->setTrigSrc((SeqTrigSrc)pmbbo->rval);
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
read_mbbi_trigSrc(mbbiRecord* pmbbi) {
    long ret = OK;

    try {
        evgSoftSeq* seq = (evgSoftSeq*)pmbbi->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        pmbbi->rval = seq->getTrigSrcCt();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbi->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbi->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_loadSeq(boRecord* pbo) {
    long ret = OK;
    unsigned long dummy;
    evgSoftSeq* seq = 0;

    try {
        if(!pbo->val)
            return 0;

        seq = (evgSoftSeq*)pbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->load();
        seq->setErr("");
    } catch(std::runtime_error& e) {
        dummy = recGblSetSevr(pbo, WRITE_ALARM, MAJOR_ALARM);
        seq->setErr(e.what());
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_unloadSeq(boRecord* pbo) {
    long ret = OK;
    unsigned long dummy;
    evgSoftSeq* seq = 0;

    try {
        if(!pbo->val)
            return 0;

        seq = (evgSoftSeq*)pbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->unload();
        seq->setErr("");
    } catch(std::runtime_error& e) {
        dummy = recGblSetSevr(pbo, WRITE_ALARM, MAJOR_ALARM);
        seq->setErr(e.what());
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_commitSeq(boRecord* pbo) {
    long ret = OK;
    unsigned long dummy;
    evgSoftSeq* seq = 0;

    try {
        if(!pbo->val)
            return 0;

        seq = (evgSoftSeq*)pbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->commit();
        seq->setErr("");
    } catch(std::runtime_error& e) {
        dummy = recGblSetSevr(pbo, WRITE_ALARM, MAJOR_ALARM);
        seq->setErr(e.what());
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_enableSeq(boRecord* pbo) {
    long ret = OK;
    unsigned long dummy;
    evgSoftSeq* seq = 0;

    try {
        if(!pbo->val)
            return 0;

        seq = (evgSoftSeq*)pbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->enable();
        seq->setErr("");
    } catch(std::runtime_error& e) {
        dummy = recGblSetSevr(pbo, WRITE_ALARM, MAJOR_ALARM);
        seq->setErr(e.what());
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_disableSeq(boRecord* pbo) {
    long ret = OK;
    unsigned long dummy;
    evgSoftSeq* seq = 0;

    try {
        if(!pbo->val)
            return 0;

        seq = (evgSoftSeq*)pbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->disable();
        seq->setErr("");
    } catch(std::runtime_error& e) {
        dummy = recGblSetSevr(pbo, WRITE_ALARM, MAJOR_ALARM);
        seq->setErr(e.what());
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }
    
    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_abortSeq(boRecord* pbo) {
    long ret = OK;
    evgSoftSeq* seq = 0;

    try {
        if(!pbo->val)
            return 0;

        seq = (evgSoftSeq*)pbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->abort(pbo->val);
        seq->setErr("");
    } catch(std::runtime_error& e) {
        seq->setErr(e.what());
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_pauseSeq(boRecord* pbo) {
    long ret = OK;
    evgSoftSeq* seq = 0;

    try {
        if(!pbo->val)
            return 0;

        seq = (evgSoftSeq*)pbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        seq->pause();
        seq->setErr("");
    } catch(std::runtime_error& e) {
        seq->setErr(e.what());
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_softTrig(boRecord* pbo) {
    long ret = OK;

    try {
        if(!pbo->val)
            return 0;

        evgSoftSeq* seq = (evgSoftSeq*)pbo->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        evgSeqRam* seqRam = seq->getSeqRam();
        if(!seqRam)
            throw std::runtime_error("Failed to lookup EVG Seq RAM");

        SCOPED_LOCK2(seq->m_lock, guard);
        seqRam->softTrig();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*(0,2)=> success and convert, don't convert)*/
static long 
read_bi_loadStatus(biRecord* pbi) {
    long ret = 2;

    try {
        evgSoftSeq* seq = (evgSoftSeq*)pbi->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        pbi->val = seq->isLoaded();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbi->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbi->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*(0,2)=> success and convert, don't convert)*/
static long 
read_bi_enaStatus(biRecord* pbi) {
    long ret = 2;

    try {
        evgSoftSeq* seq = (evgSoftSeq*)pbi->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        pbi->val = seq->isEnabled();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbi->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbi->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*(0,2)=> success and convert, don't convert)*/
static long 
read_bi_commitStatus(biRecord* pbi) {
    long ret = 2;

    try {
        evgSoftSeq* seq = (evgSoftSeq*)pbi->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        SCOPED_LOCK2(seq->m_lock, guard);
        pbi->val = seq->isCommited();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbi->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbi->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/**     Loaded Soft Sequences    **/
/*returns: (-1,0)=>(failure,success)*/
static long
init_wf_loadedSeq(waveformRecord* pwf) {
    long ret = 0;

    if(pwf->inp.type != VME_IO) {
        errlogPrintf("ERROR: Hardware link not VME_IO : %s\n", pwf->name);
        return S_db_badField;
    }
    
    try {
        std::string parm(pwf->inp.value.vmeio.parm);
        evgMrm* evg = dynamic_cast<evgMrm*>(mrf::Object::getObject(parm));
        if(!evg)
            throw std::runtime_error("Failed to lookup EVG");

        evgSeqRamMgr* seqRamMgr = evg->getSeqRamMgr();
        if(!seqRamMgr)
            throw std::runtime_error("Failed to lookup EVG Seq Ram Manager");

        pwf->dpvt = seqRamMgr;
        ret = 0;
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
read_wf_loadedSeq(waveformRecord* pwf) {
    long ret = 0;

    try {
        evgSeqRamMgr* seqRamMgr = (evgSeqRamMgr*)pwf->dpvt;
        if(!seqRamMgr) {
            errlogPrintf("Device pvt field not initialized\n");
            return -1;
        }

        epicsInt32* pBuf = (epicsInt32*)pwf->bptr;
        evgSoftSeq* seq;

        for(int i = 0; i < evgNumSeqRam; i++) {
            seq = seqRamMgr->getSeqRam(i)->getSoftSeq();
            if(seq)
                pBuf[i] = seq->getId();
            else
                pBuf[i] = -1;
        }
        pwf->nord = evgNumSeqRam;
        ret = 0;
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pwf->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long
read_si_err(stringinRecord* psi) {
    long ret = 0;

    try {
        evgSoftSeq* seq = (evgSoftSeq*)psi->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        strcpy(psi->val, seq->getErr().c_str());
        ret = 0;
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), psi->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), psi->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long
read_li_numOfRuns(longinRecord* pli) {
    long ret = 0;

    try {
        evgSoftSeq* seq = (evgSoftSeq*)pli->dpvt;
        if(!seq)
            throw std::runtime_error("Device pvt field not initialized");

        pli->val = seq->getNumOfRuns();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pli->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pli->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/**     device support entry table         **/
extern "C" {

common_dset devBoEvgTimestampInpMode = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_timestampInpMode,
};
epicsExportAddress(dset, devBoEvgTimestampInpMode);

common_dset devBiEvgTimestampInpMode = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bi,
    NULL,
    (DEVSUPFUN)read_bi_timestampInpMode,
};
epicsExportAddress(dset, devBiEvgTimestampInpMode);

common_dset devMbboEvgTimestampResolution = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbo,
    NULL,
    (DEVSUPFUN)write_mbbo_timestampResolution,
};
epicsExportAddress(dset, devMbboEvgTimestampResolution);

common_dset devMbbiEvgTimestampResolution = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbi,
    NULL,
    (DEVSUPFUN)read_mbbi_timestampResolution,
};
epicsExportAddress(dset, devMbbiEvgTimestampResolution);

common_dset devWfEvgTimestamp = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf_pvt,
    NULL,
    (DEVSUPFUN)write_wf_timestamp,
};
epicsExportAddress(dset, devWfEvgTimestamp);

common_dset devWfEvgTimestampRB = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf_pvt,
    (DEVSUPFUN)get_ioint_info_pvt,
    (DEVSUPFUN)read_wf_timestamp,
};
epicsExportAddress(dset, devWfEvgTimestampRB);

common_dset devWfEvgEventCode = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf,
    NULL,
    (DEVSUPFUN)write_wf_eventCode,
};
epicsExportAddress(dset, devWfEvgEventCode);

common_dset devWfEvgEventCodeRB = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)read_wf_eventCode,
};
epicsExportAddress(dset, devWfEvgEventCodeRB);

common_dset devMbboEvgRunMode = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbo,
    NULL,
    (DEVSUPFUN)write_mbbo_runMode,
};
epicsExportAddress(dset, devMbboEvgRunMode);

common_dset devMbbiEvgRunMode = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbi,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)read_mbbi_runMode,
};
epicsExportAddress(dset, devMbbiEvgRunMode);

common_dset devMbboEvgTrigSrc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbo,
    NULL,
    (DEVSUPFUN)write_mbbo_trigSrc,
};
epicsExportAddress(dset, devMbboEvgTrigSrc);

common_dset devMbbiEvgTrigSrc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbi,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)read_mbbi_trigSrc,
};
epicsExportAddress(dset, devMbbiEvgTrigSrc);

common_dset devBoEvgLoadSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_loadSeq,
};
epicsExportAddress(dset, devBoEvgLoadSeq);

common_dset devBoEvgUnloadSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_unloadSeq,
};
epicsExportAddress(dset, devBoEvgUnloadSeq);

common_dset devBoEvgCommitSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_commitSeq,
};
epicsExportAddress(dset, devBoEvgCommitSeq);

common_dset devBoEvgEnableSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_enableSeq,
};
epicsExportAddress(dset, devBoEvgEnableSeq);

common_dset devBoEvgDisableSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_disableSeq,
};
epicsExportAddress(dset, devBoEvgDisableSeq);

common_dset devBoEvgAbortSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_abortSeq,
};
epicsExportAddress(dset, devBoEvgAbortSeq);

common_dset devBoEvgPauseSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_pauseSeq,
};
epicsExportAddress(dset, devBoEvgPauseSeq);

common_dset devBoEvgSoftTrig = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_softTrig,
};
epicsExportAddress(dset, devBoEvgSoftTrig);

common_dset devBiEvgLoadStatus = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bi,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)read_bi_loadStatus,
};
epicsExportAddress(dset, devBiEvgLoadStatus);

common_dset devBiEvgEnaStatus = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bi,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)read_bi_enaStatus,
};
epicsExportAddress(dset, devBiEvgEnaStatus);

common_dset devBiEvgCommitStatus = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bi,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)read_bi_commitStatus,
};
epicsExportAddress(dset, devBiEvgCommitStatus);

common_dset devSiErr = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_si,
    (DEVSUPFUN)get_ioint_info_err,
    (DEVSUPFUN)read_si_err,
};
epicsExportAddress(dset, devSiErr);

common_dset devLiNumOfRuns = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_li,
    NULL,
    (DEVSUPFUN)read_li_numOfRuns,
};
epicsExportAddress(dset, devLiNumOfRuns);

common_dset devWfEvgLoadedSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf_loadedSeq,
    NULL,
    (DEVSUPFUN)read_wf_loadedSeq,
};
epicsExportAddress(dset, devWfEvgLoadedSeq);

};

