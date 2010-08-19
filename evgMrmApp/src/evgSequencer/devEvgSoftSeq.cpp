#include <iostream>
#include <stdexcept>

#include <waveformRecord.h>
#include <mbboRecord.h>
#include <boRecord.h>

#include <devSup.h>
#include <dbAccess.h>
#include <epicsExport.h>

#include "dsetshared.h"

#include "evgInit.h"
#include "evgRegMap.h"

static long 
init_record(dbCommon *pRec, DBLINK* lnk) {
	long ret = 0;

	if(lnk->type != VME_IO) {
		errlogPrintf("ERROR: Hardware link not VME_IO : %s\n", pRec->name);
		return S_db_badField;
	}
	
	try {
		evgMrm* evg = &evgmap.get(lnk->value.vmeio.card);		
		if(!evg)
			throw std::runtime_error("ERROR: Failed to lookup EVG");

		evgSoftSeqMgr* seqMgr = evg->getSoftSeqMgr();
		if(!seqMgr)
			throw std::runtime_error("ERROR: Failed to lookup EVG Seq Manager");

		evgSoftSeq* seq = seqMgr->getSoftSeq(lnk->value.vmeio.signal);
		if(!seq)
			throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

		pRec->dpvt = seq;
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
init_wf_loadedSeq(waveformRecord* pwf) {
	long ret = 0;

	if(pwf->inp.type != VME_IO) {
		errlogPrintf("ERROR: Hardware link not VME_IO : %s\n", pwf->name);
		return S_db_badField;
	}
	
	try {
		evgMrm* evg = &evgmap.get(pwf->inp.value.vmeio.card);		
		if(!evg)
			throw std::runtime_error("ERROR: Failed to lookup EVG");

		evgSeqRamMgr* seqRamMgr = evg->getSeqRamMgr();
		if(!seqRamMgr)
			throw std::runtime_error("ERROR: Failed to lookup EVG Seq Ram Manager");

		pwf->dpvt = seqRamMgr;
		ret = 0;
	} catch(std::runtime_error& e) {
		errlogPrintf("%s : %s\n", e.what(), pwf->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("%s : %s\n", e.what(), pwf->name);
		ret = S_db_noMemory;
	}

	return ret;
}

/*************** Soft Sequence Records ******************/

/**		waveform - timeStamp	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_timeStampTick(waveformRecord* pwf) {
	evgSoftSeq* seq = (evgSoftSeq*)pwf->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");
	
	return seq->setTimeStampTick((epicsUInt32*)pwf->bptr, pwf->nord);
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_timeStampSec(waveformRecord* pwf) {
	evgSoftSeq* seq = (evgSoftSeq*)pwf->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");
	
	return seq->setTimeStampSec((epicsFloat64*)pwf->bptr, pwf->nord);
}

/**		waveform - eventCode	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_eventCode(waveformRecord* pwf) {
	evgSoftSeq* seq = (evgSoftSeq*)pwf->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->setEventCode((epicsUInt8*)pwf->bptr, pwf->nord);
}

/**		mbbo - runMode		**/
/*returns: (0,2)=>(success,success no convert)*/
static long
write_mbbo_runMode(mbboRecord* pmbbo) {
	evgSoftSeq* seq = (evgSoftSeq*)pmbbo->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->setRunMode((SeqRunMode)pmbbo->val);
}

/**		mbbo - trigSrc 	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbbo_trigSrc(mbboRecord* pmbbo) {
	evgSoftSeq* seq = (evgSoftSeq*)pmbbo->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->setTrigSrc(pmbbo->rval);
}

/*************** Sequence Ram Records ******************/

/**		bo - loadSeq 	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_loadSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	evgSoftSeq* seq = (evgSoftSeq*)pbo->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->load();
}

/**		bo - unloadSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_unloadSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	evgSoftSeq* seq = (evgSoftSeq*)pbo->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->unload((dbCommon*)pbo);
}

/** 	bo - commitSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_commitSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	evgSoftSeq* seq = (evgSoftSeq*)pbo->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->commit((dbCommon*)pbo);
}

/**		bo - enableSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_enableSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	evgSoftSeq* seq = (evgSoftSeq*)pbo->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->enable();
}

/**		bo - disableSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_disableSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	evgSoftSeq* seq = (evgSoftSeq*)pbo->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->disable();
}

/**		bo - haltSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_haltSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	evgSoftSeq* seq = (evgSoftSeq*)pbo->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->halt();
}

/**		bo - Software Trigger	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_softTrig(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	evgSoftSeq* seq = (evgSoftSeq*)pbo->dpvt;
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	evgSeqRam* seqRam = seq->getSeqRam();
	if(!seqRam)
		throw std::runtime_error("ERROR: Failed to lookup EVG Seq RAM");

	return seqRam->setSoftTrig();
}

/**		waveform - seqLoaded	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_loadedSeq(waveformRecord* pwf) {
	evgSeqRamMgr* seqRamMgr = (evgSeqRamMgr*)pwf->dpvt;
	epicsUInt32* p = (epicsUInt32*)pwf->bptr;
	
	for(int i = 0; i < evgNumSeqRam; i++) {
		evgSoftSeq* seq = seqRamMgr->getSeqRam(i)->getSoftSeq();
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

common_dset devWfEvgTimeStampTick = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf,
    NULL,
    (DEVSUPFUN)write_wf_timeStampTick,
};
epicsExportAddress(dset, devWfEvgTimeStampTick);

common_dset devWfEvgTimeStampSec = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf,
    NULL,
    (DEVSUPFUN)write_wf_timeStampSec,
};
epicsExportAddress(dset, devWfEvgTimeStampSec);

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

common_dset devMbboEvgTrigSrc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbo,
    NULL,
    (DEVSUPFUN)write_mbbo_trigSrc,
};
epicsExportAddress(dset, devMbboEvgTrigSrc);

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

common_dset devBoEvgSoftTrig = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_softTrig,
};
epicsExportAddress(dset, devBoEvgSoftTrig);

common_dset devWfEvgLoadedSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf_loadedSeq,
    NULL,
    (DEVSUPFUN)write_wf_loadedSeq,
};
epicsExportAddress(dset, devWfEvgLoadedSeq);

};