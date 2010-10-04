#include <iostream>
#include <stdexcept>

#include <boRecord.h>
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
		return(S_db_badField);
	}
	
	try {
		evgMrm* evg = &evgmap.get(lnk->value.vmeio.card);
		if(!evg)
			throw std::runtime_error("Failed to lookup EVG");
			
		evgTrigEvt*  trigEvt = evg->getTrigEvt(lnk->value.vmeio.signal);
		pRec->dpvt = trigEvt;
		ret = 0;
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pRec->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pRec->name);
		ret = S_db_noMemory;
	}

	return ret;
}

/**		Initialization	**/
/*returns: (0,2)=>(success,success no convert) */
static long 
init_bo(boRecord* pbo) {
	long ret = init_record((dbCommon*)pbo, &pbo->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
init_lo(longoutRecord* plo) {
	long ret = init_record((dbCommon*)plo, &plo->out);
	if (ret == 2)
		ret = 0;
	
	return ret;
}

/**		bo - Event Trigger Enable	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo(boRecord* pbo) {
	long ret = 0;

	try {
		evgTrigEvt* trigEvt = (evgTrigEvt*)pbo->dpvt;
		if(!trigEvt)
			throw std::runtime_error("Device pvt field not initialized");

		ret = trigEvt->enable(pbo->val);
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
		ret = S_db_noMemory;
	}

	return ret;
}


/** 	longout - Event Trigger Code	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo(longoutRecord* plo) {
	long ret = 0;

	try {
		evgTrigEvt* trigEvt = (evgTrigEvt*)plo->dpvt;
		if(!trigEvt)
			throw std::runtime_error("Device pvt field not initialized");

		ret = trigEvt->setEvtCode(plo->val);
	} catch(std::runtime_error& e) {
		recGblSetSevr(plo, WRITE_ALARM, MAJOR_ALARM);
		errlogPrintf("ERROR: %s : %s\n", e.what(), plo->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), plo->name);
		ret = S_db_noMemory;
	}

	return ret;
}


/** 	device support entry table 		**/
extern "C" {

common_dset devBoEvgTrigEvt = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo,
};
epicsExportAddress(dset, devBoEvgTrigEvt);


common_dset devLoEvgTrigEvt = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo,
};
epicsExportAddress(dset, devLoEvgTrigEvt);

};
