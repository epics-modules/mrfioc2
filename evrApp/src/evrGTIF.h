/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef EVRGTIF_H_INC
#define EVRGTIF_H_INC

#include <epicsTypes.h>
#include <epicsTime.h>
#include <shareLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*General Time Interface
 *
 * C interface to the timestamp supply functions
 * of the EVR interface for use with generalTime.
 */

/** @brief Priority given to EVR's timestamp/event provider
 */
#define ER_PROVIDER_PRIORITY 50


epicsShareFunc
int EVRCurrentTime(epicsTimeStamp *pDest);

epicsShareFunc
int EVREventTime(epicsTimeStamp *pDest, int event);

#ifdef __cplusplus
}
#endif

#endif /* EVRGTIF_H_INC */
