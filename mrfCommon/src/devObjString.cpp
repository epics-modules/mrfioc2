/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */
#ifdef _WIN32
 #define NOMINMAX
 #include <algorithm>
#endif


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

    std::string s;
    {
        scopedLock<mrf::Object> g(*priv->O);
        s = priv->P->get();
    }

    size_t len = std::min(NELEMENTS(prec->val)-1, s.size());

    memcpy(prec->val, s.c_str(), len);
    prec->val[len]=0;

    return 0;
} catch(std::exception& e) {
    (void)recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
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

    {
        scopedLock<mrf::Object> g(*priv->O);
        priv->P->set(prec->val);
    }

    return 0;
} catch(std::exception& e) {
    (void)recGblSetSevr(prec, WRITE_ALARM, INVALID_ALARM);
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
extern "C" {
 OBJECT_DSET_EXPORT(SIFromString);
 OBJECT_DSET_EXPORT(SOFromString);
}
