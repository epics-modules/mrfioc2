#include <iostream>
#include <stdexcept>
#include <time.h>

#include <boRecord.h>
#include <stringinRecord.h>
#include <mbboRecord.h>
#include <mbbiRecord.h>

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
    
        pRec->dpvt = evg;
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

/*returns: (0,2)=>(success,success no convert)*/
static long 
init_mbbo(mbboRecord* pmbbo) {
    epicsStatus ret = init_record((dbCommon*)pmbbo, &pmbbo->out);
    if(ret == 0)
        ret = 2;

    return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
init_mbbi(mbbiRecord* pmbbi) {
    epicsStatus ret = init_record((dbCommon*)pmbbi, &pmbbi->inp);
    if(ret == 0)
        ret = 2;

    return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long
init_li(longinRecord* pli) {
    return init_record((dbCommon*)pli, &pli->inp);
}

/**        bo - Enable EVG    **/
/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_Enable(boRecord* pbo) {
    long ret = 0;
    
    try {
        evgMrm* evg = (evgMrm*)pbo->dpvt;
        if(!evg)
            throw std::runtime_error("Device pvt field not initialized");

        ret = evg->enable(pbo->val);
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/**    stringin - TimeStamp    **/
/*returns: (-1,0)=>(failure,success)*/
static long 
read_si_ts(stringinRecord* psi) {
    long ret = 0;
                unsigned long dummy;

    try {
        evgMrm* evg = (evgMrm*)psi->dpvt;
        if(!evg)
            throw std::runtime_error("Device pvt field not initialized");

        epicsTime ts = evg->getTS();
        ts.strftime(psi->val, sizeof(psi->val), 
                                                 "%a, %d %b %Y %H:%M:%S");

        switch(evg->m_alarmTS) {
            case TS_ALARM_NONE:
                break;
            case TS_ALARM_MINOR:
                dummy = recGblSetSevr(psi, SOFT_ALARM, MINOR_ALARM);
                break;
            case TS_ALARM_MAJOR:
                dummy = recGblSetSevr(psi, TIMEOUT_ALARM, MAJOR_ALARM);
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

    *ppvt = evg->ioScanTS;

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

        ret = evg->syncTsRequest();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
write_mbbo_tsInpType(mbboRecord* pmbbo) {
    long ret = 2;

    try {
        evgMrm* evg = (evgMrm*)pmbbo->dpvt;
        if(!evg)
            throw std::runtime_error("Device pvt field not initialized");

        ret = evg->setTsInpType((InputType)pmbbo->val);
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
write_mbbo_tsInpNum(mbboRecord* pmbbo) {
    long ret = 2;

    try {
        evgMrm* evg = (evgMrm*)pmbbo->dpvt;
        if(!evg)
            throw std::runtime_error("Device pvt field not initialized");

        ret = evg->setTsInpNum(pmbbo->val);
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
read_mbbi_tsInpType(mbbiRecord* pmbbi) {
    long ret = 2;

    try {
        evgMrm* evg = (evgMrm*)pmbbi->dpvt;
        if(!evg)
            throw std::runtime_error("Device pvt field not initialized");

        pmbbi->val = evg->getTsInpType();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbi->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbi->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
read_mbbi_tsInpNum(mbbiRecord* pmbbi) {
    long ret = 2;

    try {
        evgMrm* evg = (evgMrm*)pmbbi->dpvt;
        if(!evg)
            throw std::runtime_error("Device pvt field not initialized");

        pmbbi->val = evg->getTsInpNum();
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbi->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pmbbi->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/**    longin - Status    **/
/*returns: (-1,0)=>(failure,success)*/
static long 
read_li(longinRecord* pli) {
    long ret = 0;
    
    try {
        evgMrm* evg = (evgMrm*)pli->dpvt;
        if(!evg)
            throw std::runtime_error("Device pvt field not initialized");

        pli->val = evg->getStatus();
        ret = 0;
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pli->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pli->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/**     device support entry table     **/
extern "C" {

common_dset devBoEvgEnable = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo,
    NULL,
    (DEVSUPFUN)write_bo_Enable,
};
epicsExportAddress(dset, devBoEvgEnable);

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

common_dset devMbboEvgTsInpType = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbo,
    NULL,
    (DEVSUPFUN)write_mbbo_tsInpType,
};
epicsExportAddress(dset, devMbboEvgTsInpType);

common_dset devMbboEvgTsInpNum = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbo,
    NULL,
    (DEVSUPFUN)write_mbbo_tsInpNum,
};
epicsExportAddress(dset, devMbboEvgTsInpNum);

common_dset devMbbiEvgTsInpType = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbi,
    NULL,
    (DEVSUPFUN)read_mbbi_tsInpType,
};
epicsExportAddress(dset, devMbbiEvgTsInpType);

common_dset devMbbiEvgTsInpNum = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_mbbi,
    NULL,
    (DEVSUPFUN)read_mbbi_tsInpNum,
};
epicsExportAddress(dset, devMbbiEvgTsInpNum);

common_dset devLiEvgStatus = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_li,
    NULL,
    (DEVSUPFUN)read_li,
};
epicsExportAddress(dset, devLiEvgStatus);

};


// 