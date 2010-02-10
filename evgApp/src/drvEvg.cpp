/**************************************************************************************************
|* $(MRF)/evgApp/src/drvEvg.cpp -- EPICS Generic Driver Support for Event Generator Cards
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     25 January 2010
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 25 Jan 2010  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*    This module contains EPICS generic driver support for event generator cards. It contains
|*    code that is not specific to any particular hardware implementation.  This includes:
|*      o Event Generator card configuration routines.
|*      o Routines for maintaining the global list of all event generator cards.
|*      o "InitHooks" routines for performing phased initialization
|*      o Driver entry table, driver initialization routine, and driver report routine
|*      o Event Generator top-level interrupt service routine
|*      o Other shell-callable driver support routines.
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

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#define EVG_DRIVER_SUPPORT_MODULE

#include <stdexcept>            // Standard C++ exception definitions
#include <map>                  // Standard C++ map template

#include <float.h>
#include <errno.h>              // Standard C global error number definitions
#include <math.h>               // Standard C math library
#include <string.h>             // Standard C string processing routines

#include <epicsStdlib.h>        // EPICS Standard C library support routines
#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include <drvSup.h>             // EPICS Driver support definitions
#include <initHooks.h>          // EPICS IOC Initialization hooks support library
#include <iocsh.h>              // EPICS IOC shell support library
#include <registryFunction.h>   // EPICS Registry support library

#include  <drvEvg.h>            // Event Generator driver infrastructure routines
#include  <drvSequence.h>       // Event Generator sequence driver infrastructure routines

#include  <evg/evg.h>           // Event Generator base class definitions

#include <epicsExport.h>        // EPICS Symbol exporting macro definitions

/**************************************************************************************************/
/*  Type Definitions                                                                              */
/**************************************************************************************************/

//=====================
// CardList:  Associates logical card numbers with EVG objects
//
typedef std::map <epicsInt32, EVG*> CardList;

/**************************************************************************************************/
/*  Variables Global To This Module                                                               */
/**************************************************************************************************/

static bool       ConfigureLock = false;        // Disables further calls to configuration rtns.
static bool       InitDone      = false;        // Indicates IOC initialization is complete
static CardList   EvgCardList;                  // List of EVG objects

/**************************************************************************************************/
/*  EVG Base Class Destructor                                                                     */
/**************************************************************************************************/

EVG::~EVG() {}

/**************************************************************************************************/
/*                            Event Generator Configuration Routines                              */
/*                                                                                                */


//==================================================================================================
//  Event Generator Configuration Group Definition
//==================================================================================================
//! @defgroup   EgConfigure Event Generator Configuration Routines
//! @brief      Shell callable event generator configuration routines
//!
//! These routines may be called from the startup script to configure event generator cards.
//! 
//! @{
//!
//==================================================================================================

//**************************************************************************************************
//  EgConfigInLink () -- Configure the Input Event Link Clock Speed
//**************************************************************************************************
//! @par Description:
//!   This routine will configure the clock speed for the incoming event link.  The incoming event
//!   link may have a different clock speed than the outgoing event link.  This feature allows
//!   you to merge two different event links with different clock speeds.  The speed of the
//!   incoming event link is determined by the fractional synthesizer chip.  Consequently, if the
//!   outgoing event link clock speed is different than the incoming link clock speed, the
//!   outgoing event link speed must be determined externally from the RF input port and not
//!   internally from the fractional synthesizer chip.
//!   <br>
//!   The valid range for the event clock speed is from 50 to 125 MHz.
//!
//! @param      Card        = (input)  Logical card number for the event generator card.
//! @param      ClockSpeed  = (input)  Event clock frequency (in MegaHertz) of the incoming
//!                                    event link.  
//!
//! @return     Returns OK if the incoming event link was successfully configured.<br>
//!             Returns ERROR if the event link clock speed could not be set.
//!
//! @note
//! - Ideally, this routine should be called after the event generator card has been configured
//!   but before iocInit() is called.
//! - The incoming clock frequency is specified as character strings in order
//!   to accomodate those shells that do not support floating point input.
//!
//**************************************************************************************************

