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
#include "evgRegMap.h"

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
	
		evgSeqMgr* seqMgr = evg->getSeqMgr();
		if(!seqMgr)
			throw std::runtime_error("ERROR: Failed to lookup EVG Seq Manager");

		pRec->dpvt = seqMgr;
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
	epicsStatus ret =  init_record((dbCommon*)pbo, &pbo->out);
	if(ret == 0)
		ret = 2;
	
	return ret;
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


/*************** Sequence Records ******************/

/**		bo - createSeq		**/
/*returns: (-1,0)=>(failure,success)*/
static long
write_bo_createSeq(boRecord* pbo) {
	if(!pbo->val)
		return 0;

	evgSeqMgr* seqMgr = (evgSeqMgr*)pbo->dpvt;
	return seqMgr->createSeq(pbo->out.value.vmeio.signal);
}

/**		waveform - timeStamp	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_timeStampTick(waveformRecord* pwf) {
	evgSeqMgr* seqMgr = (evgSeqMgr*)pwf->dpvt;

	evgSequence* seq = seqMgr->getSeq(pwf->inp.value.vmeio.signal);
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->setTimeStampTick((epicsUInt32*)pwf->bptr, pwf->nord);
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_timeStampSec(waveformRecord* pwf) {
	evgSeqMgr* seqMgr = (evgSeqMgr*)pwf->dpvt;

	evgSequence* seq = seqMgr->getSeq(pwf->inp.value.vmeio.signal);
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->setTimeStampSec((epicsFloat64*)pwf->bptr, pwf->nord);
}

/**		waveform - eventCode	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_eventCode(waveformRecord* pwf) {
	evgSeqMgr* seqMgr = (evgSeqMgr*)pwf->dpvt;

	evgSequence* seq = seqMgr->getSeq(pwf->inp.value.vmeio.signal);
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->setEventCode((epicsUInt8*)pwf->bptr, pwf->nord);
}

/**		mbbo - runMode		**/
/*returns: (0,2)=>(success,success no convert)*/
static long
write_mbbo_runMode(mbboRecord* pmbbo) {
	evgSeqMgr* seqMgr = (evgSeqMgr*)pmbbo->dpvt;

	evgSequence* seq = seqMgr->getSeq(pmbbo->out.value.vmeio.signal);
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->setRunMode((SeqRunMode)pmbbo->val);
}

/**		mbbo - trigSrc 	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbbo_trigSrc(mbboRecord* pmbbo) {
	evgSeqMgr* seqMgr = (evgSeqMgr*)pmbbo->dpvt;

	evgSequence* seq = seqMgr->getSeq(pmbbo->out.value.vmeio.signal);
	if(!seq)
		throw std::runtime_error("ERROR: Failed to lookup EVG Sequence");

	return seq->setTrigSrc(pmbbo->rval);
}


/** 	device support entry table 		**/
extern "C" {

common_dset devBoEvgCreateSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_createSeq,
};
epicsExportAddress(dset, devBoEvgCreateSeq);

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

};