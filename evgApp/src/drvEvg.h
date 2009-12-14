/***************************************************************************************************
|* drvEvg.h -- Event Generator Driver Infrastructure Definitions
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   E.Bjorklund
|* Date:     06 November 2009
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 06 Nov 2009  E.Bjorklund     Original
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This header file provides interface definitions to the event generator global driver
|*   infrastructure module.
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

#ifndef DRV_EVG_H 
#define DRV_EVG_H

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
                                                                                
#include <mrfCommon.h>          // MRF event system constants and definitions
#include <debugPrint.h>         // SLAC debug print utility

#include <evg/evg.h>            // Event generator base class definition


/**************************************************************************************************/
/*  Symbol Definitions                                                                            */
/**************************************************************************************************/

//=====================
// Event clock source
//
#define EVG_CLOCK_SRC_INTERNAL  0       // Event clock is generated internally
#define EVG_CLOCK_SRC_RF        1       // Event clock is generated from the RF input port
#define EVG_CLOCK_SRC_MAX       1       // Maximum value for outgoing link clock source

/**************************************************************************************************/
/*  Common Variable Declarations                                                                  */
/**************************************************************************************************/

//=====================
// Event Generator Global Debug Flag (defined in the driver-support module)
//
#ifdef EVG_DRIVER_SUPPORT_MODULE
    epicsInt32          EvgGlobalDebugFlag = DP_INFO;
#else
    extern epicsInt32   EvgGlobalDebugFlag;
#endif


/**************************************************************************************************/
/*  Function Prototypes For Driver Infrastructure Support Routines                                */
/**************************************************************************************************/

//=====================
// Event Generator Interrupt Service Routine
//
extern "C" {
void            EgInterrupt      (EVG *pEvg);
}//end extern "C"

//=====================
// Routines that are callable from the event generator driver and device objects
//
void            EgAddCard        (epicsInt32 CardNum, EVG *pEvg);
EVG            *EgGetCard        (epicsInt32 CardNum);
bool            EgConfigDisabled ();
bool            EgInitDone       ();

void            EgConflictCheck  (epicsInt32   CardNum,
                                  epicsInt32   BusType,
                                  epicsInt32   SubUnit,
                                  const char  *SubUnitName);

//=====================
// Routines that can be called from the shell
//
extern "C" {
void            EgGlobalDebug    (epicsInt32 level);
void            EgDebug          (epicsInt32 CardNum, epicsInt32 level);
epicsStatus     EgReport         (epicsInt32 CardNum, epicsInt32 level);
}//end extern "C"

#endif // DRV_EVG_H
