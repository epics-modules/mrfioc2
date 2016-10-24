/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <math.h>
#include <string.h>

#include <dbDefs.h>
#include <errlog.h>
#include <recGbl.h>
#include <alarm.h>
#include <dbAccess.h>

#include <registryFunction.h>

#include <menuFtype.h>
#include <aSubRecord.h>
#include <epicsExport.h>

#define NINPUTS (aSubRecordU - aSubRecordA)

static
long select_string(aSubRecord *prec)
{
    unsigned i;
    char *out = prec->vala;
    DBLINK *L = &prec->inpa;
    const char **I = (const char**)&prec->a;

    /* find the first input w/o an active alarm */
    for(i=0; i<NINPUTS; i++) {
        epicsEnum16 sevr, stat;
        if(L[i].type==CONSTANT)
            continue;
        if(dbGetAlarm(&L[i], &sevr, &stat))
            continue;
        if(sevr!=NO_ALARM)
            continue;
        memcpy(out, I[i], MAX_STRING_SIZE);
        return 0;
    }

    out[0] = '\0';
    (void)recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
    return 0;
}

/**@brief Generate a time series
 *
 *@param A The first value
 *@type A DOUBLE
 *@param B Step size
 *@type B DOUBLE
 *@param C Number of samples
 *@type C LONG
 *
 *@param OUTA The output sequence
 *@type OUTA DOUBLE
 */
static
long gen_timeline(aSubRecord *prec)
{
    double x0, dx;
    epicsUInt32 count, i;
    double *result;

    if (prec->fta != menuFtypeDOUBLE
        || prec->ftb != menuFtypeDOUBLE
        || prec->ftc != menuFtypeLONG
        ) {
        errlogPrintf("%s incorrect input type. A,B (DOUBLE), C (LONG)",
                     prec->name);
        return -1;
    }

    if (prec->ftva != menuFtypeDOUBLE
        ) {
        errlogPrintf("%s incorrect output type. OUTA (DOUBLE)",
                     prec->name);
        return -1;
    }

    if (prec->nova < 1)
    {
        errlogPrintf("%s output size too small. OUTA (>=1)",
                     prec->name);
        return -1;
    }

    x0=*(double*)prec->a;
    dx=*(double*)prec->b;
    count=*(epicsUInt32*)prec->c;
    result=prec->vala;

    if (count>prec->nova)
        count=prec->nova;

    result[0]=x0;
    for (i=1; i<count; i++) {
        result[i]=result[i-1]+dx;
    }

    prec->neva = count;

    return 0;
}

/**@brief Output a delayed pulse.
 * The result is a bool array
 *
 *@param A Delay (in egu)
 *@type A DOUBLE
 *@param B Width (in egu)
 *@type B DOUBLE
 *@param C EGU per tick (sample period)
 *@type C DOUBLE
 *@param D Ensure output length is a multiple of this number
 *@type D LONG
 *
 *@param OUTA The output sequence
 *@type OUTA UCHAR
 */
static
long gen_delaygen(aSubRecord *prec)
{
    double delay, width, egupertick;
    epicsUInt32 count, i, mult;
    unsigned char *result;
    epicsUInt32 idelay, iwidth;

    if (prec->fta != menuFtypeDOUBLE
        || prec->ftb != menuFtypeDOUBLE
        || prec->ftc != menuFtypeDOUBLE
        || prec->ftd != menuFtypeLONG
        ) {
        errlogPrintf("%s incorrect input type. A,B,C (DOUBLE) D (LONG)",
                     prec->name);
        return -1;
    }

    if (prec->ftva != menuFtypeUCHAR
        ) {
        errlogPrintf("%s incorrect output type. OUTA (DOUBLE)",
                     prec->name);
        return -1;
    }

    delay=*(double*)prec->a;
    width=*(double*)prec->b;
    egupertick=*(double*)prec->c;
    mult=*(epicsUInt32*)prec->d;
    result=prec->vala;
    count=prec->nova;

    if(mult==0)
        mult=1;

    idelay=(epicsInt32)(0.5 + delay/egupertick);
    iwidth=(epicsInt32)(0.5 + width/egupertick);

    if(idelay>=count) {
        errlogPrintf("%s : invalid delay %d check units\n",prec->name,idelay);
        return -1;
    } else if(iwidth>=count) {
        errlogPrintf("%s : invalid delay %d check units\n",prec->name,iwidth);
        return -1;
    } else if(idelay+iwidth>=count) {
        errlogPrintf("%s : delay+width is too long\n",prec->name);
        return -1;
    }

    for (i=0; i<count; i++) {
        if(i<idelay) {
            result[i]=0;
        } else if(i<idelay+iwidth) {
            result[i]=1;
        } else {
            /* ensure last element is 0 */
            result[i]=0;
            if (i%mult==mult-1) {
                i++;
                break;
            }
        }
    }

    prec->neva = i;

    return 0;
}