extern "C" {
epicsStatus EgConfigInLink (epicsInt32 Card, char* ClockSpeed)
{
    //=====================
    // Local variables
    //
    epicsFloat64    Frequency = 0.0;    // Desired clock speed converted to floating point
    EVG*            pEvg;               // Pointer to event generator card object
    char*           tailPtr;            // Temp pointer used in string-to-double routine

    //=====================
    // Make sure the specified card has already been configured.
    //
    if (NULL == (pEvg = EgGetCard(Card))) {
        printf ("EgConfigInLink: Card %d has not been configured\n", Card);
        return ERROR;
    }//end if card was not configured

    //=====================
    // Translate the event clock speed into floating-point format.
    //
    if ((0 != ClockSpeed) && (strlen(ClockSpeed))) {
        errno = OK;
        Frequency = epicsStrtod (ClockSpeed, &tailPtr);

        if ((OK != errno) || (tailPtr == ClockSpeed) || (0.0==Frequency)) {
            printf ("EgConfigInLink: Invalid clock speed value for incoming event link\n");
            return ERROR;
        }//end if error converting clock speed
    }//end if ClockSpeed string is not null

    //=====================
    // Make sure that the clock frequency was specified
    //
    if (0.0 == Frequency) {
        printf ("EgConfigInLink: Incoming event link speed was not specified.\n");
        return ERROR;
    }//end if frequency not specified

    //=====================
    // Set the clock frequency and return
    //
    pEvg->SetInLinkClockSpeed (Frequency);
    return OK;

}//end EgConfigInLink()
}//end extern "C"

//**************************************************************************************************
//  EgConfigOutLink () -- Configure Output Event Link Clock Speed and Source
//**************************************************************************************************
//! @par Description:
//!   This routine will configure the clock speed and the clock source for the outgoing event link.
//!   <br>
//!   The valid range for the event clock speed is from 50 to 125 MHz.
//!   <br>
//!   The clock source may either be "Internal" (generated by the on-board fractional synthesizer
//!   chip) or "RF" (derived from the RF signal input).
//!
//! @param      Card        = (input)  Logical card number for the event generator card.
//! @param      ClockSpeed  = (input)  Event clock frequency (in MegaHertz) of the outgoing
//!                                    event link.  
//! @param      ClockSource = (input)  Specifies how the outgoing event link clock is generated.
//!                                    Legal values are:<br>
//!                                    "Internal" - Output link event clock is generated internally
//!                                                 using the fractional synthesizer chip.<br>
//!                                    "RF"       - Output link is generated from an RF signal
//!                                                 on the RF input port.<br>
//!                                    0 or empty - Use system default.
//!
//! @return     Returns OK if the outgoing event link was successfully configured.<br>
//!             Returns ERROR if the event link clock speed or source could not be set.
//!
//! @note
//! - Ideally, this routine should be called after the event generator card has been configured
//!   but before iocInit() is called.
//! - The outgoing clock frequency is specified as character strings in order
//!   to accomodate those shells that do not support floating point input.
//!
//**************************************************************************************************

