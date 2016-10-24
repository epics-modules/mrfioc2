/*************************************************************************\
* Copyright (c) 2011 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <cstdio>

#include <waveformRecord.h>
#include <menuFtype.h>

#include "devObj.h"

using namespace mrf;

static inline
long add_record_waveform(dbCommon *pcom)
{
    waveformRecord *prec=(waveformRecord*)pcom;
    switch(prec->ftvl) {
    case menuFtypeCHAR:
        return add_record_property<epicsInt8[1]>(pcom, &prec->inp);
    case menuFtypeUCHAR:
        return add_record_property<epicsUInt8[1]>(pcom, &prec->inp);
    case menuFtypeSHORT:
        return add_record_property<epicsInt16[1]>(pcom, &prec->inp);
    case menuFtypeUSHORT:
        return add_record_property<epicsUInt16[1]>(pcom, &prec->inp);
    case menuFtypeLONG:
        return add_record_property<epicsInt32[1]>(pcom, &prec->inp);
    case menuFtypeULONG:
        return add_record_property<epicsUInt32[1]>(pcom, &prec->inp);
    case menuFtypeFLOAT:
        return add_record_property<float[1]>(pcom, &prec->inp);
    case menuFtypeDOUBLE:
        return add_record_property<double[1]>(pcom, &prec->inp);
    case menuFtypeSTRING:
    default:
        printf("%s: Ftype not supported\n", prec->name);
        return S_db_errArg;
    }
}

template<typename T>
static void
readop(waveformRecord* prec)
{
    addr<T[1]> *priv=(addr<T[1]>*)prec->dpvt;
    scopedLock<mrf::Object> g(*priv->O);
    prec->nord = priv->P->get((T*)prec->bptr, prec->nelm);
}

static long read_waveform(waveformRecord* prec)
{
if (!prec->dpvt) return -1;
try {
    switch(prec->ftvl) {
    case menuFtypeCHAR:
        readop<epicsInt8>(prec); break;
    case menuFtypeUCHAR:
        readop<epicsUInt8>(prec); break;
    case menuFtypeSHORT:
        readop<epicsInt16>(prec); break;
    case menuFtypeUSHORT:
        readop<epicsUInt16>(prec); break;
    case menuFtypeLONG:
        readop<epicsInt32>(prec); break;
    case menuFtypeULONG:
        readop<epicsUInt32>(prec); break;
    case menuFtypeFLOAT:
        readop<float>(prec); break;
    case menuFtypeDOUBLE:
        readop<double>(prec); break;
    case menuFtypeSTRING:
    default:
        printf("%s: Ftype not supported\n", prec->name);
        return S_db_errArg;
    }

    return 0;
} catch(std::exception& e) {
    epicsPrintf("%s: read error: %s\n", prec->name, e.what());
    return S_db_noMemory;
}
}

OBJECT_DSET(WFIn,
            &add_record_waveform,
            &del_record_property,
            &init_record_empty,
            &read_waveform,
            NULL);

template<typename T>
static void
writeop(waveformRecord* prec)
{
    addr<T[1]> *priv=(addr<T[1]>*)prec->dpvt;
    scopedLock<mrf::Object> g(*priv->O);
    priv->P->set((const T*)prec->bptr, prec->nord);
}

static long write_waveform(waveformRecord* prec)
{
if (!prec->dpvt) return -1;
try {

    switch(prec->ftvl) {
    case menuFtypeCHAR:
        writeop<epicsInt8>(prec); break;
    case menuFtypeUCHAR:
        writeop<epicsUInt8>(prec); break;
    case menuFtypeSHORT:
        writeop<epicsInt16>(prec); break;
    case menuFtypeUSHORT:
        writeop<epicsUInt16>(prec); break;
    case menuFtypeLONG:
        writeop<epicsInt32>(prec); break;
    case menuFtypeULONG:
        writeop<epicsUInt32>(prec); break;
    case menuFtypeFLOAT:
        writeop<float>(prec); break;
    case menuFtypeDOUBLE:
        writeop<double>(prec); break;
    case menuFtypeSTRING:
    default:
        printf("%s: Ftype not supported\n", prec->name);
        return S_db_errArg;
    }

    return 0;
} catch(std::exception& e) {
    epicsPrintf("%s: read error: %s\n", prec->name, e.what());
    return S_db_noMemory;
}
}

OBJECT_DSET(WFOut,
            &add_record_waveform,
            &del_record_property,
            &init_record_empty,
            &write_waveform,
            NULL);

#include <epicsExport.h>
extern "C" {
 OBJECT_DSET_EXPORT(WFIn);
 OBJECT_DSET_EXPORT(WFOut);
}
