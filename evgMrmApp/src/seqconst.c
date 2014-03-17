/*************************************************************************\
* Copyright (c) 2012 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <dbDefs.h>
#include <errlog.h>
#include <recGbl.h>
#include <alarm.h>

#include <registryFunction.h>

#include <menuFtype.h>
#include <aSubRecord.h>


#define NINPUTS (aSubRecordU - aSubRecordA)

int seqConstDebug = 0;

static
menuFtype seq_repeat_ft[] = {
    menuFtypeULONG, menuFtypeULONG, menuFtypeULONG, menuFtypeDOUBLE, menuFtypeUCHAR
};

static
menuFtype seq_repeat_ftv[] = {
    menuFtypeDOUBLE, menuFtypeDOUBLE, menuFtypeUCHAR
};

/**@brief Periodically repeat a given set of event codes and time delays.
 * A bit mask selects if each possible repatition is actually populated.
 *
 *  Inputs
 *@param A Overall period.  In sequencer system ticks
 *@type  A ULONG
 *@param B # of cycles in the overall period.
 *@type  B ULONG
 *@param C Period bit mask.  Bit 0 is first repetition.
 *@type  C ULONG
 *@param D Cycle timestamp waveform
 *@type  D DOUBLE
 *@param E Cycle event code waveform
 *@type  E UCHAR
 *
 *  Outputs
 *@param VALA Cycle time.  (Overall period / # of cycles)
 *@type  VALA DOUBLE
 *@param VALB Period timestamp waveform
 *@type  VALB DOUBLE
 *@param VALC Period event code waveform
 *@type  VALB UCHAR
 */
static
long seq_repeat(aSubRecord *prec)
{
    unsigned int N,i;

    /* Inputs */
    epicsUInt32 period;
    epicsUInt32 num_cycles, per_mask, len_in, len_out, num_out;
    double add;
    double *cycle_D = prec->d;
    epicsUInt8 *cycle_C = prec->e;

    /* Outputs */
    double *period_D = prec->valb;
    epicsUInt8 *period_C = prec->valc;

    if(prec->nsev>=INVALID_ALARM) /* Invalid inputs */
        return -1;

    /* Check type and length of inputs and outputs */

    for(i=0; i<NELEMENTS(seq_repeat_ft); i++) {
        if((&prec->fta)[i]!=seq_repeat_ft[i]) {
            epicsPrintf("%s: Invalid type for FT%c\n", prec->name, 'A'+i);
            goto fail;
        }
        if((&prec->nea)[i]<=0) {
            epicsPrintf("%s.NE%c empty\n", prec->name, 'A'+i);
            goto fail;
        }
    }

    for(i=0; i<NELEMENTS(seq_repeat_ftv); i++) {
        if((&prec->ftva)[i]!=seq_repeat_ftv[i]) {
            epicsPrintf("%s: Invalid type for FTV%c\n", prec->name, 'A'+i);
            goto fail;
        }
        if((&prec->nova)[i]<=0) {
            epicsPrintf("%s.NOV%c empty\n", prec->name, 'A'+i);
            goto fail;
        }
    }

    period = *(epicsUInt32*)prec->a;
    num_cycles = *(epicsUInt32*)prec->b;
    per_mask = *(epicsUInt32*)prec->c;
    
    len_in = prec->ned;
    if(len_in > prec->nee)
        len_in = prec->nee;
    
    len_out = prec->novb;
    if(len_out > prec->novc)
        len_out = prec->novc;

    if(num_cycles==0) {
        goto fail;
    } else if(num_cycles>32) {
        epicsPrintf("%s: Num cycles is out of range", prec->name);
        goto fail;
    }

    /* amount to add per cycle */
    add = period/num_cycles;

    if(len_in * num_cycles > len_out) {
        epicsPrintf("%s: Not enough elements for result.  Have %u, need %u\n",
                    prec->name, len_out, len_in * num_cycles);
        goto fail;
    }

    if( period%num_cycles ) {
        epicsPrintf("%s: %u cycles does not evenly divide period %u", prec->name, num_cycles, period);
        goto fail;
    }

    *(double*)prec->vala = add;

    num_out = 0;
    for(N=0; per_mask && N<num_cycles; N++, per_mask>>=1) {
        if(!(per_mask&1))
            continue;
        for(i=0; i<len_in; i++) {
            if(cycle_D[i]>=add) {
                len_in = i; /* truncate */
                break;
            }
            *period_D++ = cycle_D[i] + add*N;
            *period_C++ = cycle_C[i];
        }
        num_out++;
    }
    
    if(num_out==0 || len_in==0) {
        /* 0 length arrays aren't handled so well, so have 1 length w/ 0 value */
        *period_D++ = 0.0;
        *period_C++ = 0;
        prec->nevb = prec->nevc = 1;
    } else {
        prec->nevb = prec->nevc = num_out * len_in;
    }

    return 0;

fail:
    recGblSetSevr(prec, CALC_ALARM, INVALID_ALARM);
    return -1;
}

