/**************************************************************************************************
|* $(TIMING)/mrfCommon/src/mrfCommon.c -- Miscellaneous Common Utility Routines
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     4 January 2010
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 04 Jan 2010  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*    This module contains miscellaneous utility routines that are useful to the MRF event system
|*    software.
|*
|*-------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*    All
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

/***************************************************************************************************
 *  mrfCommon Group Definition
 **************************************************************************************************/
/**
 * @defgroup   mrfCommon Common Utility Routines
 * @brief      Utility routines used by the MRF event system software.
 *
 * The common utility routines and definitions are found in the \$(TIMING)/mrfCommon directory.
 * The utilities in this directory include:
 * - \b  mrfFracSynth - Routines for translating between event clock frequency and
 *                      fractional synthesizer control words
 * - \b  mrfIoLink    - Parses the parameter list specified in the INP or OUT link fields.
 * - \b  mrfIoOps     - Macros for performing register-based I/O operations
 * - \b  mrfCommon    - Miscellaneous utility routines not fitting into any of the above catagories.
 *
 * @{
 *
 **************************************************************************************************/

/***************************************************************************************************
 * mrfCommon File Description
 **************************************************************************************************/
/**
 * @file       mrfCommon.c
 * @brief      Miscellaneous common utility routines useful to the MRF event system software.
 *
 **************************************************************************************************/


/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <dbCommon.h>          /* EPICS Common record field definitions                          */
#include  <alarm.h>             /* EPICS Alarm status and severity definitions                    */
#include  <mrfCommon.h>         /* MRF Common definitions                                         */

/***************************************************************************************************
 * mrfDisableRecord () -- Disable a Record From Ever Being Processed
 **************************************************************************************************/
 /**
 * @par Description:
 *   Renders an EPICS record incapable of ever being processed.
 *
 * @par Function:
 * - Set the "Processing Active" (PACT) field to "true"
 * - Set the "Disable putFields" (DISP) field to "true"
 * - Set the "Disable Value" (DISV) equal to the "Disable Link Value" (DISA)
 * - Set the record status field (STAT) to "DISABLE_ALARM"
 * - Set the record severity field (SEVR) to "INVALID_ALARM"
 *
 * @param   pRec = (input) Pointer to the record to be disabled.
 *
 **************************************************************************************************/

void mrfDisableRecord (dbCommon *pRec)
{
    pRec->pact = pRec->disp = 1;
    pRec->disv = pRec->disa;
    pRec->stat = DISABLE_ALARM;
    pRec->sevr = pRec->diss = INVALID_ALARM;

}/*end mrfDisableRecord()*/

/**
 * @}
 */
