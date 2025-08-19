/*************************************************************************\
* Copyright (c) 2013 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
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

/** @brief NSLS2 sequence selector
 *
 * Process operator timing mode selection into
 * a set of sequence rep. masks and associated status
 *
 * Inputs
 @param A Linac mode selection (enum)
 @type  A ULONG
 @param B Booster mode selection (enum)
 @type  B ULONG
 @param C Linac permitted mode mask (bitmask)
 @type  C ULONG
 @param D Booster permitted mode mask (bitmask)
 @type  D ULONG
 *
 * Outputs
 @param VALA Booster injection possible (bool)
 @type  VALA ULONG
 @param VALB Injection rep. diagnostic bit mask (bitmask)
 @type  VALB ULONG
 @param VALC Linac sequence rep. mask (bitmask)
 @type  VALC ULONG
 @param VALD Booster 1Hz sequence rep. mask (bitmask)
 @type  VALD ULONG
 @param VALE Booster 2Hz sequence rep. mask (bitmask)
 @type  VALE ULONG
 @param VALF Booster Stacking sequence rep. mask (bitmask)
 @type  VALF ULONG
 */
long seq_select(aSubRecord *prec)
{
    int fail = 0;
    epicsUInt32 LNMode, BRMode, LNMask, BRMask;
    epicsUInt32 *BRAllow = prec->vala,
                *InjMask = prec->valb,
                *LNRepMask = prec->valc,
                *BR1HzRepMask = prec->vald,
                *BR2HzRepMask = prec->vale,
                *BRStkRepMask = prec->valf;

    if(prec->fta!=menuFtypeULONG ||
        prec->ftb!=menuFtypeULONG ||
        prec->ftc!=menuFtypeULONG ||
        prec->ftd!=menuFtypeULONG ||
        prec->ftva!=menuFtypeULONG ||
        prec->ftvb!=menuFtypeULONG ||
        prec->ftvc!=menuFtypeULONG ||
        prec->ftvd!=menuFtypeULONG ||
        prec->ftve!=menuFtypeULONG ||
        prec->ftvf!=menuFtypeULONG)
    {
        errlogPrintf("%s: Invalid field types!\n", prec->name);
        (void)recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
        return 0;
    }

    LNMode = *(epicsUInt32*)prec->a;
    BRMode = *(epicsUInt32*)prec->b;
    LNMask = *(epicsUInt32*)prec->c;
    BRMask = *(epicsUInt32*)prec->d;

    /* Allow only permitted slots to be filled */
    LNMode &= LNMask;
    BRMode &= BRMask;

    if(LNMode&~0x3ff) {
        errlogPrintf("%s: Invalid LN Mode Mask\n", prec->name);
        fail = 1;
    }
    if(BRMode&~0xf) {
        errlogPrintf("%s: Invalid BR Mode Mask\n", prec->name);
        fail = 1;
    }

    if(fail) {
        (void)recGblSetSevr(prec, UDF_ALARM, INVALID_ALARM);
        LNMode = BRMode = 0;
    }

    *InjMask = (LNMode<<4)|BRMode;

    switch(*InjMask) {
    case 0x011: /* 1Hz in both */
    case 0x032: /* Stacking in both */
    case 0x21C: /* 2 Hz in both */
        *BRAllow = 1;
        break;
    default: /* All others prohibit injection into BR */
        *BRAllow = 0;
    }

    *LNRepMask = LNMode;
    *BR1HzRepMask = BRMode & 1;
    *BR2HzRepMask = (BRMode>>2) & 3;
    *BRStkRepMask = (BRMode>>1) & 1;

    return 0;
}

static registryFunctionRef asub_seq[] = {
    {"NSLS2SeqMask", (REGISTRYFUNCTION) seq_select},
};

static
void asub_nsls2_evg(void) {
    registryFunctionRefAdd(asub_seq, NELEMENTS(asub_seq));
}

#include <epicsExport.h>

epicsExportRegistrar(asub_nsls2_evg);
