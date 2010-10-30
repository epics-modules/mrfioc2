#include <iostream>
#include <stdexcept>
#include <sstream>

#include <boRecord.h>
#include <longoutRecord.h>

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
		return(S_db_badField);
	}
	
	try {
		evgMrm* evg = &evgmap.get(lnk->value.vmeio.card);
		if(!evg)
			throw std::runtime_error("Failed to lookup EVG");
			
		evgTrigEvt*  trigEvt = evg->getTrigEvt(lnk->value.vmeio.signal);
		pRec->dpvt = trigEvt;
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

/**		Initialization	**/
/*returns: (0,2)=>(success,success no convert) */
static long 
init_bo(boRecord* pbo) {
	long ret = init_record((dbCommon*)pbo, &pbo->out);
	if(ret == 0)
		ret = 2;

	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
init_lo(longoutRecord* plo) {
	long ret = init_record((dbCommon*)plo, &plo->out);
	if (ret == 2)
		ret = 0;
	
	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_ena(boRecord* pbo) {
	long ret = 0;

	try {
		evgTrigEvt* trigEvt = (evgTrigEvt*)pbo->dpvt;
		if(!trigEvt)
			throw std::runtime_error("Device pvt field not initialized");

		ret = trigEvt->enable(pbo->val);
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
write_lo_evtCode(longoutRecord* plo) {
	long ret = 0;
        unsigned long dummy;

	try {
		evgTrigEvt* trigEvt = (evgTrigEvt*)plo->dpvt;
		if(!trigEvt)
			throw std::runtime_error("Device pvt field not initialized");

		ret = trigEvt->setEvtCode(plo->val);
	} catch(std::runtime_error& e) {
		dummy = recGblSetSevr(plo, WRITE_ALARM, MAJOR_ALARM);
		errlogPrintf("ERROR: %s : %s\n", e.what(), plo->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), plo->name);
		ret = S_db_noMemory;
	}

	return ret;
}

/*returns: (0,2)=>(success,success no convert) */
static long 
init_bo_trigSrc_mxc(boRecord* pbo) {
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
		std::istringstream i(parm);
   		int mxcID;
   		if (!(i >> mxcID))
			throw std::runtime_error("Failed to read Mxc ID");

		evgMxc*  mxc = evg->getMuxCounter(mxcID);
		pbo->dpvt = mxc;
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
write_bo_trigSrc_mxc(boRecord* pbo) {
	long ret = 0;

	try {
		evgMxc* mxc = (evgMxc*)pbo->dpvt;
		if(!mxc)
			throw std::runtime_error("Device pvt field not initialized");

		ret = mxc->setMxcTrigEvtMap(pbo->out.value.vmeio.signal, pbo->val);
	} catch(std::runtime_error& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
		ret = S_dev_noDevice;
	} catch(std::exception& e) {
		errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
		ret = S_db_noMemory;
	}

	return ret;
}

/*returns: (0,2)=>(success,success no convert) */
static long 
init_bo_trigSrc_inp(boRecord* pbo) {
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
			if(a<58 && a >47)
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
write_bo_trigSrc_inp(boRecord* pbo) {
	long ret = 0;

	try {
		evgInput* inp = (evgInput*)pbo->dpvt;
		if(!inp)
			throw std::runtime_error("Device pvt field not initialized");

		ret = inp->setInpTrigEvtMap(pbo->out.value.vmeio.signal, pbo->val);
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

common_dset devBoEvgTrigEvt = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_ena,
};
epicsExportAddress(dset, devBoEvgTrigEvt);

common_dset devLoEvgTrigEvt = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_evtCode,
};
epicsExportAddress(dset, devLoEvgTrigEvt);

common_dset devBoEvgTrigEvtMxc0 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_mxc,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_mxc,
};
epicsExportAddress(dset, devBoEvgTrigEvtMxc0);

common_dset devBoEvgTrigEvtMxc1 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_mxc,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_mxc,
};
epicsExportAddress(dset, devBoEvgTrigEvtMxc1);

common_dset devBoEvgTrigEvtMxc2 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_mxc,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_mxc,
};
epicsExportAddress(dset, devBoEvgTrigEvtMxc2);

common_dset devBoEvgTrigEvtMxc3 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_mxc,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_mxc,
};
epicsExportAddress(dset, devBoEvgTrigEvtMxc3);

common_dset devBoEvgTrigEvtMxc4 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_mxc,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_mxc,
};
epicsExportAddress(dset, devBoEvgTrigEvtMxc4);

common_dset devBoEvgTrigEvtMxc5 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_mxc,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_mxc,
};
epicsExportAddress(dset, devBoEvgTrigEvtMxc5);

common_dset devBoEvgTrigEvtMxc6 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_mxc,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_mxc,
};
epicsExportAddress(dset, devBoEvgTrigEvtMxc6);

common_dset devBoEvgTrigEvtMxc7 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_mxc,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_mxc,
};
epicsExportAddress(dset, devBoEvgTrigEvtMxc7);

common_dset devBoEvgTrigEvtFpInp0 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_inp,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_inp,
};
epicsExportAddress(dset, devBoEvgTrigEvtFpInp0);

common_dset devBoEvgTrigEvtFpInp1 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_inp,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_inp,
};
epicsExportAddress(dset, devBoEvgTrigEvtFpInp1);

common_dset devBoEvgTrigEvtUnivInp0 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_inp,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_inp,
};
epicsExportAddress(dset, devBoEvgTrigEvtUnivInp0);

common_dset devBoEvgTrigEvtUnivInp1 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_inp,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_inp,
};
epicsExportAddress(dset, devBoEvgTrigEvtUnivInp1);

common_dset devBoEvgTrigEvtUnivInp2 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_inp,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_inp,
};
epicsExportAddress(dset, devBoEvgTrigEvtUnivInp2);

common_dset devBoEvgTrigEvtUnivInp3 = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc_inp,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_inp,
};
epicsExportAddress(dset, devBoEvgTrigEvtUnivInp3);
};

