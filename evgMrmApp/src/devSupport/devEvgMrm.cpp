#include <iostream>
#include <stdexcept>
#include <time.h>

#include <boRecord.h>
#include <stringinRecord.h>
#include <mbboRecord.h>
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
init_bo(boRecord* pbo) {
	long ret = init_record((dbCommon*)pbo, &pbo->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
init_si(stringinRecord* psi) {
	return init_record((dbCommon*)psi, &psi->inp);
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
init_mbbo(mbboRecord* pmbbo) {
	epicsStatus ret = init_record((dbCommon*)pmbbo, &pmbbo->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long
init_li(longinRecord* pli) {
	return init_record((dbCommon*)pli, &pli->inp);
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

/**  stringin - TimeStamp	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_si_ts(stringinRecord* psi) {
	if(!psi->dpvt)
		return -1;

	evgMrm* evg = (evgMrm*)psi->dpvt;
	timeval tv = evg->getTS();
	struct tm* ptm = localtime (&tv.tv_sec); 
	strftime(psi->val, sizeof(psi->val), 
                        "%a, %d %b %Y %H:%M:%S", ptm);

	return 0;
}

/**		bo - Update TimeStamp	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_syncTS(boRecord* pbo) {
	if(!pbo->dpvt)
		return -1;

	evgMrm* evg = (evgMrm*)pbo->dpvt;
	return evg->syncTS();
}

/** 	mbbo - TimeStamp Source **/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbbo(mbboRecord* pmbbo) {
	if(!pmbbo->dpvt)
		return -1;

	evgMrm* evg = (evgMrm*)pmbbo->dpvt;
	return evg->setupTS((TimeStampSrc)pmbbo->val);
}

/**	longin - Status	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_li(longinRecord* pli) {
	if(!pli->dpvt)
		return -1;

	evgMrm* evg = (evgMrm*)pli->dpvt;
	pli->val = evg->getStatus();
	return 0;
}

/** 	device support entry table 	**/
extern "C" {

common_dset devBoEvgEnable = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_Enable,
};
epicsExportAddress(dset, devBoEvgEnable);

common_dset devSiTimeStamp = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_si,
	NULL,
    (DEVSUPFUN)write_si_ts,
};
epicsExportAddress(dset, devSiTimeStamp);

common_dset devBoEvgSyncTS = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_syncTS,
};
epicsExportAddress(dset, devBoEvgSyncTS);

common_dset devMbboEvgTSsrc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbo,
    NULL,
    (DEVSUPFUN)write_mbbo,
};
epicsExportAddress(dset, devMbboEvgTSsrc);

common_dset devLiEvgStatus = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_li,
	NULL,
    (DEVSUPFUN)write_li,
};
epicsExportAddress(dset, devLiEvgStatus);

};


