/**************************************************************************************************
|* $(TIMING)/evgMrmApp/src/evgMrm.cpp -- Modular Register Map Event Generator Class
|*
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     9 November 2009
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 09 Nov 2009  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This module contains the implementation for the modular register map version of the
|*   Micro Research Finland (MRF) 2xx-series event generator card.
|*
|*--------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator Cards
|*     Modular Register Mask
|*
|*--------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   vxWorks
|*   RTEMS
|*
\*************************************************************************************************/

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

#include <stdexcept>               // Standard C++ exception class

#include <epicsTypes.h>            // EPICS Architecture-independent type definitions
#include <epicsMutex.h>            // EPICS Mutex support library
#include <iocsh.h>                 // EPICS IOC shell support library
#include <registryFunction.h>      // EPICS Registry support library

#include <mrfCommon.h>             // MRF Common definitions
#include <mrfCommonIO.h>           // MRF Common I/O macros
#include <mrfBusInterface.h>       // MRF Bus interface base class definition
#include <mrfVmeBusInterface.h>    // MRF VME Bus interface class definition

#include <drvEvg.h>                // Event generator driver infrastructure declarations
#include <evgMrm.h>                // Modular register map event generator class definition
#include <evgRegMap.h>             // Modular register map event generator register definitions

#include <epicsExport.h>           // EPICS Symbol exporting macro definitions

/**************************************************************************************************/
/*                          Bus-Specific Configuration Routines                                   */
/*                                                                                                */


/**************************************************************************************************
|* EgConfigureVME () -- Configuration Routine for VME Event Generator Cards
|*-------------------------------------------------------------------------------------------------
|*
|* This routine is called from the startup script to initialize a VME event generator card's
|* address, interrupt vector, and interrupt level. It is called after the EPICS code is loaded
|* but before iocInit() is called.
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   o Create a VME bus interface object for this card number
|*   o Create a modular register map EVG that will use the VME bus interface object created above.
|*   o Perform initial card initialization by invoking the card's "Configure()" method.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = EgConfigureVME (CardNum, Slot, VmeAddress, IntVector, IntLevel);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      CardNum     = (epicsInt32)  Logical card number for this Event Generator card.
|*      Slot        = (epicsInt32)  Physical VME slot number for this Event Generator card.
|*      VmeAddress  = (epicsUInt32) VME A24 address for this card's register map.
|*      IntVector   = (epicsInt32)  Interrupt vector for this card.
|*      IntLevel    = (epicsInt32)  VME interrupt request level for this card.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status      = (epicsStatus) Returns OK if the routine completed successfully.
|*                                  Returns ERROR if the routine could not configure the
|*                                  requested Event Generator card.
|*
\**************************************************************************************************/

epicsStatus
EgConfigureVME (
    epicsInt32    CardNum,  	     // Logical card number
    epicsInt32    Slot,              // VME slot
    epicsUInt32   VmeAddress,        // Desired VME address in A24 space
    epicsInt32    IntVector,         // Desired interrupt vector number
    epicsInt32    IntLevel)          // Desired interrupt level
{
    //=====================
    // Make sure card configuration is still enabled
    //
    if (EgConfigDisabled()) {
        printf ("EgConfigureVME: Event generator card configuration is no longer enabled\n");
        return ERROR;
    }//end if card configuration is no longer enabled

    //=====================
    // Local Variables
    //
    mrfBusInterface  *BusInterface;  // Pointer to the bus interface object
    evgMrm           *pEvg;          // Pointer to the event generator card object

    //=====================
    // Try to create a VME bus interface object
    //
    try {
        BusInterface = new mrfVmeBusInterface (
            CardNum,            // Logical card number
            MRF_CARD_TYPE_EVG,  // Card type is "Event Generator"
            Slot,               // VME slot number
            VmeAddress,         // Requested VME address
            IntVector,          // Interrupt vector
            IntLevel);          // Interrupt request level
    }//end try to create bus interface object

    catch (std::exception& e) {
        printf ("EgConfigureVME: Unable to create bus interface object for EVG card %d\n", CardNum);
        printf ("  %s\n", e.what());
        return ERROR;
    }//end if could not create the bus interface

    //=====================
    // Now try to create and configure a Modular Register Map Event Generator card
    // with a VME interface.
    //
    try {pEvg = new evgMrm (BusInterface);}
    catch (std::exception& e) {
        printf ("EgConfigureVME: Unable to create EVG card %d\n", CardNum);
        printf ("  %s\n", e.what());
        delete BusInterface;
        return ERROR;
    }//end if could not create the event generator card

    //=====================
    // Configure the newly created card
    //
    try {pEvg->Configure();}
    catch (std::exception& e) {
        printf ("EgConfigureVME: Unable to configure EVG card %d\n", CardNum);
        printf ("  %s\n", e.what());
        delete pEvg;
        return ERROR;
    }//end if could not create the event generator card

    //=====================
    // If we made it this far, the configuration succeeded
    //
    return OK;

}//end EgConfigureVME()