extern "C" {
epicsStatus EgConfigOutLink (epicsInt32 Card, char* ClockSpeed, char* ClockSource)
{
    //=====================
    // Local variables
    //
    epicsFloat64    Frequency = 0.0;    // Desired clock speed converted to floating point
    epicsInt16      Source = -1;        // Code for clock source
    EVG*            pEvg;               // Pointer to event generator card object
    char*           tailPtr;            // Temp pointer used in string-to-double routine

    //=====================
    // Make sure the specified card has already been configured.
    //
    if (NULL == (pEvg = EgGetCard(Card))) {
        printf ("EgConfigOutLink: Card %d has not been configured\n", Card);
        return ERROR;
    }//end if card was not configured

    //=====================
    // Translate the outgoing event clock speed into floating-point format.
    //
    if ((0 != ClockSpeed) && (strlen(ClockSpeed))) {
        errno = OK;
        Frequency = epicsStrtod (ClockSpeed, &tailPtr);

        if ((OK != errno) || (tailPtr == ClockSpeed) || (0.0==Frequency)) {
            printf ("EgConfigOutLink: Invalid clock speed value for outgoing event link\n");
            return ERROR;
        }//end if error converting output clock speed
    }//end if ClockSpeed string is not null

    //=====================
    // Make sure that the outgoing clock frequency was specified
    //
    if (0.0 == Frequency) {
        printf ("EgConfigOutLink: Outgoing event link speed was not specified.\n");
        return ERROR;
    }//end if outgoing frequency not specified

    //=====================
    // Parse the clock source
    //
    if ((0 != ClockSource) && (strlen(ClockSource))) {
        if (0 == strcmp("Internal", ClockSource))
            Source = EVG_CLOCK_SRC_INTERNAL;
        else if (0 == strcmp("RF", ClockSource))
            Source = EVG_CLOCK_SRC_RF;
        else
            printf ("EgConfigOutLink: Invalid source specification (%s).\n", ClockSource);
    }//end if clock source parameter was specified

    //=====================
    // Set the clock source first (if it was specified)
    //
    if (Source != -1)
        pEvg->SetOutLinkClockSource (Source);

    //=====================
    // Set the clock frequency and return
    //
    pEvg->SetOutLinkClockSpeed (Frequency);
    return OK;

}//end EgConfigOutLink()
}//end extern "C"

//!
//| @}
//end group EgConfigure

void
EgAddCard (epicsInt32 CardNum, EVG *pEvg) {
    EvgCardList[CardNum] = pEvg;
}//end EgAddCard()

EVG*
EgGetCard (epicsInt32 CardNum) {

    //=====================
    // Look up the requested event generator in the map
    //
    CardList::iterator Card = EvgCardList.find(CardNum);

    //=====================
    // Return a a pointer to the event generator object.
    // If the card was not found, return a null pointer.
    //
    if (Card != EvgCardList.end())
        return Card->second;

    return NULL;

}//end EgGetCard()

void
EgConflictCheck (
    epicsInt32   CardNum,       // Logical card number
    epicsInt32   BusType,       // Which type of bus the card is on
    epicsInt32   SubUnit,       // Bus sub-unit number
    const char  *SubUnitName)   // Bus-specific name for the sub-unit
{
    //=====================
    // Local Variables
    //
    EVG    *pEvg;               // Pointer to event generator object
    char    errorString [128];  // Buffer for constructing error messages

    //=====================
    // Check the list of configured event generator cards to see if:
    //   a) The card number is already in use
    //   b) There already is a configured event generator at the desired bus sub-unit
    //
    for (CardList::iterator Card = EvgCardList.begin();  Card != EvgCardList.end();  Card++) {

        //=====================
        // Get pointer to the next event generator card in the list
        //
        pEvg = Card->second;

        //=====================
        // Check to see if we have already configured an event generator with this card number
        //
        if (CardNum == pEvg->GetCardNum()) {
            sprintf (errorString, "Event Generator card %d has already been configured at %s %d",
                     CardNum, pEvg->GetSubUnitName(), pEvg->GetSubUnit());
            throw std::runtime_error(errorString);
        }//end if logical card number is already in the list

        //=====================
        // Check to see if we have already configured an event generator at this bus sub-unit
        //
        if  ((BusType == pEvg->GetBusType()) && (SubUnit == pEvg->GetSubUnit())) {
            sprintf (errorString, "Event Generator card %d has already been configured at %s %d",
                     pEvg->GetCardNum(), SubUnitName, SubUnit);
            throw std::runtime_error(errorString);
        }//end if an event generator card has already been configured at this sub-unit

    }//end for each configured event generator card

}//end EgConflictCheck()

bool EgConfigDisabled () {
    return (ConfigureLock);
}//end EgConfigDisabled()

bool EgInitDone() {
    return (InitDone);
}//end EgInitDone()

