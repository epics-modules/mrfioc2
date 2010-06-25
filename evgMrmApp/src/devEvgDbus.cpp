#include <iostream>
#include <stdexcept>

#include <mbboRecord.h>
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
			errlogPrintf("ERROR: Failed to lookup EVG");

		evgDbus* dbus = evg->getDbus(lnk->value.vmeio.signal);
		if(!dbus)
			errlogPrintf("ERROR: Failed to lookup Dbus");

		pRec->dpvt = dbus;
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


/** 	mbbo - Dbus Map **/
/*returns: (0,2)=>(success,success no convert)*/
static long 
init_mbbo(mbboRecord* pmbbo) {
	return init_record((dbCommon*)pmbbo, &pmbbo->out);
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbbo(mbboRecord* pmbbo) {
	evgDbus* dbus = (evgDbus*)pmbbo->dpvt;
	return dbus->setDbusMap(pmbbo->val);
}


/** 	device support entry table 		**/
extern "C" {

common_dset devMbboEvgDbusMap = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbo,
    NULL,
    (DEVSUPFUN)write_mbbo,
};
epicsExportAddress(dset, devMbboEvgDbusMap);

};
