#include <iostream>
#include <stdexcept>
#include <exception>

#include <biRecord.h>
#include <boRecord.h>
#include <longoutRecord.h>
#include <mbboDirectRecord.h>

#include <devSup.h>
#include <dbAccess.h>
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
		ret = 2;
	} catch(std::runtime_error& e) {
		errlogPrintf("%s : %s\n", e.what(), pRec->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("%s : %s\n", e.what(), pRec->name);
		ret = S_db_noMemory;
	}

	return ret;
}


/**		bo - Multiplexed Counter Polarity	**/
/*returns: (0,2)=>(success,success no convert) 0==2	*/
static long 
init_bo(boRecord* pbo) {
	return init_record((dbCommon*)pbo, &pbo->out);
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo(boRecord* pbo) {
	evgMxc* mxc = (evgMxc*)pbo->dpvt;
	return mxc->setMxcOutPolarity(pbo->val);
}


/**		bi -  Multiplexed Counter Status	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
init_bi(biRecord* pbi) {
	epicsStatus ret =  init_record((dbCommon*)pbi, &pbi->inp);
	if(ret == 2)
		return 0;

	return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
read_bi(biRecord* pbi) {
	evgMxc* mxc = (evgMxc*)pbi->dpvt;
	pbi->rval = mxc->getMxcOutStatus();
	return 0;
}


/** 	longout - Multiplexed Counter Prescalar	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
init_lo(longoutRecord* plo) {
	epicsUInt32 ret = init_record((dbCommon*)plo, &plo->out);
	if (ret == 2)
		ret = 0;
	
	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo(longoutRecord* plo) {
	evgMxc* mxc = (evgMxc*)plo->dpvt;
	return mxc->setMxcPrescaler(plo->val);
}


/** 	mbboDirect - Multiplexed Counter Event Trigger Map **/
/*returns: (0,2)=>(success,success no convert)*/
static long 
init_mbboD(mbboDirectRecord* pmbboD) {
	return init_record((dbCommon*)pmbboD, &pmbboD->out);
}

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
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo,
};
epicsExportAddress(dset, devLoEvgMxc);

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