/**************************************************************************************************/
/*                              Class Member Function Definitions                                 */
/*                                                                                                */


/**************************************************************************************************
|* evgMrm () -- Class Constructor
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   Constructs a VME bus interface object and initializes its data members
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      new evgMrm (BusInterface);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUTS:
|*      BusInterface  = (mrfBusInterface *) Pointer to the hardware bus interface object
|*
\**************************************************************************************************/

evgMrm::evgMrm (mrfBusInterface *BusInterface) :

    //=====================
    // Initialize the card-related "meta data"
    //
    CardNum(BusInterface->GetCardNum()),         // Set the logical card number
    CardLock(0),                                 // No card lock yet
    DebugFlag(&EvgGlobalDebugFlag),              // Use the global EVG debug level flag

    //=====================
    // Initialize the bus-related data
    //
    BusInterface(BusInterface),
    BusType(BusInterface->GetBusType()),         // Set the bus type (VME, PCI, etc.)
    SubUnit(BusInterface->GetSubUnit()),         // Set the bus sub-unit value (VME Slot, PCI Index)

    //=====================
    // Initialize the hardware-related data
    //
    pReg(0)                                     // No register map address yet.

{}//end Constructor

/**************************************************************************************************
|* SetDebugLevel () -- Set The Debug Level That Will Be Used For This Card
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   Set the debug level to be used for this card.
|*
|*   If the debug level for the card has not been explicitly set, it will use the global
|*   event generator level defined by the external variable "EvgGlobalDebugFlag"
|*
|*   If the call to this routine sets the debug level to a positive value, this card will
|*   use that value and ignore the global debug level.
|*
|*   If the call to this routine sets the debug level to a negative value, this card will 
|*   revert to the global debug level.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      SetDebugLevel (level);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      level               = (epicsInt32)   Positive numbers will set the card-specific debug level
|*                                           Negative numbers will revert to the global debug level.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS (external):
|*      EvgGlobalDebugFlag  = (epicsInt32)   Event generator global debug flag
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT OUTPUTS (member variables):
|*      DebugFlag           = (epicsInt32 *) Pointer to debug flag source
|*      LocalDebugFlag      = (epicsInt32)   Local debug level
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
void
evgMrm::SetDebugLevel (epicsInt32 level) {

    //=====================
    // If the level is negative, set the level pointer
    // to use the global debug level
    //
    if (level < 0) {
        DebugFlag = &EvgGlobalDebugFlag;
    }//end if level is negative

    //=====================
    // If the level is not negative, set the local debug flag
    // and set the level pointer to use the local flag.
    //
    else {
        LocalDebugFlag = level;
        DebugFlag = &LocalDebugFlag;
    }//end if level is not negative

}//end SetDebugLevel()

/**************************************************************************************************
|* Configure () -- Event Generator Card Configuration Routine
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   This routine is called by one of the bus-specific configuration routines which are invoked
|*   from the startup script after the EPICS code is loaded but before iocInit() is called.
|*   o Register the requested hardware address with the EPICS device support library and perform
|*     any bus-specific intialization.
|*   o Disable interrupts from the card.
|*   o Connect the generic event generator interrupt handler to the card's interrupt vector
|*   o Initialize hardware information for this card.
|*
|*-------> To Be Done........
|*   o If the card has not previously been initialized (master enable is FALSE), reset the hardware.
|*   o Make sure the on-board reference clock is set to the appropriate frequencey and all error
|*     flags are reset.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      Configure ();
|*
|*-------------------------------------------------------------------------------------------------
|* THROWS:
|*      runtime_error
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS (member variables):
|*      BusInterface  = (mrfBusInterface *)  Pointer to the hardware bus interface object
|*      CardNum       = (epicsInt32)         Logical card number
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT OUTPUTS (member variables):
|*      CardLock      = (epicsMutexId)       Mutex to lock access to this card.
|*      FPGAVersion   = (epicsUInt32)        Card's firmware revision number
|*
\**************************************************************************************************/

