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
		evgMrm* evg = &evgmap.get(lnk->value.vmeio.card);	
		if(!evg)
			throw std::runtime_error("Failed to lookup EVG");
	
		evgMxc* mxc = evg->getMuxCounter(lnk->value.vmeio.signal);
		pRec->dpvt = mxc;
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

/**		Initializaton	**/
/*returns: (0,2)=>(success,success no convert) 0==2	*/
static long 
init_bo(boRecord* pbo) {
	long ret = init_record((dbCommon*)pbo, &pbo->out);
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

/*returns: (0,2)=>(success,success no convert)*/
static long 
init_mbboD(mbboDirectRecord* pmbboD) {
	long ret = init_record((dbCommon*)pmbboD, &pmbboD->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/**		bo - Multiplexed Counter Polarity	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo(boRecord* pbo) {
	long ret = 0;

	try {
		evgMxc* mxc = (evgMxc*)pbo->dpvt;
		if(!mxc)
			throw std::runtime_error("Device pvt field not initialized");

		ret = mxc->setMxcOutPolarity(pbo->val);
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
		ret = S_db_noMemory;
	}

	return ret;
}


/**		bi -  Multiplexed Counter Status	**/
/*returns: (0,2)=>(success,success no convert)*/
static long 
read_bi(biRecord* pbi) {
	long ret = 0;

	try {
		evgMxc* mxc = (evgMxc*)pbi->dpvt;
		if(!mxc)
			throw std::runtime_error("Device pvt field not initialized");

		pbi->rval = mxc->getMxcOutStatus();
		ret = 0;
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbi->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbi->name);
		ret = S_db_noMemory;
	}

	return ret;
}


/** 	longin - Multiplexed Counter Prescalar	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_li(longinRecord* pli) {
	long ret = 0;

	try {
		evgMxc* mxc = (evgMxc*)pli->dpvt;
		if(!mxc)
			throw std::runtime_error("Device pvt field not initialized");

		pli->val = mxc->getMxcPrescaler();
		ret = 0;
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pli->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pli->name);
		ret = S_db_noMemory;
	}

	return ret;
}

/** 	ao - Multiplexed Counter Frequency	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_ao(aoRecord* pao) {
	long ret = 0;

	try {
		evgMxc* mxc = (evgMxc*)pao->dpvt;
		if(!mxc)
			throw std::runtime_error("Device pvt field not initialized");

		ret = mxc->setMxcFreq(pao->val);
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pao->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pao->name);
		ret = S_db_noMemory;
	}

	return ret;
}


/** 	ai - Multiplexed Counter Frequency	**/
/*returns: (0,2)=>(success,success no convert)*/
static long
write_ai(aiRecord* pai) {
	long ret = 0;

	try {
		evgMxc* mxc = (evgMxc*)pai->dpvt;
		if(!mxc)
			throw std::runtime_error("Device pvt field not initialized");

		pai->val = mxc->getMxcFreq();
		ret = 2;
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pai->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pai->name);
		ret = S_db_noMemory;
	}

	return ret;
}

/** 	mbboDirect - Multiplexed Counter Event Trigger Map **/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbboD(mbboDirectRecord* pmbboD) {
	long ret = 0;

	try {
		evgMxc* mxc = (evgMxc*)pmbboD->dpvt;
		if(!mxc)
			throw std::runtime_error("Device pvt field not initialized");

		ret = mxc->setMxcTrigEvtMap(pmbboD->val);
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pmbboD->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pmbboD->name);
		ret = S_db_noMemory;
	}

	return ret;
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
