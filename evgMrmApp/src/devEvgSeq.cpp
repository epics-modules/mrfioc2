#include <iostream>
#include <stdexcept>

#include <boRecord.h>
#include <waveformRecord.h>
#include <mbboRecord.h>

#include <devSup.h>
#include <dbAccess.h>
#include <errlog.h>
#include <epicsExport.h>

#include "dsetshared.h"

#include <evgInit.h>
#include "evgSeqRam.h"
#include "evgRegMap.h"

typedef struct {
	evgSeqRamMgr* seqRamMgr;
	evgSequence* seq;
} seqPvt;

static long 
init_record(dbCommon *pRec, DBLINK* lnk) {
	long ret = 0;

	if(lnk->type != VME_IO) {
		errlogPrintf("ERROR: Hardware link not VME_IO : %s\n", pRec->name);
		return S_db_badField;
	}
	
	try {
		evgMrm* evg = FindEvg(lnk->value.vmeio.card);		
		if(!evg)
			throw std::runtime_error("ERROR: Failed to lookup EVG");
	
		evgSeqRamMgr* seqRamMgr = evg->getSeqRamMgr();
		if(!seqRamMgr)
			throw std::runtime_error("ERROR: Failed to lookup EVG SeqRam Manager");

		evgSequence* seq = seqRamMgr->getSequence(lnk->value.vmeio.signal);
		if(!seq)
			throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

		seqPvt* pvt = new seqPvt;
		pvt->seqRamMgr = seqRamMgr;
		pvt->seq = seq;
		pRec->dpvt = pvt;
		ret = 0;
	} catch(std::runtime_error& e) {
		errlogPrintf("%s : %s\n", e.what(), pRec->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("%s : %s\n", e.what(), pRec->name);
		ret = S_db_noMemory;
	}

	return ret;
}

/** 	bo - Initialization	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
init_bo(boRecord* pbo) {
	return init_record((dbCommon*)pbo, &pbo->out);
}

/**		waveform - Initialization	**/
/*returns: (-1,0)=>(failure,success)*/
static long
init_wf(waveformRecord* pwf) {
	return init_record((dbCommon*)pwf, &pwf->inp);
}

/**		mbbo - Initialization	**/
/*returns: (-1,0)=>(failure,success)*/
static long
init_mbbo(mbboRecord* pmbbo) {
	epicsStatus ret = init_record((dbCommon*)pmbbo, &pmbbo->out);
	if(ret == 0)
		ret = 2;
	
	return ret;
}


/*************** Sequence Ram Support Records ******************/

/**		longout - loadSeq 	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_loadSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;
	
	seqPvt* pvt = (seqPvt*)pbo->dpvt;
	return (pvt->seqRamMgr)->load(pvt->seq);
}

/**		longout - unloadSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_unloadSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	seqPvt* pvt = (seqPvt*)pbo->dpvt;
	return pvt->seqRamMgr->unload(pvt->seq);
}

/** 	longout - commitSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_commitSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	seqPvt* pvt = (seqPvt*)pbo->dpvt;
	return pvt->seqRamMgr->commit(pvt->seq, (dbCommon*)pbo);
}

/**		longout - enableSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_enableSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	seqPvt* pvt = (seqPvt*)pbo->dpvt;
	return pvt->seqRamMgr->enable(pvt->seq);
}

/**		longout - disableSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_disableSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	seqPvt* pvt = (seqPvt*)pbo->dpvt;
	return pvt->seqRamMgr->disable(pvt->seq);
}


/*************** Sequence Records ******************/

/**		waveform - timeStamp	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_timeStamp(waveformRecord* pwf) {
	seqPvt* pvt = (seqPvt*)pwf->dpvt;
	return pvt->seq->setTimeStamp((epicsUInt32*)pwf->bptr, pwf->nelm);
}

/**		waveform - eventCode	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_eventCode(waveformRecord* pwf) {
	seqPvt* pvt = (seqPvt*)pwf->dpvt;
	return pvt->seq->setEventCode((epicsUInt8*)pwf->bptr, pwf->nelm);
}

/**		mbbo - runMode		**/
static long
write_mbbo_runMode(mbboRecord* pmbbo) {
	seqPvt* pvt = (seqPvt*)pmbbo->dpvt;
	return pvt->seq->setRunMode((SeqRunMode)pmbbo->val);
}


/*************** Sequence Ram Records ******************/

/**		waveform - seqLoaded	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_loadedSeq(waveformRecord* pwf) {
	seqPvt* pvt = (seqPvt*)pwf->dpvt;

	epicsUInt32* p = (epicsUInt32*)pwf->bptr;
	
	for(int i = 0; i < evgNumSeqRam; i++) {
		evgSequence* seq = pvt->seqRamMgr->getSeqRam(i)->getSequence();
		if(seq) {
			p[i] = seq->getId();
		} else {
			p[i] = -1;
		}
	}
	
	return 0;
}


/** 	device support entry table 		**/
extern "C" {

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


common_dset devWfEvgTimeStamp = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf,
    NULL,
    (DEVSUPFUN)write_wf_timeStamp,
};
epicsExportAddress(dset, devWfEvgTimeStamp);

common_dset devWfEvgEventCode = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf,
    NULL,
    (DEVSUPFUN)write_wf_eventCode,
};
epicsExportAddress(dset, devWfEvgEventCode);

common_dset devMbboEvgRunMode = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbo,
    NULL,
    (DEVSUPFUN)write_mbbo_runMode,
};
epicsExportAddress(dset, devMbboEvgRunMode);


common_dset devWfEvgLoadedSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf,
    NULL,
    (DEVSUPFUN)write_wf_loadedSeq,
};
epicsExportAddress(dset, devWfEvgLoadedSeq);

};
