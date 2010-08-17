#include <iostream>
#include <stdexcept>

#include <mbboDirectRecord.h>
#include <devSup.h>
#include <dbAccess.h>
#include <epicsExport.h>
#include <errlog.h>

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
			throw std::runtime_error("ERROR: Failed to lookup EVG");
		
		std::string parm(lnk->value.vmeio.parm);
		evgInput* inp = evg->getInput(lnk->value.vmeio.signal, parm);
		if(!inp)
			throw std::runtime_error("ERROR: Failed to lookup EVG Input");
	
		pRec->dpvt = inp;
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


/**	 Initialization **/
/*returns: (0,2)=>(success,success no convert)*/
static long 
init_mbboD(mbboDirectRecord* pmbboD) {
	long ret = init_record((dbCommon*)pmbboD, &pmbboD->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/*returns:(0,2)=>(success,success no convert*/
static long 
init_bo(boRecord* pbo) {
	long ret = init_record((dbCommon*)pbo, &pbo->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/**		mbboDirect - Input-Dbus Map	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbboD_inpDbus(mbboDirectRecord* pmbboD) {
	if(!pmbboD->dpvt)
		return -1;

	evgInput* inp = (evgInput*)pmbboD->dpvt;
	return inp->setInpDbusMap((epicsUInt32)pmbboD->val);
}

/**		mbboDirect - Input-TrigEvt Map	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbboD_inpTrigEvt(mbboDirectRecord* pmbboD) {
	if(!pmbboD->dpvt)
		return -1;

	evgInput* inp = (evgInput*)pmbboD->dpvt;
	return inp->setInpTrigEvtMap((epicsUInt32)pmbboD->val);
}

/**		bo - Input IRQ Enable	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_enaIRQ(boRecord* pbo) {
	if(!pbo->dpvt)
		return -1;

	evgInput* inp = (evgInput*)pbo->dpvt;
	return inp->enaExtIrq(pbo->val);
}

/** 	device support entry table 		**/
extern "C" {

common_dset devMbboDEvgInpDbusMap = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbboD,
    NULL,
    (DEVSUPFUN)write_mbboD_inpDbus,
};
epicsExportAddress(dset, devMbboDEvgInpDbusMap);

common_dset devMbboDEvgInpTrigEvtMap = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbboD,
    NULL,
    (DEVSUPFUN)write_mbboD_inpTrigEvt,
};
epicsExportAddress(dset, devMbboDEvgInpTrigEvtMap);

common_dset devBoDEvgInpEnaIrq = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_enaIRQ,
};
epicsExportAddress(dset, devBoDEvgInpEnaIrq);

};
