/**************************************************************************************************
|* $(TIMING)/evgApp/src/devSequence.h -- EPICS Driver Support for EVG Sequencers
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
|*    This module contains the implementation of the Sequence class
|*
|*-------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator Cards
|*     Modular Register Mask
|*
|*-------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   vxWorks
|*   RTEMS
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|*  o This module does not support the APS-style register map because it relies on interrupts
|*    from the event generator card.  EVG interrupts are not supported in the APS register map.
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

#ifndef EVG_DEV_SEQUENCE_INC
#define EVG_DEV_SEQUENCE_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include  <Sequence.h>          // MRF Sequence Class


/**************************************************************************************************/
/*  Function Prototypes                                                                           */
/**************************************************************************************************/

Sequence  *EgDeclareSequence (epicsInt32 CardNum, epicsInt32 SeqNum);
Sequence  *EgGetSequence     (epicsInt32 CardNum, epicsInt32 SeqNum);


#endif // EVG_DEV_SEQUENCE_INC
