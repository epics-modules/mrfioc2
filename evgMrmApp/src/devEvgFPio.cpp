#include <iostream>
#include <stdexcept>

#include <longoutRecord.h>

#include <devSup.h>
#include <dbAccess.h>
#include <epicsExport.h>

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
	
	std::string parm(lnk->value.vmeio.parm);
	evgFPio* io = evg->getFPio(lnk->value.vmeio.signal, parm);
	pRec->dpvt = io;
	return 2;
}

/** 	longout - Multiplexed Counter Prescalar	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
init_lo(longoutRecord* plo) {
	epicsUInt32 ret = init_record((dbCommon*)plo, &plo->out);
	if (ret == 2)
		ret = 0;
	
	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo(longoutRecord* plo) {
	evgFPio* io = (evgFPio*)plo->dpvt;
	return io->setIOMap(plo->val);
}


/** 	device support entry table 		**/
extern "C" {

struct {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   write_lo;       /* longout record dependent*/
} devLoEvgFPioMap = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo,
    NULL,
    (DEVSUPFUN)write_lo,
};
epicsExportAddress(dset, devLoEvgFPioMap);

};
