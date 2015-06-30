/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/* EVR GeneralTime InterFace
 *
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <epicsTypes.h>
#include <epicsTime.h>

#include <stdexcept>
#include <errlog.h>
#include <epicsMutex.h>
#include <epicsGuard.h>
#include <epicsTime.h>
#include <epicsVersion.h>

#include <epicsExport.h>

#include "mrf/object.h"
#include "evr/evr.h"
#include "evrGTIF.h"

struct priv {
    int ok;
    epicsTimeStamp *ts;
    int event;
    priv(epicsTimeStamp *t, int e) : ok(epicsTimeERROR), ts(t), event(e) {}
};

static EVR* lastSrc = 0;

static epicsMutexId lastLock;

epicsShareFunc
int EVRInitTime()
{
    if(lastLock)
        return 0;

    lastLock = epicsMutexMustCreate();
    return 0;
}

static
bool visitTime(mrf::Object* obj, void* raw)
{
    EVR *evr = dynamic_cast<EVR*>(obj);
    if(!evr)
        return true;

    priv *p = (priv*)raw;
    bool tsok=evr->getTimeStamp(p->ts, p->event);
    if (tsok) {
        lastSrc = evr;
        p->ok=epicsTimeOK;
        return false;
    } else
        return true;
}

extern "C"
epicsShareFunc
int EVREventTime(epicsTimeStamp *pDest, int event)
{
try {
    epicsMutexMustLock(lastLock);

    if(lastSrc) {
        if(lastSrc->getTimeStamp(pDest, event)) {
            epicsMutexUnlock(lastLock);
            return epicsTimeOK;
        }
    }
    priv p(pDest, event);
    mrf::Object::visitObjects(&visitTime, (void*)&p);
    epicsMutexUnlock(lastLock);
    return p.ok;
} catch (std::exception& e) {
    epicsMutexUnlock(lastLock);
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

#if EPICS_VERSION==3 && ( EPICS_REVISION>14 || (EPICS_REVISION==14 && EPICS_MODIFICATION>=9) )

#include <generalTimeSup.h>

extern "C"
void EVRTime_Registrar()
{
    // int ret=0;
    // ret|=EVRInitTime();
    // ret|=generalTimeCurrentTpRegister("EVR", ER_PROVIDER_PRIORITY, &EVRCurrentTime);
    // ret|=generalTimeEventTpRegister  ("EVR", ER_PROVIDER_PRIORITY, &EVREventTime);
    // if (ret)
    //     epicsPrintf("Failed to register EVR time provider\n");
}

#else
extern "C"
void EVRTime_Registrar() {}

#endif

#include <epicsExport.h>
extern "C"{
 epicsExportRegistrar(EVRTime_Registrar);
}
