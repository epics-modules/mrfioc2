/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/** Arbitrary size lookup table mapping integer to string.
 *
 * record(stringin, "blah") {
 *   field(INP , "some:int")
 *   info(lut0, " 0 = zero")
 *   info(lut1, " 1 = one")
 *   info(lut2, " 3 = three")
 *   info(lutXX, "unknown")
 */

#include <stdexcept>
#include <map>
#include <string>
#include <sstream>

#include <stdio.h>
#include <errno.h>

#include <epicsTypes.h>
#include <dbAccess.h>
#include <dbStaticLib.h>
#include <dbScan.h>
#include <link.h>
#include <devSup.h>
#include <recGbl.h>
#include <devLib.h> // For S_dev_*
#include <alarm.h>
#include <errlog.h>

#include <stringinRecord.h>

#include "mrfCommon.h"
#include "devObj.h"

#include <epicsExport.h>

namespace {

std::string strip(const std::string& inp)
{
    size_t S = inp.find_first_not_of(" \t"),
           E = inp.find_last_not_of(" \t");
    if(S==inp.npos)
        return std::string(); // empty
    else
        return inp.substr(S, E-S+1);
}

struct DBENT {
    DBENTRY entry;
    template<typename REC>
    explicit DBENT(REC *prec)
    {
        dbInitEntry(pdbbase, &entry);
        if(dbFindRecord(&entry, prec->name))
            throw std::logic_error("Failed to lookup DBENTRY from dbCommon");
    }
    ~DBENT() {
        dbFinishEntry(&entry);
    }
};

struct LUTPriv {
    typedef std::map<epicsInt32, std::string> lut_t;
    lut_t lut;
    std::string unknown;
};

static
long init_record_lut(stringinRecord *prec)
{
    try {
        mrf::auto_ptr<LUTPriv> priv(new LUTPriv);

        priv->unknown = "<unknown>";

        DBENT entry(prec);

        for(long status = dbFirstInfo(&entry.entry); status==0; status = dbNextInfo(&entry.entry))
        {
            const char * const name = dbGetInfoName(&entry.entry);

            std::string line(dbGetInfoString(&entry.entry));

            if(strcmp(name, "lutXX")==0) {
                priv->unknown = strip(line);
                if(prec->tpro>1)
                    printf("%s : LUT <fallback> -> \"%s\"\n", prec->name, priv->unknown.c_str());
                continue;

            } else if(strncmp(name, "lut", 3)!=0) {
                continue;
            }

            size_t eq = line.find_first_of('=');
            if(eq==line.npos) {
                fprintf(stderr, "%s : info %s value missing '=' : %s\n", prec->name, name, line.c_str());
                return 0;
            }

            epicsUInt32 raw;
            {
                std::string num(line.substr(0, eq));
                if(epicsParseUInt32(num.c_str(), &raw, 0, 0)) {
                    fprintf(stderr, "%s : info %s index not number \"%s\"\n", prec->name, name, num.c_str());
                    throw std::runtime_error("Invalid LUT entry");
                }
            }

            std::pair<LUTPriv::lut_t::iterator, bool> ret =
                    priv->lut.insert(std::make_pair(raw, strip(line.substr(eq+1))));

            if(prec->tpro>1)
                printf("%s : LUT %u -> \"%s\"\n", prec->name, ret.first->first, ret.first->second.c_str());
        }

        /* initialize w/ unknown value */
        strncpy(prec->val, priv->unknown.c_str(), sizeof(prec->val));
        prec->val[sizeof(prec->val)-1] = '\0';

        prec->dpvt = priv.release();

        return 0;
    }CATCH(-ENODEV)
}

static
long read_lut(stringinRecord *prec)
{
    const LUTPriv * const priv = static_cast<LUTPriv*>(prec->dpvt);
    if(!priv) {
        (void)recGblSetSevr(prec, COMM_ALARM, INVALID_ALARM);
        return 0;
    }
    try {
        const std::string *val = &priv->unknown;

        epicsUInt32 raw;
        long status = dbGetLink(&prec->inp, DBR_LONG, &raw, 0, 0);

        if(status) {
            (void)recGblSetSevr(prec, LINK_ALARM, INVALID_ALARM);

        } else {
            LUTPriv::lut_t::const_iterator it = priv->lut.find(raw);
            if(it==priv->lut.end()) {
                (void)recGblSetSevr(prec, READ_ALARM, INVALID_ALARM);
            } else {
                val = &it->second;
            }
        }

        if(prec->tpro>2)
            printf("%s : LUT status=%ld select=%u VAL=\"%s\"\n",
                   prec->name, status, (unsigned)raw, val->c_str());

        strncpy(prec->val, val->c_str(), sizeof(prec->val));
        prec->val[sizeof(prec->val)-1] = '\0';

        return 0;
    }CATCH(-ENODEV)
}

common_dset devLUTSI = {
    5,
    0,
    0,
    (DEVSUPFUN)&init_record_lut,
    0,
    (DEVSUPFUN)&read_lut,
    0
};

} // namespace

extern "C" {
epicsExportAddress(dset, devLUTSI);
}
