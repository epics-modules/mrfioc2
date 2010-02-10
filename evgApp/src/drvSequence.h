/**************************************************************************************************
|* $(MRF)/evgApp/src/drvSequence.h -- EPICS Driver Support for EVG Sequencers
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     23 November 2009
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 23 Nov 2009  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*    This module provides function prototypes for the MRF event generator sequence driver
|*
|*    An event generator sequence is an abstract object that has no hardware implementation.
|*    Its purpose is to provide the event and timestamp lists used by the EVG sequence RAM
|*    objects.
|*
|*-------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator Cards
|*
|*-------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   All
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
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included with this distribution.
|*
\*************************************************************************************************/

#ifndef EVG_DRV_SEQUENCE_INC
#define EVG_DRV_SEQUENCE_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions
#include  <evg/Sequence.h>      // MRF Sequence Base Class


/**************************************************************************************************/
/*  Function Prototypes                                                                           */
/**************************************************************************************************/

void       EgAddSequence       (Sequence* pSeq);
Sequence  *EgGetSequence       (epicsInt32 CardNum, epicsInt32 SeqNum);
void       EgFinalizeSequences (epicsInt32 CardNum);
void       EgReportSequences   (epicsInt32 CardNum, epicsInt32 Level);

#endif // EVG_DRV_SEQUENCE_INC
