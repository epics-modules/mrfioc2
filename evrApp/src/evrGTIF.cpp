
#include <epicsTypes.h>
#include <epicsTime.h>

#include "cardmap.h"

#include <stdexcept>
#include <errlog.h>

#define epicsExportSharedSymbols
#include "evrGTIF.h"

extern "C"
epicsStatus ErEventInterest(int card, epicsInt32 event, int add)
{
try {
    EVR *evr=getEVR<EVR>(card);
    if (!evr) return -1;

    if (!evr->interestedInEvent(event,add==1))
        return -1;

    return 0;
} catch(std::exception& e) {
    errlogPrintf("Error: ErEventInterest(card=%d,evt=%d,op=%d) => %s\n",
      card, event, add, e.what());
    return -2;
}
}

extern "C"
epicsStatus ErGetTimeStamp (epicsInt32 card, epicsInt32 event, epicsTimeStamp* ts)
{
try {
    EVR *evr=getEVR<EVR>(card);
    if (!evr || !ts) return -1;

    if (!evr->getTimeStamp(ts, event))
        return -1;

    return 0;
} catch(std::exception& e) {
    errlogPrintf("Error: ErGetTimeStamp(card=%d,evt=%d,ts=*) => %s\n",
      card, event, e.what());
    return -2;
}
}

extern "C"
epicsStatus ErGetTicks (int card, epicsUInt32* ticks)
{
try {
    EVR *evr=getEVR<EVR>(card);
    if (!evr || !ticks) return -1;

    if (!evr->getTicks(ticks))
        return -1;

    return 0;
} catch(std::exception& e) {
    errlogPrintf("Error: ErGetTicks(card=%d,ticks=*) => %s\n",
      card, e.what());
    return -2;
}
}
