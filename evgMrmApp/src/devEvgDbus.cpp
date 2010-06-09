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
	if(lnk->type != VME_IO) {
		errlogPrintf("ERROR: init_record: Hardware link not VME_IO\n");
		return(S_db_badField);
	}

	evgMrm* evg = FindEvg(lnk->value.vmeio.card);		
	if(!evg)
		throw std::runtime_error("Failed to lookup device");

	evgDbus* dbus = evg->getDbus(lnk->value.vmeio.signal);
	pRec->dpvt = dbus;
	return 2;
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
	if(! dbus) {
		printf("ERROR: Dbus not initialized.\n");
		return -1;
	}
		
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