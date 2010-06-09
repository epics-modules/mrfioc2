#include <iostream>
#include <stdexcept>

#include <longoutRecord.h>
#include <waveformRecord.h>

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
		errlogPrintf("ERROR: init_record: Hardware link not VME_IO\n");
		return(S_db_badField);
	}

	evgMrm* evg = FindEvg(lnk->value.vmeio.card);		
	if(!evg)
		throw std::runtime_error("Failed to lookup device");
	
	pRec->dpvt = evg;

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

// /** 	longout - get_ioint-info	**/
// static long
// get_ioint_info_commitSeq( int cmd,
// 				longoutRecord* plo,
// 				IOSCANPVT* ppvt) { 	
// 	evgMrm* evg = (evgMrm*)plo->dpvt;
// 	*ppvt = evg->irqStop0;
// 	return 0;
// }

/**		longout - loadSeq 	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_loadSeq(longoutRecord* plo) {
	//evgSeqRamSup* seqRamSup = (evgSeqRamSup*)plo->dpvt;
	evgMrm* evg = (evgMrm*)plo->dpvt;
	return (evg->getSeqRamSup())->load(plo->val);
}

/**		longout - unloadSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_unloadSeq(longoutRecord* plo) {
	evgMrm* evg = (evgMrm*)plo->dpvt;
	return (evg->getSeqRamSup())->unload(plo->val);
}

/** 	longout - commitSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_commitSeq(longoutRecord* plo) {
	evgMrm* evg = (evgMrm*)plo->dpvt;
	return (evg->getSeqRamSup())->commit(plo->val, (dbCommon*)plo);
}

/**		longout - enableSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_enableSeq(longoutRecord* plo) {
	evgMrm* evg = (evgMrm*)plo->dpvt;
	return (evg->getSeqRamSup())->enable(plo->val);
}

/**		longout - disableSeq	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_disableSeq(longoutRecord* plo) {
	evgMrm* evg = (evgMrm*)plo->dpvt;
	return (evg->getSeqRamSup())->disable(plo->val);
}

/**		waveform - timeStamp	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_timeStamp(waveformRecord* pwf) {
	evgMrm* evg = (evgMrm*)pwf->dpvt;
	evgSequence* seq = evg->getSeqRamSup()->getSequence(pwf->inp.value.vmeio.signal);
	if(!seq)
		return -1;

	return seq->setTimeStamp((epicsUInt32*)pwf->bptr, pwf->nelm);
}

/**		waveform - eventCode	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_eventCode(waveformRecord* pwf) {
	evgMrm* evg = (evgMrm*)pwf->dpvt;
	evgSequence* seq = evg->getSeqRamSup()->getSequence(pwf->inp.value.vmeio.signal);
	if(!seq)
		return -1;

	return seq->setEventCode((epicsUInt8*)pwf->bptr, pwf->nelm);
}

/**		waveform - seqLoaded	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_seqLoaded(waveformRecord* pwf) {
	evgMrm* evg = (evgMrm*)pwf->dpvt;
	evgSeqRamSup* sup = evg->getSeqRamSup();

	epicsUInt32* p = (epicsUInt32*)pwf->bptr;

	for(int i = 0; i < evgNumSeqRam; i++) {
		evgSequence* seq = sup->getSeqRam(i)->getSequence();
		if(seq)
			p[i] = seq->getId();
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

common_dset devWfEvgSeqLoaded = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf,
    NULL,
    (DEVSUPFUN)write_wf_seqLoaded,
};
epicsExportAddress(dset, devWfEvgSeqLoaded);

};