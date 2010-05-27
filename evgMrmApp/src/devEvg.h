#include <iostream>
#include <devSup.h>

extern "C" {

struct devLoEvg {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   write_lo;       /* longout record dependent*/
};

struct devAoEvg {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   write_lo;       /* longout record dependent*/
	DEVSUPFUN 	special_linconv;
};

struct devBoEvg {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   write_bo;       /* bo record dependent*/
};

struct devBiEvg {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   read_bi;        /* bi record dependent*/
};

struct devWfEvg {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   read_wf;       /* waveform record dependent*/
};

struct devMbboEvg {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   write_mbbo;     /* mbbo record dependent*/
};

struct devMbboDEvg {
    long        number;         /* number of support routines*/
    DEVSUPFUN   report;         /* print report*/
    DEVSUPFUN   init;           /* init support layer*/
    DEVSUPFUN   init_record;    /* init device for particular record*/
    DEVSUPFUN   get_ioint_info; /* get io interrupt information*/
    DEVSUPFUN   write_mbboD;       /* mbboDirect record dependent*/
};

};