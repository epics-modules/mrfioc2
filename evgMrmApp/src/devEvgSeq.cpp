#include <iostream>
#include <stdexcept>

#include <longoutRecord.h>
#include <waveformRecord.h>

#include <devSup.h>
#include <dbAccess.h>
#include <epicsExport.h>

#include <evgInit.h>
#include <devEvg.h>

static long 
init_record(dbCommon *pRec, DBLINK* lnk) {
	if(lnk->type != VME_IO) {
		errlogPrintf("ERROR: init_record: Hardware link not VME_IO\n");
		return(S_db_badField);
	}

	evgMrm* evg = FindEvg(lnk->value.vmeio.card);		
	if(!evg)
		throw std::runtime_error("Failed to lookup device");
	
	pRec->dpvt = evg->getSeqRamSup();

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


/**		longout - Write 	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_loadSeq(longoutRecord* plo) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)plo->dpvt;
	return seqRamSup->load(plo->val);
}

static long 
write_lo_unloadSeq(longoutRecord* plo) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)plo->dpvt;
	return seqRamSup->unload(plo->val);
}

static long 
write_lo_commitSeq(longoutRecord* plo) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)plo->dpvt;
	return seqRamSup->commit(plo->val);
}

static long 
write_lo_disableSeq(longoutRecord* plo) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)plo->dpvt;
	return seqRamSup->disable(plo->val);
}

/**		waveform - Write	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_wf_timeStamp(waveformRecord* pwf) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)pwf->dpvt;
	evgSequence* seq = seqRamSup->getSequence(pwf->inp.value.vmeio.signal);
	if(!seq)
		return -1;

	return seq->setTimeStamp((epicsUInt32*)pwf->bptr, pwf->nelm);
}

static long 
write_wf_eventCode(waveformRecord* pwf) {
	evgSeqRamSup* seqRamSup = (evgSeqRamSup*)pwf->dpvt;
	evgSequence* seq = seqRamSup->getSequence(pwf->inp.value.vmeio.signal);
	if(!seq)
		return -1;

	return seq->setEventCode((epicsUInt8*)pwf->bptr, pwf->nelm);
}

/** 	device support entry table 		**/
extern "C" {

devLoEvg devLoEvgLoadSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_loadSeq,
};
epicsExportAddress(dset, devLoEvgLoadSeq);


devLoEvg devLoEvgUnloadSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_unloadSeq,
};
epicsExportAddress(dset, devLoEvgUnloadSeq);


devLoEvg devLoEvgCommitSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_commitSeq,
};
epicsExportAddress(dset, devLoEvgCommitSeq);


devLoEvg devLoEvgDisableSeq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_disableSeq,
};
epicsExportAddress(dset, devLoEvgDisableSeq);


devWfEvg devWfEvgTimeStamp = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf,
    NULL,
    (DEVSUPFUN)write_wf_timeStamp,
};
epicsExportAddress(dset, devWfEvgTimeStamp);


devWfEvg devWfEvgEventCode = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_wf,
    NULL,
    (DEVSUPFUN)write_wf_eventCode,
};
epicsExportAddress(dset, devWfEvgEventCode);

};