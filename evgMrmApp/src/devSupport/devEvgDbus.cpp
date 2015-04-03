#include <iostream>
#include <stdexcept>
#include <sstream>

#include <boRecord.h>
#include <mbboRecord.h>
#include <devSup.h>
#include <dbAccess.h>
#include <recGbl.h>
#include <errlog.h>
#include "mrf/databuf.h"
#include <epicsExport.h>

#include "devObj.h"

#include <evgInit.h>

/*returns: (0,2)=>(success,success no convert) */
static long 
init_bo_src_inp(boRecord* pbo) {
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
write_bo_src_inp(boRecord* pbo) {
    long ret = 0;

    try {
        evgInput* inp = (evgInput*)pbo->dpvt;
        if(!inp)
            throw std::runtime_error("Device pvt field not initialized");

        inp->setDbusMap(pbo->out.value.vmeio.signal, pbo->val);
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

common_dset devBoEvgDbusSrcInp = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_src_inp,
    NULL,
    (DEVSUPFUN)write_bo_src_inp,
};
epicsExportAddress(dset, devBoEvgDbusSrcInp);

};
