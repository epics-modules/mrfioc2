#include <iostream>
#include <stdexcept>
#include <time.h>

#include <boRecord.h>
#include <longinRecord.h>
#include <longinRecord.h>
#include <stringinRecord.h>
#include <mbboRecord.h>
#include <mbbiRecord.h>

#include <devSup.h>
#include <dbAccess.h>
#include <recGbl.h>
#include <errlog.h>
#include <epicsExport.h>

#include "devObj.h"

#include <evgInit.h>

static long 
init_record(dbCommon *pRec, DBLINK* lnk) {
    long ret = 0;

    if(lnk->type != VME_IO) {
        errlogPrintf("ERROR: Hardware link not VME_IO : %s\n", pRec->name);
        return S_db_badField;
    }
    
    try {
        std::string parm(lnk->value.vmeio.parm);
        pRec->dpvt = mrf::Object::getObject(parm);
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

/**     Initialization     **/
/*returns: (0,2)=>(success,success no convert) 0==2    */
static long 
init_bo(boRecord* pbo) {
    long ret = init_record((dbCommon*)pbo, &pbo->out);
    if(ret == 0)
        ret = 2;

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
init_si(stringinRecord* psi) {
    return init_record((dbCommon*)psi, &psi->inp);
}

/*returns: (-1,0)=>(failure,success)*/
static long
write_bo_resetMxc(boRecord* pbo) {
    long ret = 0;

    try {
        evgMrm* evg = (evgMrm*)pbo->dpvt;
        if(!evg)
            throw std::runtime_error("Device pvt field not initialized");

        evg->resetMxc(pbo->val);
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
read_si_ts(stringinRecord* psi) {
    long ret = 0;

    try {
        evgMrm* evg = (evgMrm*)psi->dpvt;
        if(!evg)
            throw std::runtime_error("Device pvt field not initialized");

        epicsTime ts = evg->getTimestamp();
        ts.strftime(psi->val, sizeof(psi->val), 
                                                 "%a, %d %b %Y %H:%M:%S");

        switch(evg->m_alarmTimestamp) {
            case TS_ALARM_NONE:
                break;
            case TS_ALARM_MINOR:
                recGblSetSevr(psi, SOFT_ALARM, MINOR_ALARM);
                break;
            case TS_ALARM_MAJOR:
                recGblSetSevr(psi, TIMEOUT_ALARM, MAJOR_ALARM);
                break;
            default:
                errlogPrintf("ERROR: Wrong Timestamp alarm Status\n");
        }

        ret = 0;
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), psi->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), psi->name);
        ret = S_db_noMemory;
    }

    return ret;
}

static long 
get_ioint_info(int cmd, stringinRecord *psi, IOSCANPVT *ppvt) {
    evgMrm* evg = (evgMrm*)psi->dpvt;
    if(!evg) {
        errlogPrintf("ERROR: Device pvt field not initialized\n");
        return -1;
    }

    *ppvt = evg->ioScanTimestamp;

    return 0;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_syncTS(boRecord* pbo) {
    long ret = 0;

    try {
        evgMrm* evg = (evgMrm*)pbo->dpvt;
        if(!evg)
            throw std::runtime_error("Device pvt field not initialized");

        evg->syncTsRequest();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}


/**     device support entry table     **/
extern "C" {

common_dset devBoEvgResetMxc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_resetMxc,
};
epicsExportAddress(dset, devBoEvgResetMxc);

common_dset devSiTimeStamp = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_si,
    (DEVSUPFUN)get_ioint_info,
    (DEVSUPFUN)read_si_ts,
};
epicsExportAddress(dset, devSiTimeStamp);

common_dset devBoEvgSyncTS = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_syncTS,
};
epicsExportAddress(dset, devBoEvgSyncTS);

};

