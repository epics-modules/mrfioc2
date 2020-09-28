/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef EVRGTIF_H_INC
#define EVRGTIF_H_INC

#include <epicsTypes.h>
#include <epicsTime.h>
#include <evr/evrAPI.h>

#ifdef __cplusplus
extern "C" {
#endif

/*General Time Interface
 *
 * C interface to the timestamp supply functions
 * of the EVR interface for use with generalTime.
 */

/* Must be called before other functions.  Returns non-zero on error */
EVR_API
int EVRInitTime();

EVR_API
int EVRCurrentTime(epicsTimeStamp *pDest);

EVR_API
int EVREventTime(epicsTimeStamp *pDest, int event);

#ifdef __cplusplus
}
#endif

#endif /* EVRGTIF_H_INC */