/**@brief Merge several sorted sequences.
 *
 *  Inputs
 *@param A First time waveform
 *@type  A DOUBLE
 *@param B First code waveform
 *@type  B UCHAR
 *@param C Second time waveform
 *@type  C DOUBLE
 *@param D Second code waveform
 *@type  D UCHAR
 *...
 *
 *  Outputs
 *@param VALA Output time waveform
 *@type  VALA DOUBLE
 *@param VALB Output code waveform
 *@type  VALB UCHAR
 */
static
long seq_merge(aSubRecord *prec)
{
    unsigned int i;
    epicsUInt32 in_pos[NINPUTS/2]; // position in each pair of input times/codes
    unsigned int ninputs;

    epicsUInt32 maxout = prec->nova;
    double *out_T = prec->vala;
    epicsUInt8 *out_C = prec->valb;

    if(prec->nsev>=INVALID_ALARM) /* Invalid inputs */
        return -1;

    memset(in_pos, 0, sizeof(in_pos));

    if(maxout > prec->novb)
        maxout = prec->novb;

    /* check output types */
    if(prec->ftva!=menuFtypeDOUBLE || prec->ftvb!=menuFtypeUCHAR ||
       prec->nova==0 || prec->novb==0)
    {
        epicsPrintf("%s: Invalid types for lengths for VALA and/or VALB\n", prec->name);
        goto alarm;
    }

    {
        unsigned int N;
        for(N=0; N<NINPUTS/2; N++) {
            if((&prec->fta)[2*N]!=menuFtypeDOUBLE)
                break;
            if((&prec->fta)[2*N+1]!=menuFtypeUCHAR)
                break;

            /* Fail unless lengths match */
            if((&prec->nea)[2*N]!=(&prec->nea)[2*N+1]) {
                epicsPrintf("%s: NE%c (%d) != NE%c (%d)\n",
                            prec->name,
                            (char)('A'+2*N), (&prec->nea)[2*N],
                            (char)('A'+2*N+1), (&prec->nea)[2*N+1]);
                goto fail;
            }
        }

        ninputs = N;
    }

    if(ninputs==0) {
        epicsPrintf("%s: No inputs configured!\n", prec->name);
        goto fail;
    }

    if(seqConstDebug>1) {
        printf("%s Merge\n", prec->name);
    }

    for(i=0; i<maxout; i++) {
        unsigned int N;
        int found_element = -1;
        double last_time = -1.0; /* Initial value not used, but silences warning */

        for(N=0; N<ninputs; N++) {
            double *T;
            epicsUInt8 *C;
	    
            T = (double*)(&prec->a)[2*N];
            C = (epicsUInt8*)(&prec->a)[2*N+1];

            while(in_pos[N]<(&prec->nea)[2*N] && C[in_pos[N]]==0)
                in_pos[N]++; /* skip entries with code 0 */

            if(in_pos[N]==(&prec->nea)[2*N])
                continue; /* This input completely consumed */

            if(found_element==-1 || T[in_pos[N]]<last_time) {
                found_element = N;
                out_T[i] = T[in_pos[N]];
                out_C[i] = C[in_pos[N]];
                last_time = out_T[i];

            } else if(T[in_pos[N]]==last_time && found_element!=-1) {
                /* Duplicate timestamp! */
                printf("%s: Dup timestamp.  %c[%u] and %c[%u]\n",
                       prec->name,
                       'A'+(2*found_element), in_pos[found_element]-1,
                       'A'+(2*N), in_pos[N]);
                printf(" Found element: %u\n", found_element);
                printf(" i=%u, N=%u, ninputs=%u\n", i, N, ninputs);
                printf(" Out %u C=%u T=%f  [", i, out_C[i], out_T[i]);
                for(N=0; N<ninputs; N++)
                    printf("%u, ", in_pos[N/2]);
                printf("]\n");
                goto fail;
            }
        }

        if(found_element==-1) {
            /* All inputs consumed */
            goto done;
        } else if(i>0 && out_T[i] < out_T[i-1]) {
            epicsPrintf("%s: input %c times not sorted!\n", prec->name, 'A'+(2*found_element));
	    goto fail;
        } else {
            in_pos[found_element]++;
        }

        if(seqConstDebug>1) {
            printf("Out %u C=%u T=%f  [", i, out_C[i], out_T[i]);
            for(N=0; N<ninputs; N++)
                printf("%u, ", in_pos[N/2]);
            printf("\n");
        }
    }

    if(seqConstDebug>0) {
        epicsPrintf("%s: merge result has %u element\n", prec->name, i);
    }

    {
        unsigned int N;
        unsigned int nogood = 0;
        for(N=0; N<ninputs; N++) {
            if(in_pos[N]!=(&prec->nea)[2*N]) {
                epicsPrintf("%s.%c: Not completely consumed!  %u of %u\n",
			    prec->name, 'A'+(2*N), in_pos[N], (&prec->nea)[2*N]);
		nogood = 1;
            }
        }
        if(nogood)
          goto fail;
    }

done:
    if(i==0) {
        if(seqConstDebug>0)
            epicsPrintf("%s: merged yields empty sequence\n", prec->name);
        /* result is really empty */
        out_T[0] = 0.0;
        out_C[0] = 0;
        i = 1;
    }
    prec->neva = i;
    prec->nevb = i;

    return 0;

fail:
    /* when possible ensure a sane output (do nothing)
     * in case the alarm is ignored.
     */
    out_T[0] = 0.0;
    out_C[0] = 0;
    prec->neva = 1;
    prec->nevb = 1;
alarm:
    recGblSetSevr(prec, CALC_ALARM, INVALID_ALARM);
    return -1;
}