/**************************************************************************************************
|* EgInitHooks () -- Perform Initialization Functions at Specific Points During IOC Initialization
|*-------------------------------------------------------------------------------------------------
|*
|* This routine that is designed to be called from device or driver support initialization
|* routines.  It completes the initialization of the event generator hardware at specific
|* stages during the iocInit() process in order to facilitate "bumpless" soft reboots.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o AfterInitRecSup
|*      Perform a "Reboot Initialization" on each of the registered event generator cards.
|*      This will typically involve checking the "Enable" state of the card to determine
|*      whether this is a hard or soft reboot.  If it is a "hard" reboot (card was disabled),
|*      initialize the hardware to a default "safe" state.
|*
|*    o AfterInterruptAccept
|*      Invoke the IntEnable() methof for each registered event generator card.
|*      Sequence RAM loading is usually postponed until this step in order allow all
|*      the sequence RAM records to be initialized first.
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      State              = (initHooksState)  Current state of the IOC initialization process
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS:
|*      EvgCardList         = (CardList)       List of all configured event generator cards
|*      EvgGlobalDebugFlag  = (epicsInt32)     Event generator global debug level
|*
\**************************************************************************************************/

static
void EgInitHooks (initHookState State)
{
    //=====================
    // Local variable declarations
    //
    EVG  *pEvg;  // Pointer to event generator object

    //=====================
    // Select the current IOC initialization state
    //
    switch (State) {

   /*---------------------------------------------------------------------------------------------*
    * Before Initial Calls To The Device Support "init" Functions                                 *
    *---------------------------------------------------------------------------------------------*
    * At this point in the IOC initialization process, we are about to call the device support
    * "init" routines for the first time. Check the master enable bit in each Event Generator
    * card's control register to see if this is a hard or soft reboot.  If it is a hard reboot,
    * place the Event Generator in a "Null" initial state before device support is called.
    */
    case initHookAfterInitRecSup:
        DEBUGPRINT (DP_INFO, EvgGlobalDebugFlag,
                   ("drvEvg: EgInitHooks(AfterInitDevSup) called\n"));

        //=====================
        // Loop to perform a "reboot initialization" for every event generator card we know about.
        //
        for (CardList::iterator Card = EvgCardList.begin();  Card != EvgCardList.end();  Card++) {
            pEvg = Card->second;
            pEvg->RebootInit();
        }//end for each configured event generator card

        break;

   /*---------------------------------------------------------------------------------------------*
    * After It Is Safe To Accept Interrupts                                                       *
    *---------------------------------------------------------------------------------------------*
    * At this point we know that the IOC initialization process is almost complete and the
    * system is ready to start accepting interrupts.  We also know that all sequence and
    * sequence event records have been initially processed, either during the "Initialize Database"
    * phase or the "Initial Process" phase (depending on the value of their PINI fields), and that 
    * their definitions can now be loaded into the Event Generator's sequence RAMs.
    */
    case initHookAfterInterruptAccept:
        DEBUGPRINT (DP_INFO, EvgGlobalDebugFlag,
                   ("drvEvg: EgInitHooks(AfterInterruptAccept) called\n"));

        //=====================
        // Indicate that initialization is complete
        //
        InitDone = true;

        //=====================
        // Loop to complete the initialization for every event generator card we know about.
        //
        for (CardList::iterator Card = EvgCardList.begin();  Card != EvgCardList.end();  Card++) {
            pEvg = Card->second;

            //=====================
            // Finalize the sequences associated with this EVG
            //
            EgFinalizeSequences (pEvg->GetCardNum());

            //=====================
            // Enable interrupts on this EVG
            //
            pEvg->IntEnable();

        }//end for each configured event generator card

        break;

   /*---------------------------------------------------------------------------------------------*
    * Ignore All Other States                                                                     *
    *---------------------------------------------------------------------------------------------*
    */
    default:
        break;

    }/*end select initialization state*/

}/*end EgInitHooks()*/

/**************************************************************************************************/
/*                           EPICS Device Driver Entry Table Routines                             */
/*                                                                                                */


/**************************************************************************************************
|* EgDrvInit () -- Driver Initialization Routine
|*-------------------------------------------------------------------------------------------------
|*
|* This routine is called from the EPICS iocInit() routine. It gets called prior to any of the
|* device or record support initialization routines.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o Disable any further calls to the EgConfigure routine.
|*    o Register an initHooks routine to distribute the hardware initialization in such a way
|*      as to allow for "bumpless" soft reboots.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT OUTPUTS:
|*      ConfigureLock = (epicsBoolean) Set to TRUE, to disable any further calls to the
|*                                     "EgConfigure" routine.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      Returns OK if there were no problems encountered.
|*      Returns ERROR if we could not register the initHooks routine.
|*
\**************************************************************************************************/

