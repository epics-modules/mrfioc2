#include <iostream>
#include <stdexcept>

#include <boRecord.h>
#include <waveformRecord.h>
#include <mbboRecord.h>

#include <devSup.h>
#include <dbAccess.h>
#include <epicsExport.h>

#include "dsetshared.h"

#include "evgInit.h"
#include "evgMrm.h"
#include "evgSeqManager.h"
//#include "evgSeqRam.h"
#include "evgRegMap.h"

typedef struct {
	evgSeqRamMgr* seqRamMgr;
	evgSeqMgr* seqMgr;
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

		evgSeqMgr* seqMgr = evgMrm::getSeqMgr();
		if(!seqMgr)
			throw std::runtime_error("ERROR: Failed to lookup EVG Seq Manager");

		seqPvt* pvt = new seqPvt;
		pvt->seqRamMgr = seqRamMgr;
		pvt->seqMgr = seqMgr;
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



/** 	Initialization	**/
/*returns:(0,2)=>(success,success no convert*/
static long 
init_bo(boRecord* pbo) {
	epicsStatus ret = init_record((dbCommon*)pbo, &pbo->out);
	if(ret == 0)
		ret = 2;
	
	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long
init_wf(waveformRecord* pwf) {
	return init_record((dbCommon*)pwf, &pwf->inp);
}

/*************** Sequence Ram Records ******************/

/**		bo - loadSeq 	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_loadSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	seqPvt* pvt = (seqPvt*)pbo->dpvt;
	evgSequence* seq = pvt->seqMgr->getSequence(pbo->out.value.vmeio.signal);
	if(!seq)
		return -1;

	return pvt->seqRamMgr->load(seq);
}

/**		bo - unloadSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_unloadSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	seqPvt* pvt = (seqPvt*)pbo->dpvt;
	evgSequence* seq = pvt->seqMgr->getSequence(pbo->out.value.vmeio.signal);
	if(!seq)
		return -1;

	return pvt->seqRamMgr->unload(seq, (dbCommon*)pbo);
}

/** 	bo - commitSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_commitSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	seqPvt* pvt = (seqPvt*)pbo->dpvt;
	evgSequence* seq = pvt->seqMgr->getSequence(pbo->out.value.vmeio.signal);
	if(!seq)
		return -1;

	return pvt->seqRamMgr->commit(seq, (dbCommon*)pbo);
}

/**		bo - enableSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_enableSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	seqPvt* pvt = (seqPvt*)pbo->dpvt;
	evgSequence* seq = pvt->seqMgr->getSequence(pbo->out.value.vmeio.signal);
	if(!seq)
		return -1;

	return pvt->seqRamMgr->enable(seq);
}

/**		bo - disableSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_disableSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	seqPvt* pvt = (seqPvt*)pbo->dpvt;
	evgSequence* seq = pvt->seqMgr->getSequence(pbo->out.value.vmeio.signal);
	if(!seq)
		return -1;

	return pvt->seqRamMgr->disable(seq);
}

/**		bo - haltSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_haltSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	seqPvt* pvt = (seqPvt*)pbo->dpvt;
	evgSequence* seq = pvt->seqMgr->getSequence(pbo->out.value.vmeio.signal);
	if(!seq)
		return -1;

	return pvt->seqRamMgr->halt(seq);
}

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

common_dset devBoEvgHaltSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_haltSeq,
};
epicsExportAddress(dset, devBoEvgHaltSeq);

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