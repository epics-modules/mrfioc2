#include <iostream>
#include <stdexcept>
#include <exception>

#include <biRecord.h>
#include <boRecord.h>
#include <longoutRecord.h>
#include <mbboDirectRecord.h>

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
		errlogPrintf("ERROR: PV: %s\nHardware link not VME_IO\n", pRec->name);
		return S_db_badField;
	}

	try {
		evgMrm* evg = FindEvg(lnk->value.vmeio.card);		
		if(!evg)
			throw std::runtime_error("ERROR: Failed to lookup EVG");

		evgMxc* mxc = evg->getMuxCounter(lnk->value.vmeio.signal);
		if(!mxc)
			throw std::runtime_error("ERROR: Failed to lookup MXC");

		pRec->dpvt = mxc;
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

/**		Initializaton	**/
/*returns: (0,2)=>(success,success no convert) 0==2	*/
static long 
init_bo(boRecord* pbo) {
	epicsStatus ret = init_record((dbCommon*)pbo, &pbo->out);
	if (ret == 0)
		ret = 2;
	
	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
init_bi(biRecord* pbi) {
	return init_record((dbCommon*)pbi, &pbi->inp);
	
}

/*returns: (-1,0)=>(failure,success)*/
static long 
init_li(longinRecord* pli) {
	return init_record((dbCommon*)pli, &pli->inp);
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
init_ao(aoRecord* pao) {
	epicsStatus ret = init_record((dbCommon*)pao, &pao->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
init_ai(aiRecord* pai) {
	return init_record((dbCommon*)pai, &pai->inp);
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
init_mbboD(mbboDirectRecord* pmbboD) {
	epicsStatus ret = init_record((dbCommon*)pmbboD, &pmbboD->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/**		bo - Multiplexed Counter Polarity	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo(boRecord* pbo) {
	evgMxc* mxc = (evgMxc*)pbo->dpvt;
	return mxc->setMxcOutPolarity(pbo->val);
}


/**		bi -  Multiplexed Counter Status	**/
/*returns: (0,2)=>(success,success no convert)*/
static long 
read_bi(biRecord* pbi) {
	evgMxc* mxc = (evgMxc*)pbi->dpvt;
	pbi->rval = mxc->getMxcOutStatus();
	return 0;
}


/** 	longin - Multiplexed Counter Prescalar	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_li(longinRecord* pli) {
	evgMxc* mxc = (evgMxc*)pli->dpvt;
	pli->val = mxc->getMxcPrescaler();
	return 0;
}

/** 	ao - Multiplexed Counter Frequency	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_ao(aoRecord* pao) {
	evgMxc* mxc = (evgMxc*)pao->dpvt;
	return mxc->setMxcFreq(pao->val);
}


/** 	ai - Multiplexed Counter Frequency	**/
/*returns: (0,2)=>(success,success no convert)*/
static long 
write_ai(aiRecord* pai) {
	evgMxc* mxc = (evgMxc*)pai->dpvt;
	pai->val = mxc->getMxcFreq();
	return 2;
}

/** 	mbboDirect - Multiplexed Counter Event Trigger Map **/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbboD(mbboDirectRecord* pmbboD) {
	evgMxc* mxc = (evgMxc*)pmbboD->dpvt;
	return mxc->setMxcTrigEvtMap(pmbboD->val);
}


/** 	device support entry table 		**/
extern "C" {

common_dset devBoEvgMxc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo,
};
epicsExportAddress(dset, devBoEvgMxc);

common_dset devBiEvgMxc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bi,
    NULL,
    (DEVSUPFUN)read_bi,
};
epicsExportAddress(dset, devBiEvgMxc);

common_dset devLoEvgMxc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_li,
    NULL,
    (DEVSUPFUN)write_li,
};
epicsExportAddress(dset, devLoEvgMxc);

common_dset devAoEvgMxc = {
    6,
    NULL,
    NULL,
    (DEVSUPFUN)init_ao,
    NULL,
    (DEVSUPFUN)write_ao,
	NULL
};
epicsExportAddress(dset, devAoEvgMxc);

common_dset devAiEvgMxc = {
    6,
    NULL,
    NULL,
    (DEVSUPFUN)init_ai,
    NULL,
    (DEVSUPFUN)write_ai,
	NULL
};
epicsExportAddress(dset, devAiEvgMxc);

common_dset devMbboDEvgMxc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbboD,
    NULL,
    (DEVSUPFUN)write_mbboD,
};
epicsExportAddress(dset, devMbboDEvgMxc);

};
