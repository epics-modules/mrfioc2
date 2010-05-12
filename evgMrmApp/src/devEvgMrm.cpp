#include <iostream>
#include <stdexcept>

#include <aoRecord.h>
#include <longoutRecord.h>
#include <boRecord.h>
#include <biRecord.h>

#include <devSup.h>
#include <dbAccess.h>
#include <epicsExport.h>

#include <evgInit.h>

static long 
init_record(dbCommon *pRec, DBLINK* lnk) {
	if(lnk->type != VME_IO) {
		errlogPrintf("ERROR: init_record: Hardware link not VME_IO\n");
		return S_db_badField;
	}
	
	evgMrm* evg = FindEvg(lnk->value.vmeio.card);
		
	if(!evg) {
		errlogPrintf("ERROR: Failed to lookup EVG%d\n", lnk->value.vmeio.card);
		return S_dev_noDevice;
	}

	pRec->dpvt = evg;
	return 2;

}

/**		ao - Clock Speed	**/
/*returns: (0,2)=>(success,success no convert)*/
static long 
init_ao_clkSpeed(aoRecord* pao) {
	return init_record((dbCommon*)pao, &pao->out);
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_ao_clkSpeed(aoRecord* pao) {
	if(pao->dpvt) {
		evgMrm* evg = (evgMrm*)pao->dpvt;
		return evg->setClockSpeed(pao->val);
	} else {
		errlogPrintf("ERROR: Record %s is uninitailized\n", pao->name);
		return -1;
	}
}


/**		lo - Clock Source	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
init_lo_clkSrc(longoutRecord* plo) {
	epicsStatus ret =  init_record((dbCommon*)plo, &plo->out);
	if(ret == 2)
		return 0;

	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_clkSrc(longoutRecord* plo) {
	if(plo->dpvt) {
		evgMrm* evg = (evgMrm*)plo->dpvt;
		return evg->setClockSource(plo->val);
	} else {
		errlogPrintf("ERROR: Record %s is uninitailized\n", plo->name);
		return -1;
	}
}

/**		bo - Software Event Enable	**/
/*returns: (0,2)=>(success,success no convert) 0==2	*/
static long 
init_bo_softEvtEna(boRecord* pbo) {
	return init_record((dbCommon*)pbo, &pbo->out);
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_bo_softEvtEna(boRecord* pbo) {
	if(pbo->dpvt) {
		evgMrm* evg = (evgMrm*)pbo->dpvt;
		return evg->softEvtEnable(pbo->val);
	} else {
		errlogPrintf("ERROR: Record %s is uninitailized\n", pbo->name);
		return -1;
	}
}


/**		bi -  Software Event Enabled	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
init_bi_softEvtEna(biRecord* pbi) {
	epicsStatus ret = init_record((dbCommon*)pbi, &pbi->inp);
	if(ret == 2)
		return 0;

	return ret;
}

/*returns: (0,2)=>(success,success no convert)*/
static long 
read_bi_softEvtEna(biRecord* pbi) {
	if(pbi->dpvt) {
		evgMrm* evg = (evgMrm*)pbi->dpvt;
		pbi->val = evg->softEvtEnabled();
		return 2;
	} else {
		errlogPrintf("ERROR: Record %s is uninitailized\n", pbi->name);
		return -1;
	}
}


/** 	longout - Software Event Code	**/
/*returns: (-1,0)=>(failure,success)*/
static long 
init_lo_softEvtCode(longoutRecord* plo) {
	long ret = init_record((dbCommon*)plo, &plo->out);
	if (ret == 2)
		ret = 0;
	
	return ret;
}

/*returns: (-1,0)=>(failure,success)*/
static long 
write_lo_softEvtCode(longoutRecord* plo) {
	if(plo->dpvt) {
		evgMrm* evg = (evgMrm*)plo->dpvt;
		return evg->setSoftEvtCode(plo->val);
	} else {
		errlogPrintf("ERROR: Record %s is uninitailized\n", plo->name);
		return -1;
	}
}


/** 	device support entry table 	**/
extern "C" {

/*		ao - Clock Speed	*/
struct {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   write_lo;       /* longout record dependent*/
	DEVSUPFUN 	special_linconv;
} devAoEvgClock = {
    6,
    NULL,
    NULL,
    (DEVSUPFUN)init_ao_clkSpeed,
    NULL,
    (DEVSUPFUN)write_ao_clkSpeed,
	NULL
};
epicsExportAddress(dset, devAoEvgClock);

/*		lo - Clock Source	*/
struct {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   write_lo;       /* longout record dependent*/
} devLoEvgClock = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo_clkSrc,
    NULL,
    (DEVSUPFUN)write_lo_clkSrc,
};
epicsExportAddress(dset, devLoEvgClock);

/*		bo -  Software Event Enable	*/
struct {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   write_bo;       /* bo record dependent*/
} devBoEvgSoftEvt = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bo_softEvtEna,
    NULL,
    (DEVSUPFUN)write_bo_softEvtEna,
};
epicsExportAddress(dset, devBoEvgSoftEvt);

/*		bi -  Software Event Enable	*/
struct {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   read_bi;        /* bi record dependent*/
} devBiEvgSoftEvt = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_bi_softEvtEna,
    NULL,
    (DEVSUPFUN)read_bi_softEvtEna,
};
epicsExportAddress(dset, devBiEvgSoftEvt);

/* 	longout - Software Event Code	*/
struct {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   write_lo;       /* longout record dependent*/
} devLoEvgSoftEvt = {
    5,
    NULL,
    NULL,
    (DEVSUPFUN)init_lo_softEvtCode,
    NULL,
    (DEVSUPFUN)write_lo_softEvtCode,
};
epicsExportAddress(dset, devLoEvgSoftEvt);

};