/**@brief Sequence shifter
 *
 *  Inputs
 *@param A Delay
 *@type  A DOUBLE
 *@param B time waveform
 *@type  B DOUBLE
 *...
 *
 *  Outputs
 *@param VALA Output time waveform
 *@type  VALA DOUBLE
 */
static
long seq_shift(aSubRecord *prec)
{
    double delay;
    const double *input=(const double*)prec->b;
    double *output=(double*)prec->vala;
    epicsUInt32 i;

    if(prec->fta!=menuFtypeDOUBLE
       || prec->ftb!=menuFtypeDOUBLE
       || prec->ftva!=menuFtypeDOUBLE)
    {
        epicsPrintf("%s: invalid types\n", prec->name);
        goto fail;
    } else if(prec->nea==0 || prec->neb > prec->nova) {
        epicsPrintf("%s: incorrect sizes\n", prec->name);
        goto fail;
    }

    delay = *(double*)prec->a;

    for(i=0; i<prec->neb; i++)
        output[i] = input[i]+delay;

    prec->neva = prec->neb;

    return 0;

fail:
    recGblSetSevr(prec, CALC_ALARM, INVALID_ALARM);
    return -1;
}

static registryFunctionRef asub_seq[] = {
    {"Seq Repeat", (REGISTRYFUNCTION) seq_repeat},
    {"Seq Merge", (REGISTRYFUNCTION) seq_merge},
    {"Seq Shift", (REGISTRYFUNCTION) seq_shift},
};

static
void asub_evg(void) {
    registryFunctionRefAdd(asub_seq, NELEMENTS(asub_seq));
}

#include <epicsExport.h>

epicsExportRegistrar(asub_evg);
epicsExportAddress(int, seqConstDebug);
