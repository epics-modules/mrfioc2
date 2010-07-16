
#include <math.h>

#include <dbDefs.h>
#include <errlog.h>

#include <registryFunction.h>

#include <menuFtype.h>
#include <aSubRecord.h>

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
 *
 *@param OUTA The output sequence
 *@type OUTA UCHAR
 */
static
long gen_delaygen(aSubRecord *prec)
{
	double delay, width, egupertick;
	epicsUInt32 count, i;
	unsigned char *result;
	epicsUInt32 idelay, iwidth;

	if (prec->fta != menuFtypeDOUBLE
		|| prec->ftb != menuFtypeDOUBLE
		|| prec->ftc != menuFtypeDOUBLE
		) {
		errlogPrintf("%s incorrect input type. A,B,C (DOUBLE)",
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
	result=prec->vala;
	count=prec->nova;

	idelay=delay/egupertick;
	iwidth=width/egupertick;

	if(idelay<0 || idelay>=count) {
		errlogPrintf("%s : invalid delay %d check units\n",prec->name,idelay);
		return -1;
	} else if(iwidth<0 || iwidth>=count) {
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
			if (i%20==19) {
				i++;
				break;
			}
		}
	}

	prec->neva = i;

	return 0;
}

static registryFunctionRef asub_seq[] = {
	{"Timeline", (REGISTRYFUNCTION) gen_timeline},
	{"Delay Gen", (REGISTRYFUNCTION) gen_delaygen}
};

static
void asub_evr(void) {
	registryFunctionRefAdd(asub_seq, NELEMENTS(asub_seq));
}

#include <epicsExport.h>

epicsExportRegistrar(asub_evr);

