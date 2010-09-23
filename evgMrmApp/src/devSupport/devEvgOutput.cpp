#include <iostream>
#include <stdexcept>
#include <map>

#include <mbboRecord.h>
#include <devSup.h>
#include <dbAccess.h>
#include <errlog.h>
#include <epicsExport.h>

#include "dsetshared.h"

#include <evgInit.h>

static std::map<std::string, epicsUInt32> strToEnum;

static long 
init(int after) {
	if(!after) {
		strToEnum["None_Output"] = 0;
		strToEnum["FP_Output"] = 1;
		strToEnum["Univ_Output"] = 2;
	}
	
	return 0;
}

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
		
		std::string parm(lnk->value.vmeio.parm);
		evgOutput* out = evg->getOutput(lnk->value.vmeio.signal, 
										(OutputType)strToEnum[parm]);
		pRec->dpvt = out;	
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

/**		Initialization 	**/
/*returns: (0,2)=>(success,success no convert)*/
static long 
init_mbbo(mbboRecord* pmbbo) {
	epicsStatus ret = init_record((dbCommon*)pmbbo, &pmbbo->out);
	if(ret == 0)
		ret = 2;
	return ret;
}

/** 	mbbo - Output Map **/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbbo(mbboRecord* pmbbo) {
	long ret = 0;

	try {
		evgOutput* out = (evgOutput*)pmbbo->dpvt;
		if(!out)
			throw std::runtime_error("Device pvt field not initialized");

		ret = out->setOutMap(pmbbo->rval);
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
		ret = S_db_noMemory;
	}

	return ret;
}


/** 	device support entry table 		**/
extern "C" {

common_dset devMbboEvgOutMap = {
	5,
	NULL,
	(DEVSUPFUN)init,
	(DEVSUPFUN)init_mbbo,
	NULL,
	(DEVSUPFUN)write_mbbo,
};
epicsExportAddress(dset, devMbboEvgOutMap);

};
