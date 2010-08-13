#include <iostream>
#include <stdexcept>

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
		
		std::string parm(lnk->value.vmeio.parm);
		evgOutput* out = evg->getOutput(lnk->value.vmeio.signal, parm);
		if(!out)
			throw std::runtime_error("ERROR: Failed to lookup Output");

		pRec->dpvt = out;
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
	if(!pmbbo->dpvt)
		return -1;

	evgOutput* out = (evgOutput*)pmbbo->dpvt;
	return out->setOutMap(pmbbo->rval);
}


/** 	device support entry table 		**/
extern "C" {

common_dset devMbboEvgOutMap = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbo,
    NULL,
    (DEVSUPFUN)write_mbbo,
};
epicsExportAddress(dset, devMbboEvgOutMap);

};
