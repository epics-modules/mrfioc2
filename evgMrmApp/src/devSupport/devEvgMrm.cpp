#include <iostream>
#include <stdexcept>

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
		evgMrm* evg = FindEvg(lnk->value.vmeio.card);		
		if(!evg)
			throw std::runtime_error("ERROR: Failed to lookup EVG");

		pRec->dpvt = evg;
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

/** 	Initialization 	**/
/*returns: (0,2)=>(success,success no convert) 0==2	*/
static long 
init_bo_Enable(boRecord* pbo) {
	long ret = init_record((dbCommon*)pbo, &pbo->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/**		bo - Enable EVG	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_Enable(boRecord* pbo) {
	if(!pbo->dpvt)
		return -1;

	evgMrm* evg = (evgMrm*)pbo->dpvt;
	return evg->enable(pbo->val);
}


/** 	device support entry table 	**/
extern "C" {

common_dset devBoEvgEnable = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_Enable,
    NULL,
    (DEVSUPFUN)write_bo_Enable,
};
epicsExportAddress(dset, devBoEvgEnable);

};


