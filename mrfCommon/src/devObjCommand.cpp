/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <boRecord.h>

#include "devObj.h"

using namespace mrf;

static
long exec_bo(boRecord *prec)
{
    try {
        addr<void> *priv=(addr<void>*)prec->dpvt;
        {
            scopedLock<mrf::Object> g(*priv->O);
            priv->P->exec();
        }
        return 0;
    } catch(std::exception& e) {
        (void)recGblSetSevr(prec, WRITE_ALARM, INVALID_ALARM);
        epicsPrintf("%s: read error: %s\n", prec->name, e.what());
        return S_db_noMemory;
    }
}

OBJECT_DSET(BOCommand,
            (&add_record_out<boRecord,void>),
            &del_record_property,
            &init_record_empty,
            &exec_bo,
            NULL);

#include <epicsExport.h>
extern "C" {
 OBJECT_DSET_EXPORT(BOCommand);
}
