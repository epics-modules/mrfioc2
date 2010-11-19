#include <iostream>
#include <stdexcept>
#include <sstream>

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
		evgMrm* evg = &evgmap.get(lnk->value.vmeio.card);		
		if(!evg)
			throw std::runtime_error("Failed to lookup EVG");

		evgDbus* dbus = evg->getDbus(lnk->value.vmeio.signal);
		pRec->dpvt = dbus;
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

/*returns: (0,2)=>(success,success no convert)*/
static long 
init_mbbo(mbboRecord* pmbbo) {
	long ret =  init_record((dbCommon*)pmbbo, &pmbbo->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_mbbo_map(mbboRecord* pmbbo) {
	long ret = 0;
	
	try {
		evgDbus* dbus = (evgDbus*)pmbbo->dpvt;
		if(!dbus)
			throw std::runtime_error("Device pvt field not initialized");

		ret = dbus->setDbusMap(pmbbo->val);
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
		ret = S_db_noMemory;
	}

	return ret;
}

/*returns: (0,2)=>(success,success no convert) */
static long 
init_bo_inp(boRecord* pbo) {
	long ret = 0;

	if(pbo->out.type != VME_IO) {
		errlogPrintf("ERROR: Hardware link not VME_IO : %s\n", pbo->name);
		return(S_db_badField);
	}
	
	try {
		evgMrm* evg = &evgmap.get(pbo->out.value.vmeio.card);
		if(!evg)
			throw std::runtime_error("Failed to lookup EVG");

		std::string parm(pbo->out.value.vmeio.parm);

		unsigned int i = 0;
		for(; i < parm.size(); i++) {
			char a = parm[i];
			if(a<58 && a>47)
				break;
		}
	
		std::string strInpNum = parm.substr(i);
		std::string strInpType = parm.substr(0,i);

		int inpNum;
		InputType inpType;

		std::istringstream iss(strInpNum);
   		if (!(iss >> inpNum))
			throw std::runtime_error("Failed to read Input Number");

		inpType = (InputType)InpStrToEnum[strInpType];

		evgInput*  inp = evg->getInput(inpNum, inpType);
		pbo->dpvt = inp;
		ret = 2;
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
		ret = S_db_noMemory;
	}

	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_inp(boRecord* pbo) {
	long ret = 0;

	try {
		evgInput* inp = (evgInput*)pbo->dpvt;
		if(!inp)
			throw std::runtime_error("Device pvt field not initialized");

		ret = inp->setInpDbusMap(pbo->out.value.vmeio.signal, pbo->val);
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
		ret = S_db_noMemory;
	}

	return ret;
}


/** 	device support entry table 		**/
extern "C" {

common_dset devMbboEvgDbusMap = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbo,
    NULL,
    (DEVSUPFUN)write_mbbo_map,
};
epicsExportAddress(dset, devMbboEvgDbusMap);

common_dset devBoEvgDbusInp = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_inp,
    NULL,
    (DEVSUPFUN)write_bo_inp,
};
epicsExportAddress(dset, devBoEvgDbusInp);

};
