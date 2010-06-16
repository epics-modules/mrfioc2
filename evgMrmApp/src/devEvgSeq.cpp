#include <iostream>
#include <stdexcept>

#include <longoutRecord.h>
#include <waveformRecord.h>
#include <mbboRecord.h>

#include <devSup.h>
#include <dbAccess.h>
#include <epicsExport.h>

#include "dsetshared.h"

#include <evgInit.h>
#include "evgSeqRam.h"
#include "evgRegMap.h"

static long 
init_record(dbCommon *pRec, DBLINK* lnk) {
	if(lnk->type != VME_IO) {
		errlogPrintf("ERROR: Hardware link not VME_IO\n");
		return(S_db_badField);
	}

	evgMrm* evg = FindEvg(lnk->value.vmeio.card);		
	if(!evg)
		throw std::runtime_error("ERROR: Failed to lookup device");
	
	evgSeqRamSup* seqRamSup = evg->getSeqRamSup();
	if(!seqRamSup)
		throw std::runtime_error("ERROR: Failed to lookup device");
 
	pRec->dpvt = seqRamSup;
	return 0;
}

/** 	longout - Initialization	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
init_lo(longoutRecord* plo) {
	return init_record((dbCommon*)plo, &plo->out);
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
write_lo_loadSeq(longoutRecord* plo) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)plo->dpvt;
	return seqRamSup->load(plo->val);
}

/**		longout - unloadSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_unloadSeq(longoutRecord* plo) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)plo->dpvt;
	return seqRamSup->unload(plo->val);
}

/** 	longout - commitSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_commitSeq(longoutRecord* plo) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)plo->dpvt;
	return seqRamSup->commit(plo->val, (dbCommon*)plo);
}

/**		longout - enableSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_enableSeq(longoutRecord* plo) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)plo->dpvt;
	return seqRamSup->enable(plo->val);
}

/**		longout - disableSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_disableSeq(longoutRecord* plo) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)plo->dpvt;
	return seqRamSup->disable(plo->val);
}


/*************** Sequence Records ******************/

/**		waveform - timeStamp	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_timeStamp(waveformRecord* pwf) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)pwf->dpvt;
	if(! seqRamSup)
		return -1;

	evgSequence* seq = seqRamSup->getSequence(pwf->inp.value.vmeio.signal);
	if(!seq)
		return -1;

	return seq->setTimeStamp((epicsUInt32*)pwf->bptr, pwf->nelm);
}

/**		waveform - eventCode	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_eventCode(waveformRecord* pwf) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)pwf->dpvt;
	if(! seqRamSup)
		return -1;
	
	evgSequence* seq = seqRamSup->getSequence(pwf->inp.value.vmeio.signal);
	if(!seq)
		return -1;

	return seq->setEventCode((epicsUInt8*)pwf->bptr, pwf->nelm);
}

/**		mbbo - runMode		**/
static long
write_mbbo_runMode(mbboRecord* pmbbo) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)pmbbo->dpvt;
	if(! seqRamSup)
		return -1;
	
	evgSequence* seq = seqRamSup->getSequence(pmbbo->out.value.vmeio.signal);
	if(!seq)
		return -1;

	return seq->setRunMode((SeqRunMode)pmbbo->val);
}


/*************** Sequence Ram Records ******************/

/**		waveform - seqLoaded	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_loadedSeq(waveformRecord* pwf) {
	evgSeqRamSup* sup = (evgSeqRamSup*)pwf->dpvt;

	epicsUInt32* p = (epicsUInt32*)pwf->bptr;

	for(int i = 0; i < evgNumSeqRam; i++) {
		evgSequence* seq = sup->getSeqRam(i)->getSequence();
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

common_dset devLoEvgLoadSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_loadSeq,
};
epicsExportAddress(dset, devLoEvgLoadSeq);


common_dset devLoEvgUnloadSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_unloadSeq,
};
epicsExportAddress(dset, devLoEvgUnloadSeq);


common_dset devLoEvgCommitSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_commitSeq,
};
epicsExportAddress(dset, devLoEvgCommitSeq);

common_dset devLoEvgEnableSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_enableSeq,
};
epicsExportAddress(dset, devLoEvgEnableSeq);

common_dset devLoEvgDisableSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_disableSeq,
};
epicsExportAddress(dset, devLoEvgDisableSeq);


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