/**@brief Bool array constructor
 * Build a >16-bit boolean array from component integers.
 *
 * Bits are taken for each component in MSB order and placed
 * into the output array in MSB order.
 *
 *@param A First (Most significant) 16 bits
 *@type A SHORT
 *@param B Next 16 bits
 *@type B SHORT
 *@param C Next 16 bits
 *@type C SHORT
 *
 *@param OUTA The output sequence
 *@type OUTA UCHAR
 */
static
long gen_bitarraygen(aSubRecord *prec)
{
    epicsEnum16 *intype=&prec->fta;
    epicsUInt16 **indata=(epicsUInt16 **)&prec->a;
    epicsUInt32 *inlen=&prec->noa;

    epicsUInt32 outlen=prec->nova, curlen;

    epicsUInt8 *result=prec->vala;
    epicsUInt32 numinputs = outlen/16;

    if(prec->ftva!=menuFtypeUCHAR) {
        errlogPrintf("%s incorrect output type. A (UCHAR))\n",
                     prec->name);
        return -1;
    }
    if(outlen%16)
        numinputs++;
    if(numinputs>NINPUTS)
        numinputs=NINPUTS;

    for(curlen=0; curlen<numinputs; curlen++) {
        if(intype[curlen]!=menuFtypeUSHORT) {
            errlogPrintf("%s incorrect input type. %c (UCHAR))\n",
                         prec->name, 'A'+curlen);
            return -1;
        } else if(inlen[curlen]!=1){
            errlogPrintf("%s incorrect input length. %c (1))\n",
                         prec->name, 'A'+curlen);
            return -1;
        }
    }

    for(curlen=0; curlen<outlen; curlen++) {
        epicsUInt32 I=curlen/16;
        epicsUInt32 B=15-(curlen%16);
        if(I>NINPUTS)
            break;

        result[curlen]=!!(*indata[I]&(1<<B));
    }

    return 0;
}

static
long gun_bunchTrain(aSubRecord *prec)
{
	int bunchPerTrain;
	unsigned char *result;
	epicsUInt32 count;
	int i, j;


    if (prec->fta != menuFtypeULONG) {
        errlogPrintf("%s incorrect input type. A(ULONG)",
                     prec->name);
        return -1;
    }

    if (prec->ftva != menuFtypeUCHAR) {
        errlogPrintf("%s incorrect output type. OUTA (DOUBLE)",
                     prec->name);
        return -1;
    }

    bunchPerTrain = *(int*)prec->a;
    result = prec->vala;

    if(bunchPerTrain<1 || bunchPerTrain>150) {
        errlogPrintf("%s : invalid number of bunches per train %d.\n",prec->name,bunchPerTrain);
        return -1;
    }

    count = 0;

    for(i = 0; i < bunchPerTrain; i++) {
        for(j = 0; j < 10; j++) {
            count++;
            if(j < 5)
                result[i*10 + j] = 1;
            else
                result[i*10 + j] = 0;
        }
    }
    for(i = 0; i <10; i++, count++) {
        result[count] = 0;
    }

    prec->neva = count;
    return 0;
}

static registryFunctionRef asub_seq[] = {
    {"Select String", (REGISTRYFUNCTION) select_string},
    {"Timeline", (REGISTRYFUNCTION) gen_timeline},
    {"Bit Array Gen", (REGISTRYFUNCTION) gen_bitarraygen},
    {"Delay Gen", (REGISTRYFUNCTION) gen_delaygen},
    {"Bunch Train", (REGISTRYFUNCTION) gun_bunchTrain}
};

static
void asub_evr(void) {
    registryFunctionRefAdd(asub_seq, NELEMENTS(asub_seq));
}

#include <epicsExport.h>

epicsExportRegistrar(asub_evr);

