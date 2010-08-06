#include <iostream>
#include <stdexcept>

#include <aoRecord.h>
#include <longoutRecord.h>
#include <devSup.h>
#include <dbAccess.h>
#include <errlog.h>
#include <epicsExport.h>

#include "dsetshared.h"

#include <evgInit.h>

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

		evgEvtClk* evtClk = evg->getEvtClk();
		if(!evtClk)
			throw std::runtime_error("ERROR: Failed to lookup EVG Event Clock");

		pRec->dpvt = evtClk;
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

/**		Initialization	**/
/*returns: (0,2)=>(success,success no convert) */
static long 
init_bo(boRecord* pbo) {
	long ret = init_record((dbCommon*)pbo, &pbo->out);
	if (ret == 0)
		ret = 2;
	
	return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
init_ao(aoRecord* pao) {
	long ret = init_record((dbCommon*)pao, &pao->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
init_ai(aiRecord* pai) {
	return init_record((dbCommon*)pai, &pai->inp);
}

/*returns: (-1,0)=>(failure,success)*/
static long 
init_lo(longoutRecord* plo) {
	return init_record((dbCommon*)plo, &plo->out);
}

/**		bo - Event Clock Source	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_evtClkSrc(boRecord* pbo) {
	if(!pbo->dpvt)
		return -1;

	evgEvtClk* evtClk = (evgEvtClk*)pbo->dpvt;
	return evtClk->setEvtClkSrc(pbo->val);
}

/**		ao - RF Input Frequency	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_ao_RFref(aoRecord* pao) {
	if(!pao->dpvt)
		return -1;

	evgEvtClk* evtClk = (evgEvtClk*)pao->dpvt;
	return evtClk->setRFref(pao->val);
}

/**		lo - RF Divider	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_RFdiv(longoutRecord* plo) {
	if(!plo->dpvt)
		return -1;

	evgEvtClk* evtClk = (evgEvtClk*)plo->dpvt;
	return evtClk->setRFdiv(plo->val);
}

/**		ao - Frac Synth Frequency	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_ao_fracSynFreq(aoRecord* pao) {
	if(!pao->dpvt)
		return -1;

	evgEvtClk* evtClk = (evgEvtClk*)pao->dpvt;
	return evtClk->setFracSynFreq(pao->val);
}

/**		ai - Evt Clock Speed	**/
/*returns: (0,2)=>(success,success no convert)*/
static long 
write_ai_evtClkSpeed(aiRecord* pai) {
	if(!pai->dpvt)
		return -1;

	evgEvtClk* evtClk = (evgEvtClk*)pai->dpvt;
	pai->val = evtClk->getEvtClkSpeed();
	return 2;
}


/** 	device support entry table 	**/
extern "C" {
common_dset devBoEvgEvtClkSrc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_evtClkSrc,
};
epicsExportAddress(dset, devBoEvgEvtClkSrc);

common_dset devAoEvgRFref = {
    6,
    NULL,
    NULL,
    (DEVSUPFUN)init_ao,
    NULL,
    (DEVSUPFUN)write_ao_RFref,
	NULL
};
epicsExportAddress(dset, devAoEvgRFref);

common_dset devLoEvgRFdiv = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_RFdiv,
};
epicsExportAddress(dset, devLoEvgRFdiv);

common_dset devAoEvgFracSynFreq = {
    6,
    NULL,
    NULL,
    (DEVSUPFUN)init_ao,
    NULL,
    (DEVSUPFUN)write_ao_fracSynFreq,
	NULL
};
epicsExportAddress(dset, devAoEvgFracSynFreq);

common_dset devAiEvgEvtClkSpeed = {
    6,
    NULL,
    NULL,
    (DEVSUPFUN)init_ai,
    NULL,
    (DEVSUPFUN)write_ai_evtClkSpeed,
	NULL
};
epicsExportAddress(dset, devAiEvgEvtClkSpeed);

};