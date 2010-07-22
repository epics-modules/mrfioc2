#include <iostream>
#include <stdexcept>

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
		errlogPrintf("ERROR: Hardware link not VME_IO : %s\n", pRec->name);
		return S_db_badField;
	}

	try {
		evgMrm* evg = FindEvg(lnk->value.vmeio.card);		
		if(!evg)
			throw std::runtime_error("ERROR: Failed to lookup EVG");

		std::string parm(lnk->value.vmeio.parm);
		evgInput* inp = evg->getInput(lnk->value.vmeio.signal, parm);
		if(!inp)
			throw std::runtime_error("ERROR: Failed to lookup EVG Input");
	
		pRec->dpvt = inp;
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


/** 	mbboDirect - Initialization **/
/*returns: (0,2)=>(success,success no convert)*/
static long 
init_mbboD(mbboDirectRecord* pmbboD) {
	return init_record((dbCommon*)pmbboD, &pmbboD->out);
}

/*returns:(0,2)=>(success,success no convert*/
static long 
init_bo(boRecord* pbo) {
	return init_record((dbCommon*)pbo, &pbo->out);
}

/**		mbboDirect - Input-Dbus Map	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbboD_inpDbus(mbboDirectRecord* pmbboD) {
	evgInput* inp = (evgInput*)pmbboD->dpvt;
	return inp->setInpDbusMap((epicsUInt32)pmbboD->val);
}

/**		mbboDirect - Input-TrigEvt Map	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbboD_inpTrigEvt(mbboDirectRecord* pmbboD) {
	evgInput* inp = (evgInput*)pmbboD->dpvt;
	return inp->setInpTrigEvtMap((epicsUInt32)pmbboD->val);
}

/**		bo - Input IRQ Enable	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_enaIRQ(boRecord* pbo) {
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