#include <iostream>
#include <stdexcept>
#include <sstream>

#include <boRecord.h>
#include <longoutRecord.h>

#include <devSup.h>
#include <dbAccess.h>
#include <recGbl.h>
#include <errlog.h>
#include <epicsExport.h>

#include "devObj.h"

#include <evgInit.h>

/*returns: (0,2)=>(success,success no convert) */
static long 
init_bo_trigSrc(boRecord* pbo) {
    long ret = 0;

    if(pbo->out.type != VME_IO) {
        errlogPrintf("ERROR: Hardware link not VME_IO : %s\n", pbo->name);
        return(S_db_badField);
    }
    
    try {
        std::string parm(pbo->out.value.vmeio.parm);
        pbo->dpvt = mrf::Object::getObject(parm);
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

        mxc->setTrigEvtMap(pbo->out.value.vmeio.signal, pbo->val);
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
write_bo_trigSrc_ac(boRecord* pbo) {
    long ret = 0;

    try {
        evgAcTrig* acTrig = (evgAcTrig*)pbo->dpvt;
        if(!acTrig)
            throw std::runtime_error("Device pvt field not initialized");

        acTrig->setTrigEvtMap(pbo->out.value.vmeio.signal, pbo->val);
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }
    ret = 0;
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

        inp->setTrigEvtMap(pbo->out.value.vmeio.signal, pbo->val);
    } catch(std::runtime_error& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_dev_noDevice;
    } catch(std::exception& e) {
        errlogPrintf("ERROR: %s : %s\n", e.what(), pbo->name);
        ret = S_db_noMemory;
    }

    return ret;
}

/**     device support entry table         **/
extern "C" {

common_dset devBoEvgTrigEvtMxc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_mxc,
};
epicsExportAddress(dset, devBoEvgTrigEvtMxc);

common_dset devBoEvgTrigEvtAc = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_ac,
};
epicsExportAddress(dset, devBoEvgTrigEvtAc);

common_dset devBoEvgTrigEvtInp = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_trigSrc,
    NULL,
    (DEVSUPFUN)write_bo_trigSrc_inp,
};
epicsExportAddress(dset, devBoEvgTrigEvtInp);

};

