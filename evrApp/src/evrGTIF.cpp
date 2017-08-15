/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/* EVR GeneralTime InterFace
 *
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#include <epicsTypes.h>
#include <epicsTime.h>

#include <stdexcept>
#include <errlog.h>
#include <epicsMutex.h>
#include <epicsGuard.h>
#include <epicsTime.h>
#include <epicsVersion.h>
#include <initHooks.h>

#include <epicsExport.h>

#include "mrf/object.h"
#include "evr/evr.h"
#include "evrGTIF.h"

#ifndef M_time
#define S_time_unsynchronized epicsTimeERROR
#endif

struct priv {
    int ok;
    epicsTimeStamp *ts;
    int event;
    priv(epicsTimeStamp *t, int e) : ok(S_time_unsynchronized), ts(t), event(e) {}
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
    return S_time_unsynchronized;
}
}

extern "C"
int EVRCurrentTime(epicsTimeStamp *pDest)
{
    return EVREventTime(pDest, epicsTimeEventCurrentTime);
}

int mrmGTIFEnable = 1;

#if EPICS_VERSION_INT >= VERSION_INT(3,14,9,0)

#include <generalTimeSup.h>

static
void EVRTime_Hooks(initHookState state)
{
    if(state!=initHookAtBeginning)
        return;

    int ret=0;
    ret|=EVRInitTime();
    // conditionally register the current (aka wall clock) time provider.
    // May be disabled to trade off accuracy and precision for execution speed
    // since EPICS calls this a *lot*.
    if(mrmGTIFEnable)
        ret|=generalTimeCurrentTpRegister("EVR", ER_PROVIDER_PRIORITY, &EVRCurrentTime);
    else
        epicsPrintf("EVR Current time provider NOT register\n");
    // always register the event time provider (no fallback, and not called by default)
    ret|=generalTimeEventTpRegister  ("EVR", ER_PROVIDER_PRIORITY, &EVREventTime);
    if (ret)
        epicsPrintf("Failed to register EVR time provider\n");
}

extern "C"
void EVRTime_Registrar()
{
    initHookRegister(&EVRTime_Hooks);
}

#else
extern "C"
void EVRTime_Registrar() {}

#endif

#include <epicsExport.h>
extern "C"{
 epicsExportRegistrar(EVRTime_Registrar);
 epicsExportAddress(int, mrmGTIFEnable);
}
