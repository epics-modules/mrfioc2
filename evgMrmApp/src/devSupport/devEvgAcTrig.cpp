#include <iostream>
#include <stdexcept>

#include <longoutRecord.h>
#include <aoRecord.h>
#include <boRecord.h>

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
		
        evgAcTrig* acTrig = evg->getAcTrig();
        if(!acTrig)
            throw std::runtime_error("Failed to lookup EVG AC Trigger");
	
        pRec->dpvt = acTrig;
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

/** 	Initialization	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
init_lo(longoutRecord* plo) {
    return init_record((dbCommon*)plo, &plo->out);
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
init_ao(aoRecord* pao) {
    long ret = init_record((dbCommon*)pao, &pao->out);
    if(ret == 0)
        ret = 2;

    return ret;
}

/*returns: (0,2)=>(success,success no convert) */
static long 
init_bo(boRecord* pbo) {
    long ret = init_record((dbCommon*)pbo, &pbo->out);
    if (ret == 0)
        ret = 2;
	
    return ret;
}

/**        Read/Write        **/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_divider(longoutRecord* plo) {
    long ret = 0;
    unsigned long dummy;

    try {
       evgAcTrig* acTrig = (evgAcTrig*)plo->dpvt;
        if(!acTrig)
             throw std::runtime_error("Device pvt field not initialized");
		
        ret = acTrig->setDivider(plo->val);
    } catch(std::runtime_error& e) {
        dummy = recGblSetSevr(plo, READ_ALARM, MAJOR_ALARM);
        errlogPrintf("ERROR: %s : %s\n", e.what(), plo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), plo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_ao_phase(aoRecord* pao) {
    long ret = 0;
    unsigned long dummy;

    try {
       evgAcTrig* acTrig = (evgAcTrig*)pao->dpvt;
        if(!acTrig)
             throw std::runtime_error("Device pvt field not initialized");
		
        ret = acTrig->setPhase(pao->val);
    } catch(std::runtime_error& e) {
        dummy = recGblSetSevr(pao, READ_ALARM, MAJOR_ALARM);
        errlogPrintf("ERROR: %s : %s\n", e.what(), pao->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pao->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_bypass(boRecord* pbo) {
    long ret = 0;
    unsigned long dummy;

    try {
        evgAcTrig* acTrig = (evgAcTrig*)pbo->dpvt;
        if(!acTrig)
            throw std::runtime_error("Device pvt field not initialized");
		
        ret = acTrig->bypass(pbo->val);
    } catch(std::runtime_error& e) {
        dummy = recGblSetSevr(pbo, READ_ALARM, MAJOR_ALARM);
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
write_bo_syncSrc(boRecord* pbo) {
    long ret = 0;
    unsigned long dummy;

    try {
        evgAcTrig* acTrig = (evgAcTrig*)pbo->dpvt;
        if(!acTrig)
            throw std::runtime_error("Device pvt field not initialized");
		
        ret = acTrig->setSyncSrc(pbo->val);
    } catch(std::runtime_error& e) {
        dummy = recGblSetSevr(pbo, READ_ALARM, MAJOR_ALARM);
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/** 	device support entry table 	**/
extern "C" {
common_dset devLoEvgAcDivider = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo_divider,
};
epicsExportAddress(dset, devLoEvgAcDivider);

common_dset devAoEvgAcPhase = {
    6,
    NULL,
    NULL,
    (DEVSUPFUN)init_ao,
    NULL,
    (DEVSUPFUN)write_ao_phase,
    NULL,
};
epicsExportAddress(dset, devAoEvgAcPhase);

common_dset devBoEvgAcBypass = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_bypass,
};
epicsExportAddress(dset, devBoEvgAcBypass);

common_dset devBoEvgAcSyncSrc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_syncSrc,
};
epicsExportAddress(dset, devBoEvgAcSyncSrc);

};
