#include <iostream>
#include <stdexcept>

#include <longoutRecord.h>
#include <boRecord.h>
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
		evgMrm* evg = &evgmap.get(lnk->value.vmeio.card);
		if(!evg)
			throw std::runtime_error("Failed to lookup EVG");
		
		evgSoftEvt* softEvt = evg->getSoftEvt();
		if(!softEvt)
			throw std::runtime_error("Failed to lookup EVG Soft Evt");
	
		pRec->dpvt = softEvt;
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

/** 	Initialization	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
init_lo(longoutRecord* plo) {
	return init_record((dbCommon*)plo, &plo->out);
}

/** 	longout - Software Event Code	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_setEvtCode(longoutRecord* plo) {
	long ret = 0;

	try {
		evgSoftEvt* softEvt = (evgSoftEvt*)plo->dpvt;
		if(!softEvt)
			throw std::runtime_error("Device pvt field not initialized");
		
		ret = softEvt->setEvtCode(plo->val);
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), plo->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), plo->name);
		ret = S_db_noMemory;
	}

	return ret;
}

/** 	device support entry table 	**/
extern "C" {
/* 	longout - Software Event Code	*/
common_dset devLoEvgSoftEvt = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_setEvtCode,
};
epicsExportAddress(dset, devLoEvgSoftEvt);

};