static
epicsStatus EgDrvInit (void)
{
    //=====================
    // Output a debug message if the debug flag is set.
    //
    DEBUGPRINT (DP_INFO, EvgGlobalDebugFlag, ("drvEvg: EgDrvInit() entered\n"));

    //=====================
    // Prevent any future event generator configuration calls
    //
    ConfigureLock = true;

    //=====================
    // Register an "initHooks" routine so that we can distribute the hardware initialization
    // in a fashion that will allow for "bumpless" soft reboots.
    //
    if (OK != initHookRegister(EgInitHooks)) {
        DEBUGPRINT (DP_FATAL, EvgGlobalDebugFlag,
                   ("EgDrvInit: ERROR - Unable to set up init-hooks routine.\n"));
        return ERROR;
    }//end if could not register our init-hooks routine

    return OK;

}//end EgDrvInit()

/**************************************************************************************************
|* EgDrvReport () -- Event Generator Driver Report Routine
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   This routine gets called by the EPICS "dbior" routine.  It invokes the event generator
|*   object's Report() routine for each configured event generator.
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      level       = (epicsInt32) Indicator of how much information is to be displayed
|*                                 Larger numbers produce more information.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      Always returns OK;
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS:
|*      EvgCardList = (CardList) List of all configured event generator cards
|*
\**************************************************************************************************/

epicsStatus EgDrvReport (epicsInt32 level)
{
    epicsInt32   NumCards = 0;       // Number of configured cards we found
    EVG         *pEvg;               // Pointer to event generator object

    //=====================
    // Print report header
    //
    printf ("-------------------- MRF Event Generator Hardware Report --------------------\n");

    //=====================
    // Loop to report on each configured event generator card
    //
    for (CardList::iterator Card = EvgCardList.begin();  Card != EvgCardList.end();  Card++) {
        pEvg = Card->second;

        //=====================
        // Hardware-specific report
        //
        pEvg->Report(level);

        //=====================
        // Report any associated event sequences
        //
        if (level > 0)
            EgReportSequences (pEvg->GetCardNum(), level);

        NumCards++;
    }//end for each configured Event Generator Card

    //=====================
    // If we didn't find any configured Event Generator cards, say so.
    //
    if (!NumCards)
        printf ("  No event generator cards were configured\n");

    //=====================
    // Always return success
    //
    return OK;

}//end EgDrvReport()

/**************************************************************************************************/
/*  Driver Support Entry Table (DRVET)                                                            */
/**************************************************************************************************/

extern "C" {
static
drvet drvMrfEvg = {
    2,                                  // Number of entries in the table
    (DRVSUPFUN)EgDrvReport,             // Driver Support Layer device report routine
    (DRVSUPFUN)EgDrvInit                // Driver Support layer device initialization routine
};

epicsExportAddress (drvet, drvMrfEvg);

};//end extern "C"

/**************************************************************************************************/
/*                                   Interrupt Service Routine                                    */
/*                                                                                                */
extern "C" {

void
EgInterrupt (EVG *pEvg) {
    pEvg->Interrupt();
}//end EgInterrupt()

};//end extern "C"

/**************************************************************************************************/
/*                            Shell Callable Driver Utility Routines                              */
/*                                                                                                */

