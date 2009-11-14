#define EVG_DRIVER_SUPPORT_MODULE

#include <stdexcept>            // Standard C++ exception definitions
#include <map>                  // Standard C++ map template

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include <drvSup.h>             // EPICS Driver support definitions
#include <initHooks.h>          // EPICS IOC Initialization hooks support library
#include <iocsh.h>              // EPICS IOC shell support library
#include <registryFunction.h>   // EPICS Registry support library

#include  <drvEvg.h>            // Event Generator driver infrastructure routines
#include  <evg/evg.h>           // Event Generator base class definitions

#include <epicsExport.h>        // EPICS Symbol exporting macro definitions

typedef std::map <epicsInt32, EVG*> CardList;

static bool       ConfigureLock = false;
static CardList   EvgCardList;

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
    * system is ready to start accepting interrupts.  We also know that all egevent records
    * have been initially processed, either during the "Initialize Database" phase or
    * the "Initial Process" phase (depending on the value of their PINI fields), and that 
    * their definitions can now be loaded into the Event Generator's sequence RAMs.
    */
    case initHookAfterInterruptAccept:
        DEBUGPRINT (DP_INFO, EvgGlobalDebugFlag,
                   ("drvEvg: EgInitHooks(AfterInterruptAccept) called\n"));

        //=====================
        // Loop to perform a "reboot initialization" for every event generator card we know about.
        //
        for (CardList::iterator Card = EvgCardList.begin();  Card != EvgCardList.end();  Card++) {
            pEvg = Card->second;
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
        pEvg->Report(level);
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
    return pEvg->Report(level);
 
}//end EgReport()
}//end extern "C"

/**************************************************************************************************/
/*                              EPICS IOC Shell Registery                                         */
/*                                                                                                */


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
    iocshRegister (&EgGlobalDebugDef , EgGlobalDebugCall );
    iocshRegister (&EgDebugDef       , EgDebugCall );
    iocshRegister (&EgReportDef      , EgReportCall );
}/*end drvEvgRegistrar()*/

extern "C" {
epicsExportRegistrar (drvEvgRegistrar);
}//end extern "C"
