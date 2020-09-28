/***************************************************************************************************
|* mrfFracSynth.h -- EPICS Support Routines for the Micrel SY87739L Fractional-N Synthesizer Chip
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund (LANSCE)
|* Date:     13 September 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 13 Sep 2006  E.Bjorklund     Original Release
|*
|*--------------------------------------------------------------------------------------------------
|* REFERENCE:
|* Micrel SY87739L Product Description.  Available at:
|*        http://www.micrel.com
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This header file contains the function templates for the utility functions used to create and
|* analyze control words for the Micrel SY87739L Fractional-N Synthesizer chip.
|* 
\**************************************************************************************************/


/**************************************************************************************************
|*                                     COPYRIGHT NOTIFICATION
|**************************************************************************************************
|*  
|* THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
|* AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
|* AND IN ALL SOURCE LISTINGS OF THE CODE.
|*
|**************************************************************************************************
|*
|* Copyright (c) 2006 Los Alamos National Security, LLC
|* as Operator of Los Alamos National Laboratory.
|*
|**************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included with this distribution.
|*
\*************************************************************************************************/

#ifndef SY87739L_H
#define SY87739L_H

/**************************************************************************************************/
/*  Other Header Files Required By This File                                                      */
/**************************************************************************************************/

#include <epicsTypes.h>                 /* EPICS Architecture-independent type definitions        */

#include <mrf/mrfCommonAPI.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/*  Function Prototypes for Fractional Synthesizer Utility Routines                               */
/**************************************************************************************************/

MRFCOMMON_API
epicsStatus   mrfSetEventClockSpeed (epicsFloat64,  epicsUInt32,  epicsFloat64,
                                                      epicsFloat64*, epicsUInt32*, epicsInt32);

MRFCOMMON_API
epicsUInt32   FracSynthControlWord  (epicsFloat64, epicsFloat64, epicsInt32, epicsFloat64*);

MRFCOMMON_API
epicsFloat64  FracSynthAnalyze      (epicsUInt32, epicsFloat64, epicsInt32);

/**************************************************************************************************/
/*  Special Macros to Define Commonly Used Symbols                                                */
/*  (note that these values can be overridden in the invoking module)                             */
/**************************************************************************************************/


/*---------------------
 * Success return code
 */
#ifndef OK
#define OK     (0)
#endif

/*---------------------
 * Failure return code
 */
#ifndef ERROR
#define ERROR  (-1)
#endif

/*---------------------
 * Success, but do not perform linear conversions (ai & ao record device support routines)
 */
#ifndef NO_CONVERT
#define NO_CONVERT (2)
#endif

#ifdef __cplusplus
}
#endif

#endif
