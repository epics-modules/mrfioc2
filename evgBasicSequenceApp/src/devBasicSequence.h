/**************************************************************************************************
|* $(MRF)/evgBasicSequenceApp/src/devBasicSequence.h --  EPICS Device Support for Basic Sequences
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     14 January 2010
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 14 Jan 2010  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*    This module contains definitions for the EPICS Basic Sequence Event device support module.y
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

#ifndef EVG_DEV_BASIC_SEQ_INC
#define EVG_DEV_BASIC_SEQ_INC


/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions
#include  <BasicSequence.h>     // MRF Basic Sequence class


/**************************************************************************************************/
/*  Exported Utility Routine Prototypes                                                           */
/**************************************************************************************************/

BasicSequence*  EgDeclareBasicSequence (epicsInt32 Card, epicsInt32 SeqNum);


#endif // EVG_DEV_BASIC_SEQ_INC //