/**************************************************************************************************
|* EgGlobalDebug () -- Set the Event Generator Global Debug Flag to the Desired Level.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   This routine may called from the startup script or the EPICS/IO shell to control the amount of
|*   debug information produced by event generator drivers and device support code.  It takes one
|*   input parameter, which specifies the desired debug level.
|*
|*   The global debug level applies to all event generator cards that have not declared a local
|*   debug level (using the "EgDebug()" routine) and all general-purpose event generator routines,
|*   such as the ones defined in this module.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      EgGLobalDebug (level);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      level               = (epicsInt32) New global debug level.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT OUTPUTS:
|*      EvgGlobalDebugFlag  = (epicsInt32) Event generator global debug flag
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* Debug printing uses the SLAC "debugPrint" utility. Debug levels are defined as:
|*    0: DP_NONE -  No debug output is produced
|*    1: DP_FATAL - Print messages only for fatal errors
|*    2: DP_ERROR - Print fatal and non-fatal error messages 
|*    3: DP_WARN  - Print warning, fatal and non-fatal error messages
|*    4: DP_INFO  - Print informational messages, warnings, and errors
|*    5: DP_DEBUG - Print all of the above plus special messages inserted for code debugging.
|*
\**************************************************************************************************/

extern "C" {
void EgGlobalDebug (epicsInt32 level) {

    EvgGlobalDebugFlag = level;

}//end EgGlobalDebug()
}//end extern "C"

/**************************************************************************************************
|* EgDebug () -- Set The Desired Debug Level For A Specific Event Generator Card
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   This routine may called from the startup script or the EPICS/IO shell to set the debug level
|*   for one particular event generator card.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      EgGLobalDebug (CardNum, level);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      CardNum = (epicsInt32) Logical card number to set the debug level for
|*      level   = (epicsInt32) Positive numbers will set the card-specific debug level.
|*                             Negative numbers will revert to the global debug level.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|* Debug printing uses the SLAC "debugPrint" utility. Debug levels are defined as:
|*    0: DP_NONE -  No debug output is produced
|*    1: DP_FATAL - Print messages only for fatal errors
|*    2: DP_ERROR - Print fatal and non-fatal error messages 
|*    3: DP_WARN  - Print warning, fatal and non-fatal error messages
|*    4: DP_INFO  - Print informational messages, warnings, and errors
|*    5: DP_DEBUG - Print all of the above plus special messages inserted for code debugging.
|*
\**************************************************************************************************/

extern "C" {
void EgDebug (epicsInt32 CardNum,  epicsInt32 level) {

    //=====================
    // Look up the requested card number
    //
    EVG *pEvg = EgGetCard (CardNum);
    if (NULL == pEvg) {
        printf ("Event Generator Card %d has not been configured\n", CardNum);
        return;
    }//end if requested card was not configured

    //=====================
    // Set the card-specific debug level
    //
    pEvg->SetDebugLevel (level);

}//end EgDebug()
}//end extern "C"

/**************************************************************************************************
|* EgReport () -- Event Generator Card Report Routine
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   This routine may be called by the IOC shell.  It looks up the requested logical card number
|*   and invokes that card's Report() routine.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = EgReport (CardNum, level);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      CardNum     = (epicsInt32) Logical card number of the event generator we wish
|*                                 to get a report of
|*      level       = (epicsInt32) Indicator of how much information is to be displayed
|*                                 Larger numbers produce more information.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      OK:     If we successfully reported on the specified card
|*      ERROR:  If the requested card number was not configured or the report failed.
|*
\**************************************************************************************************/

extern "C" {
epicsStatus EgReport (epicsInt32 CardNum, epicsInt32 level) {

    //=====================
    // Look up the requested card number
    //
    EVG *pEvg = EgGetCard (CardNum);
    if (NULL == pEvg) {
        printf ("Event Generator Card %d has not been configured\n", CardNum);
        return ERROR;
    }//end if requested card was not configured

    //=====================
    // If the requested card was configured, call its Report() routine
    //
    pEvg->Report(level);
    if (level > 0) EgReportSequences (pEvg->GetCardNum(), level);
    return OK;
 
}//end EgReport()
}//end extern "C"

/**************************************************************************************************/
/*                              EPICS IOC Shell Registery                                         */
/*                                                                                                */


/**************************************************************************************************/
/*   EgConfigInLink() -- Configure The Incoming Event Link Clock                                  */
/**************************************************************************************************/

static const iocshArg         EgConfigInLinkArg0     = {"Card"       , iocshArgInt};
static const iocshArg         EgConfigInLinkArg1     = {"ClockSpeed" , iocshArgString};
static const iocshArg *const  EgConfigInLinkArgs[2]  = {&EgConfigInLinkArg0,
                                                        &EgConfigInLinkArg1};
