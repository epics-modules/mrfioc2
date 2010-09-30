/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2008 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 *      Original Authors: Bob Dalesio and Marty Kraimer
 *
 * Modified from devWfSoft.c
 *
 * The mailbox waveform will read or write from the linked record.
 * It is intended as an online (ie fast) save/restore of waveforms.
 * Needed because the waveformRecord does not have a DOL field.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <alarm.h>
#include <dbEvent.h>
#include <dbDefs.h>
#include <dbAccess.h>
#include <recGbl.h>
#include <devSup.h>
#include <waveformRecord.h>

static long init_record(waveformRecord *prec)
{
    /* INP must be CONSTANT, PV_LINK, DB_LINK or CA_LINK*/
    switch (prec->inp.type) {
    case CONSTANT:
        prec->nord = 0;
        break;
    case PV_LINK:
    case DB_LINK:
    case CA_LINK:
        break;
    default:
        recGblRecordError(S_db_badField, (void *)prec,
            "devWfMailbox (init_record) Illegal INP field");
        return(S_db_badField);
    }
    return 0;
}

static long read_wf(waveformRecord *prec)
{
    long nRequest = prec->nelm;

    if (!prec->rarm) {
        /* If not "armed" then put */
        prec->rarm=0;
        db_post_events(prec, &prec->rarm, DBE_VALUE|DBE_ARCHIVE);
        dbPutLink(&prec->inp, prec->ftvl, prec->bptr, prec->nord);
    } else {
        /* If "armed" then get */
        if (prec->inp.type != CONSTANT)
            dbGetLinkValue(&prec->inp, prec->ftvl, prec->bptr, 0, &nRequest);
        if (nRequest > 0) {
            prec->nord = nRequest;
            if (prec->tsel.type == CONSTANT &&
                prec->tse == epicsTimeEventDeviceTime)
                dbGetTimeStamp(&prec->inp, &prec->time);
        }
    }

    return 0;
}

#include <epicsExport.h>

struct {
    long      number;
    DEVSUPFUN report;
    DEVSUPFUN init;
    DEVSUPFUN init_record;
    DEVSUPFUN get_ioint_info;
    DEVSUPFUN read_wf;
} devWfMailbox = {
    5,
    NULL,
    NULL,
    init_record,
    NULL,
    read_wf
};
epicsExportAddress(dset, devWfMailbox);
