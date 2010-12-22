/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <epicsTypes.h>
#include <epicsTime.h>

#include "evrmap.h"
#include "evr/evr.h"

#include <stdexcept>
#include <errlog.h>
#include <epicsTime.h>
#include <epicsVersion.h>

#define epicsExportSharedSymbols
#include "evrGTIF.h"

struct priv {
    int ok;
    epicsTimeStamp *ts;
    int event;
    priv(epicsTimeStamp *t, int e) : ok(epicsTimeERROR), ts(t), event(e) {}
};

static
bool visitTime(priv* p,short,EVR& evr)
{
    bool tsok=evr.getTimeStamp(p->ts, p->event);
    if (tsok) {
        p->ok=epicsTimeOK;
        return true;
    } else
        return false;
}

extern "C"
epicsShareFunc
int EVREventTime(epicsTimeStamp *pDest, int event)
{
try {
    priv p(pDest, event);
    evrmap.visit(&p, &visitTime);
    return p.ok;
} catch (std::exception& e) {
    epicsPrintf("EVREventTime failed: %s\n", e.what());
    return epicsTimeERROR;
}
}

extern "C"
epicsShareFunc
int EVRCurrentTime(epicsTimeStamp *pDest)
{
    return EVREventTime(pDest, epicsTimeEventCurrentTime);
}

#if EPICS_VERSION==3 && EPICS_REVISION==14 && EPICS_MODIFICATION>=9

#include <generalTimeSup.h>

extern "C"
void EVRTime_Registrar()
{
    int ret=0;
    ret|=generalTimeCurrentTpRegister("EVR", ER_PROVIDER_PRIORITY, &EVRCurrentTime);
    ret|=generalTimeEventTpRegister  ("EVR", ER_PROVIDER_PRIORITY, &EVREventTime);
    if (ret)
        epicsPrintf("Failed to register EVR time provider\n");
}

#else
extern "C"
void EVRTime_Registrar() {}

#endif

#include <epicsExport.h>

epicsExportRegistrar(EVRTime_Registrar);