static const iocshFuncDef     EgConfigInLinkDef      = {"EgConfigInLink", 2, EgConfigInLinkArgs};

static
void EgConfigInLinkCall (const iocshArgBuf * args) {
    EgConfigInLink (args[0].ival, args[1].sval);
}/*end EgConfigInLinkCall()*/


/**************************************************************************************************/
/*   EgConfigOutLink() -- Configure The Outgoing Event Link Clock                                 */
/**************************************************************************************************/

static const iocshArg         EgConfigOutLinkArg0    = {"Card"       , iocshArgInt};
static const iocshArg         EgConfigOutLinkArg1    = {"ClockSpeed" , iocshArgString};
static const iocshArg         EgConfigOutLinkArg2    = {"ClockSource", iocshArgString};
static const iocshArg *const  EgConfigOutLinkArgs[3] = {&EgConfigOutLinkArg0,
                                                        &EgConfigOutLinkArg1,
                                                        &EgConfigOutLinkArg2};
static const iocshFuncDef     EgConfigOutLinkDef     = {"EgConfigOutLink", 3, EgConfigOutLinkArgs};

static
void EgConfigOutLinkCall (const iocshArgBuf * args) {
    EgConfigOutLink (args[0].ival, args[1].sval, args[2].sval);
}/*end EgConfigOutLinkCall()*/


/**************************************************************************************************/
/*   EgGlobalDebug() -- Set Global Debug Level                                                    */
/**************************************************************************************************/

static const iocshArg         EgGlobalDebugArg0    = {"level", iocshArgInt};
static const iocshArg *const  EgGlobalDebugArgs[1] = {&EgGlobalDebugArg0};

static const iocshFuncDef     EgGlobalDebugDef     = {"EgGlobalDebug", 1, EgGlobalDebugArgs};

static
void EgGlobalDebugCall (const iocshArgBuf *args) {
    EgGlobalDebug (args[0].ival);
}//end EgGlobalDebugCall()


/**************************************************************************************************/
/*   EgDebug() -- Set Card-Specific Debug Level                                                   */
/**************************************************************************************************/

static const iocshArg         EgDebugArg0    = {"CardNum", iocshArgInt};
static const iocshArg         EgDebugArg1    = {"level",   iocshArgInt};
static const iocshArg *const  EgDebugArgs[2] = {&EgDebugArg0, &EgDebugArg1};

static const iocshFuncDef     EgDebugDef     = {"EgDebug", 2, EgDebugArgs};

static
void EgDebugCall (const iocshArgBuf *args) {
    EgDebug (args[0].ival, args[1].ival);
}//end EgDebugCall()


/**************************************************************************************************/
/*   EgReport() -- Device Report on One Card                                                      */
/**************************************************************************************************/

static const iocshArg         EgReportArg0    = {"CardNum", iocshArgInt};
static const iocshArg         EgReportArg1    = {"level",   iocshArgInt};
static const iocshArg *const  EgReportArgs[2] = {&EgReportArg0, &EgReportArg1};

static const iocshFuncDef     EgReportDef     = {"EgReport", 2, EgReportArgs};

static
void EgReportCall (const iocshArgBuf *args) {
    EgReport (args[0].ival, args[1].ival);
}//end EgReportCall()


/**************************************************************************************************/
/*  EPICS Registrar Function for this Module                                                      */
/**************************************************************************************************/

static void drvEvgRegistrar() {
    iocshRegister (&EgConfigInLinkDef  , EgConfigInLinkCall  );
    iocshRegister (&EgConfigOutLinkDef , EgConfigOutLinkCall );
    iocshRegister (&EgGlobalDebugDef   , EgGlobalDebugCall   );
    iocshRegister (&EgDebugDef         , EgDebugCall         );
    iocshRegister (&EgReportDef        , EgReportCall        );
}/*end drvEvgRegistrar()*/

extern "C" {
epicsExportRegistrar (drvEvgRegistrar);
}//end extern "C"
