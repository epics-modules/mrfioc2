/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/* devMbboDirectSoft.c */
/* base/src/dev $Revision-Id$ */
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Matthew Needes
 *      Date:            10-08-93
 *
 * Revised for mrfioc2 to restore B* fields from VAL in init_record
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "mbboDirectRecord.h"
#include "epicsExport.h"

/* Create the dset for devMbboSoft */
static long init_record(mbboDirectRecord *prec);
static long write_mbbo(mbboDirectRecord *prec);
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_mbbo;
}devMbboDirectRestore={
	5,
	NULL,
	NULL,
	init_record,
	NULL,
	write_mbbo
};
epicsExportAddress(dset,devMbboDirectRestore);

static long init_record(mbboDirectRecord *prec)
{
    unsigned int i;
    epicsUInt8 *B=&prec->b0;
    epicsUInt16 mask=1;

    for(i=0; i<16; i++, mask<<=1)
        B[i] = !!(prec->val & mask);

    return 2; /* dont convert */

} /* end init_record() */

static long write_mbbo(mbboDirectRecord	*prec)
{
    long status;
    prec->rval=prec->val;
    prec->rval <<= prec->shft;

    status = dbPutLink(&prec->out,DBR_ULONG,&prec->rval,1);
    if(status) {
        recGblSetSevr(prec, WRITE_ALARM, INVALID_ALARM);
    }
    return(0);
}
