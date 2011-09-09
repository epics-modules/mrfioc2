#include <math.h>
#include <stdio.h>

#include <dbDefs.h>
#include <errlog.h>

#include <registryFunction.h>
#include <epicsExport.h>

#include <menuFtype.h>
#include <aSubRecord.h>

long bunchTrigger(aSubRecord *prec)
{
    int event = 16;
    int time  = 100;

    if (prec->fta != menuFtypeLONG) {
        errlogPrintf("%s incorrect input type. A(LONG)",
                     prec->name);
        return -1;
    }

    if ( (prec->ftva != menuFtypeLONG) |
         (prec->ftvb != menuFtypeLONG) |
         (prec->ftvc != menuFtypeLONG) |
         (prec->ftvf != menuFtypeLONG) ) {
        errlogPrintf("%s incorrect output type. OUTA OUTB OUTC OUTF(LONG)",
                     prec->name);
        return -1;
    }

    if (prec->ftvd != menuFtypeUCHAR) {
        errlogPrintf("%s incorrect output type. OUTD (UCHAR)",
                     prec->name);
        return -1;
    }

    if (prec->ftve != menuFtypeDOUBLE) {
        errlogPrintf("%s incorrect output type. OUTE (DOUBLE)",
                     prec->name);
        return -1;
    }

    int bunchTrainFreq = *(int*)prec->a;
    unsigned char *eventCode = prec->vald;
    double *timestamp = prec->vale;

    if(bunchTrainFreq<1 || bunchTrainFreq>10) {
        errlogPrintf("%s : invalid number of bunch train frequency %d.\n",prec->name,bunchTrainFreq);
        return -1;
    }

    epicsUInt32 i = 0;
    eventCode[0] = event;
    timestamp[0] = 0;
    for(i = 1; i <= bunchTrainFreq; i++) {
        eventCode[i] = event;
        timestamp[i] = timestamp[i-1] + time;
    }

    prec->nevd = i;
    prec->neve = i;

    *(int*)(prec->vala) = 0;  //EGU
    *(int*)(prec->valb) = 1;  //mSec
    *(int*)(prec->valc) = 0;  //Normal
    *(int*)(prec->valf) = 1;  //Commit

    return 0;
}

static registryFunctionRef asub_seq[] = {
    {"BunchTrigger", (REGISTRYFUNCTION) bunchTrigger}
};

static
void asub_evg(void) {
    registryFunctionRefAdd(asub_seq, NELEMENTS(asub_seq));
}

epicsExportRegistrar(asub_evg);