void
evgMrm::Configure ()
{

    //=====================
    // Create the card locking mutex
    //
    if (0 == (CardLock = epicsMutexCreate()))
        throw std::runtime_error("Unable to create mutex for event generator card");

    //=====================
    // Try to register this card at the requested bus address space
    //
    if (0 == (pReg = BusInterface->ConfigBusAddress (EVG_REGMAP_SIZE)))
        throw std::runtime_error(BusInterface->GetErrorText());

    //=====================
    // Disable all interupt sources
    // Then try to connect the interrupt service routine
    //
    BE_WRITE32 (pReg, InterruptEnable, 0);
    if (OK != BusInterface->ConfigBusInterrupt((EPICS_ISR_FUNC)EgInterrupt, (void *)this))
        throw std::runtime_error(BusInterface->GetErrorText());

    //=====================
    // Initialize some member variables from the current hardware values
    //
    FPGAVersion = BE_READ32 (pReg, FPGAVersion);

    //=====================
    // Output some debug informational messages if desired
    //
    DEBUGPRINT (DP_INFO, *DebugFlag,
                ("Event Generator Card %d,  Serial Number %s,  Firmware Revision = 0x%04x\n",
                 BusInterface->GetCardNum(),
                 BusInterface->GetSerialNumber(),
                 FPGAVersion));

    DEBUGPRINT (DP_INFO, *DebugFlag,
                ("    Initialized in %s %d\n",
                 BusInterface->GetSubUnitName(),
                 BusInterface->GetSubUnit()));

    //=====================
    // Add this card to the list of known event generator cards. 
    //
    EgAddCard (CardNum, this);

}//end Configure()

void
evgMrm::RebootInit() {
    DEBUGPRINT (DP_INFO, *DebugFlag, ("RebootInit() Routine Called for EVG Card %d\n", CardNum));
}//end RebootInit()

void
evgMrm::IntEnable() {
    DEBUGPRINT (DP_INFO, *DebugFlag, ("IntEnable() Routine Called for EVG Card %d\n", CardNum));
}//end IntEnable()

void
evgMrm::Interrupt () {
    BITCLR(BE,32, pReg, InterruptEnable, EVG_IRQ_ENABLE);
}//end Interrupt()

epicsStatus
evgMrm::Report (epicsInt32 level) const {

    //=====================
    // Report bus-specific information first
    //
    BusInterface->BusHwReport();

    //=====================
    // Card and level specific information will go here.
    //

    //=====================
    // Always return success
    //
    return OK;

}//end Report()

/**************************************************************************************************
|* ~evgMrm () -- Class Destructor
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   Free up the hardware and software resources associated with this event generator card.
|*     o Delete the bus interface
|*     o Delete the card mutex
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ~evgMrm ();
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS (member variables):
|*      BusInterface  (mrfBusInterface *) Pointer to the hardware bus interface object
|*      CardLock      (epicsMutexId)      Mutex to lock access to the card.
|*
\**************************************************************************************************/

evgMrm::~evgMrm () {

    //=====================
    // Debug print
    //
    DEBUGPRINT (DP_INFO, *DebugFlag,("Destructor called for event generator card %d\n", CardNum));

    //=====================
    // Delete the bus interface object
    //
    delete BusInterface;

    //=====================
    // Delete the card lock mutex
    //
    if (0 != CardLock)
        epicsMutexDestroy (CardLock);

}//end destructor

/**************************************************************************************************/
/*                              EPICS IOC Shell Registery                                         */
/*                                                                                                */


/**************************************************************************************************/
/*   EgConfigureVME() -- Configure A VME Event Generator Card                                     */
/**************************************************************************************************/

static const iocshArg         EgConfigureVMEArg0    = {"Card",        iocshArgInt};
static const iocshArg         EgConfigureVMEArg1    = {"Slot",        iocshArgInt};
static const iocshArg         EgConfigureVMEArg2    = {"Vme Address", iocshArgInt};
static const iocshArg         EgConfigureVMEArg3    = {"Int Vector",  iocshArgInt};
static const iocshArg         EgConfigureVMEArg4    = {"Int Level",   iocshArgInt};
static const iocshArg *const  EgConfigureVMEArgs[5] = {&EgConfigureVMEArg0,
                                                       &EgConfigureVMEArg1,
                                                       &EgConfigureVMEArg2,
                                                       &EgConfigureVMEArg3,
                                                       &EgConfigureVMEArg4};
static const iocshFuncDef    EgConfigureVMEDef      = {"EgConfigureVME", 5, EgConfigureVMEArgs};

static
void EgConfigureVMECall (const iocshArgBuf *args) {
    EgConfigureVME (args[0].ival,
                    args[1].ival,
                    (epicsUInt32)args[2].ival,
                    args[3].ival,
                    args[4].ival);
}//end EgConfigureVMECall()


/**************************************************************************************************/
/*  EPICS Registrar Function for this Module                                                      */
/**************************************************************************************************/

static void evgMrmRegistrar() {
    iocshRegister (&EgConfigureVMEDef , EgConfigureVMECall );
}/*end evgMrmRegistrar()*/

extern "C" {
epicsExportRegistrar (evgMrmRegistrar);
}//end extern "C"
