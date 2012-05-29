/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <stringoutRecord.h>
#include <stringinRecord.h>

#include "devObj.h"

using namespace mrf;

/************** mbbi *************/

static long read_string(stringinRecord* prec)
{
if (!prec->dpvt) return -1;
try {
    addr<std::string> *priv=(addr<std::string>*)prec->dpvt;

    std::string s = priv->P->get();

    size_t len = std::min(NELEMENTS(prec->val)-1, s.size());

    memcpy(prec->val, s.c_str(), len);
    prec->val[len]=0;

    return 0;
} catch(std::exception& e) {
    epicsPrintf("%s: read error: %s\n", prec->name, e.what());
    return S_db_noMemory;
}
}

OBJECT_DSET(SIFromString,
            (&add_record_inp<stringinRecord,std::string>),
            &del_record_property,
            &init_record_empty,
            &read_string,
            NULL);


static long write_string(stringoutRecord* prec)
{
if (!prec->dpvt) return -1;
try {
    addr<std::string> *priv=(addr<std::string>*)prec->dpvt;

    priv->P->set(prec->val);

    return 0;
} catch(std::exception& e) {
    epicsPrintf("%s: write error: %s\n", prec->name, e.what());
    return S_db_noMemory;
}
}

OBJECT_DSET(SOFromString,
            (&add_record_out<stringoutRecord,std::string>),
            &del_record_property,
            &init_record_empty,
            &write_string,
            NULL);

#include <epicsExport.h>

OBJECT_DSET_EXPORT(SIFromString);
OBJECT_DSET_EXPORT(SOFromString);

