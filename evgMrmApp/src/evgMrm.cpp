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
#include <epicsInterrupt.h>        // EPICS Interrupt context support routines
#include <epicsMutex.h>            // EPICS Mutex support library
#include <epicsThread.h>           // EPICS Thread support library

#include <errlog.h>                // EPICS Error logging support library
#include <iocsh.h>                 // EPICS IOC shell support library
#include <registryFunction.h>      // EPICS Registry support library

#include <mrfCommon.h>             // MRF Common definitions
#include <mrfCommonIO.h>           // MRF Common I/O macros
#include <mrfBusInterface.h>       // MRF Bus interface base class definition
#include <mrfFracSynth.h>          // MRF fractional synthesizer support routines
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

extern "C" {
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
}//end extern "C"

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
    CardMutex(new epicsMutex),                   // Create the card access mutex
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
    pReg(0),                                    // No register map address yet.

    //=====================
    // Initialize Event Link Clock data
    //
    OutLinkFrequency(0.0),                      // Event clock frequency for the outgoing link
    InLinkFrequency(0.0),                       // Event clock frequency for the incoming link
    SecsPerTick(8.0e-9),                        // Seconds per event clock tick
    FracSynthWord(0),                           // Fractional synthesizer control word
    OutLinkSource(EVG_CLOCK_SRC_INTERNAL)       // Clock source for outgoing event link

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
|*      CardMutex     = (epicsMutex *)       Mutex to lock access to this card.
|*      FPGAVersion   = (epicsUInt32)        Card's firmware revision number
|*
\**************************************************************************************************/

void
evgMrm::Configure ()
{

    //=====================
    // Try to register this card at the requested bus address space
    //
    if (0 == (pReg = BusInterface->ConfigBusAddress (EVG_REGMAP_SIZE)))
        throw std::runtime_error(BusInterface->GetErrorText());

    //=====================
    // Disable all interupt sources
    // Then try to connect the interrupt service routine
    //
    WRITE32 (pReg, InterruptEnable, 0);
    if (OK != BusInterface->ConfigBusInterrupt((EPICS_ISR_FUNC)EgInterrupt, (void *)this))
        throw std::runtime_error(BusInterface->GetErrorText());

    //=====================
    // Initialize some member variables from the current hardware values
    //
    FPGAVersion = READ32 (pReg, FPGAVersion);

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

    //=====================
    // Do all those things that we were waiting until the end of
    // the initialization phase to do.
    //
    if (!InLinkFrequency)
        InLinkFrequency = OutLinkFrequency;

    SetFracSynth();

}//end IntEnable()

void
evgMrm::Interrupt () {
    BITCLR32 (pReg, InterruptEnable, EVG_IRQ_ENABLE);
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

//**************************************************************************************************
//  SetOutLinkClockSource () -- Set the Output Event Link Clock Source
//**************************************************************************************************
//! @par Description:
//!   Sets the source for the outgoing event link event clock.  The clock can be either generated
//!   internally (using the fractional synthesizer chip) or externally (using the RF input port).
//!
//! @par Function:
//!   If configuration is still in progress, just record the clock source value.  If configuration
//!   has completed, set the requested value in the ClockSource register.<br>
//!
//! @param      ClockSource = (input)  New value for the outgoing event link clock source.
//!
//! @return     Returns OK if the clock source was successfully configured.<br>
//!             Returns ERROR if the clock source value was invalid.
//!
//! @par Member Variables Referenced:
//! - \e        OutLinkSource = (modified) The requested outgoing event link clock source
//! - \e        pReg          = (input)    Address of the EVG register map
//!
//! @note
//! - Note that if the incoming link and the outgoing link have different clock frequencies,
//!   then the outgoing link's clock source may not be set to "Internal".
//!
//**************************************************************************************************

epicsStatus
evgMrm::SetOutLinkClockSource (epicsInt16 ClockSource)
{
    //=====================
    // Local variables
    //
    epicsStatus   status = OK;  // Local status variable

    //=====================
    // Range check
    //
    if ((ClockSource < 0) || (ClockSource > EVG_CLOCK_SRC_MAX)) {
        errlogPrintf ("Invalid output link clock source (%d)\n", ClockSource);
        return ERROR;
    }//end if value out of range

    //=====================
    // Lock the card and record the desired setting
    //
    CardMutex->lock();
    OutLinkSource = ClockSource;

    //=====================
    // If initialization is finished, set the clock source
    //
    if (EgInitDone()) {

        //=====================
        // Do not allow the out link clock source to be "Internal" if the out link frequency
        // does not equal the in link frequency.
        //
        if ((EVG_CLK_SRC_EXTRF == ClockSource) && (InLinkFrequency != OutLinkFrequency)) {
            errlogPrintf ("Cannot set outgoing link clock source to \"Internal\" when\n");
            errlogPrintf ("Incoming link frequency does not equal outgoing link frequency");

            status = ERROR;
            ClockSource = EVG_CLOCK_SRC_RF;
        }//end if trying to set internal clock source when input freq. not equal output freq.

        //=====================
        // Set the outgoing event link clock source
        //
        switch (ClockSource) {

        case EVG_CLOCK_SRC_INTERNAL:  // Use internal fractional synthesizer to generate the clock
            BITCLR8 (pReg, ClockSource, EVG_CLK_SRC_EXTRF);
            break;

        case EVG_CLOCK_SRC_RF:        // Use external RF source to generate the clock
            BITSET8 (pReg, ClockSource, EVG_CLK_SRC_EXTRF);
            break;

        }//end switch
    }//end if we are not still in configuration mode

    //=====================
    // Release the card mutex and return
    //
    CardMutex->unlock();
    return status;

}//end SetOutLinkClockSource()

//**************************************************************************************************
//  SetOutLinkClockSpeed () -- Set the Output Event Link Clock Speed
//**************************************************************************************************
//! @par Description:
//!   Sets the event clock speed for the outgoing event link.
//!
//! @par Function:
//!   If configuration is still in progress, just record the clock source value.  If configuration
//!   has completed, set the requested value in the ClockSpeed register.
//!   
//!
//! @param      ClockSpeed = (input)  New value (in megahertz) for the outgoing event link
//!                                   clock speed
//!
//! @return     Returns OK if the clock speed was successfully set.<br>
//!             Returns ERROR if the clock speed value was out of range.
//!
//! @par Member Variables Referenced:
//! - \e        CardMutex        = (input)    Mutex guarding access to this card
//! - \e        InLinkFrequency  = (input)    The requested incoming event link clock frequency
//! - \e        OutLinkFrequency = (modified) The requested outgoing event link clock frequency
//! - \e        SecsPerTick      = (modified) Seconds per tick.  Scale factor for ai/ao records
//! - \e        FracSynthWord    = (modified) Fractional synthesizer control word
//!
//**************************************************************************************************

epicsStatus
evgMrm::SetOutLinkClockSpeed (epicsFloat64 ClockSpeed)
{
    //=====================
    // Local variables
    //
    epicsUInt32    ControlWord; // Fractional synthesizer control word for requested frequency
    epicsFloat64   Error;       // Error (in ppm) between requested frequency & control word

    //=====================
    // See if the requested clock frequency can be accommodated by the hardware
    //
    ControlWord = FracSynthControlWord (ClockSpeed, MRF_FRAC_SYNTH_REF, *DebugFlag, &Error);
    if ((!ControlWord) || (Error > 100.0)) {
        errlogPrintf ("Cannot set event clock speed to %f MHz.\n", ClockSpeed);
        return ERROR;
    }//end if value out of range

    //=====================
    // Lock access to the card and record the desired setting
    //
    CardMutex->lock();
    OutLinkFrequency = ClockSpeed;
    SecsPerTick = 1e-6 / OutLinkFrequency;

    //=====================
    // If we haven't set a frequency for the incoming event link,
    // use the outgoing link frequency to set the fractional synthesizer.
    //
    if (!InLinkFrequency)
        FracSynthWord = ControlWord;

    //=====================
    // If initialization is finished, and no incoming link frequency has been set,
    // use the outgoing event link clock speed to set the fractional synthesizer.
    //
    if (EgInitDone() && !InLinkFrequency)
        SetFracSynth();

    //=====================
    // Unlock the card and return
    //
    CardMutex->unlock();
    return OK;

}//end SetOutLinkClockSpeed()

//**************************************************************************************************
//  SetInLinkClockSpeed () -- Set the Input Event Link Clock Speed
//**************************************************************************************************
//! @par Description:
//!   Sets the event clock speed for the incoming event link.
//!
//! @par Function:
//!   If configuration is still in progress, just record the clock source value.  If configuration
//!   has completed, set the requested value in the ClockSpeed register.
//!   
//!
//! @param      ClockSpeed = (input)  New value (in megahertz) for the outgoing event link
//!                                   clock speed
//!
//! @return     Returns OK if the clock speed was successfully set.<br>
//!             Returns ERROR if the clock speed value was out of range.
//!
//! @par Member Variables Referenced:
//! - \e        CardMutex        = (input)    Mutex guarding access to this card
//! - \e        InLinkFrequency  = (modified) The requested outgoing event link clock frequency
//! - \e        FracSynthWord    = (modified) Fractional synthesizer control word
//!
//**************************************************************************************************

epicsStatus
evgMrm::SetInLinkClockSpeed (epicsFloat64 ClockSpeed)
{
    //=====================
    // Local variables
    //
    epicsUInt32    ControlWord; // Fractional synthesizer control word for requested frequency
    epicsFloat64   Error;       // Error (in ppm) between requested frequency & control word

    //=====================
    // See if the requested clock frequency can be accommodated by the hardware
    //
    ControlWord = FracSynthControlWord (ClockSpeed, MRF_FRAC_SYNTH_REF, *DebugFlag, &Error);
    if ((!ControlWord) || (Error > 100.0)) {
        errlogPrintf ("Cannot set event clock speed to %f MHz.\n", ClockSpeed);
        return ERROR;
    }//end if value out of range

    //=====================
    // Lock access to the card and record the desired setting
    //
    CardMutex->lock();
    InLinkFrequency = ClockSpeed;
    FracSynthWord = ControlWord;

    //=====================
    // If initialization is finished, set the incoming event link frequency
    // to set the fractional synthesizer control word.
    //
    if (EgInitDone()) SetFracSynth();

    //=====================
    // Unlock the card and return
    //
    CardMutex->unlock();
    return OK;

}//end SetInLinkClockSpeed()

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
|*      CardMutex     (epicsMutex *)      Mutex to lock access to the card.
|*
\**************************************************************************************************/

evgMrm::~evgMrm () {

    //=====================
    // Debug print
    //
    DEBUGPRINT (DP_INFO, *DebugFlag,("Destructor called for event generator card %d\n", CardNum));

    //=====================
    // Delete the bus interface object and the card mutex object
    //
    delete BusInterface;
    delete CardMutex;

}//end destructor

/**************************************************************************************************
|* SetFracSynth () -- Load a new Control Word Into the Fractional Synthesizer Chip
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   Sets the fractional synthesizer control word.  The fractional synthesizer has two functions:
|*     1) It sets the expected frequency for the incoming event link.
|*     2) If the outgoing event link's clock source is set to "Internal", it also sets the
|*        frequency of the outgoing event link clock.
|*
|*   If the new control word is the same as the control word already loaded, exit without doing
|*   anything (in order to avoid a resync-bump).  Once the new control word has been loaded,
|*   wait for it to take effect and then clear the resulting "Receiver Violation" error.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      SetFracSynth ();
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS (member variables):
|*      CardMutex     = (epicsMutex *) Mutex guarding access to this card
|*      FracSynthWord = (epicsUInt32)  Desired fractional synthesizer control word
|*      pReg          = (epicsUInt32)  Address of the EVG register map
|*
|*
\**************************************************************************************************/

void
evgMrm::SetFracSynth ()
{
    epicsUInt32     EnableMask;   // Currently enabled EVG interrupts
    epicsUInt32     Key;          // Used to restore interrupt level after locking

    //=====================
    // If the desired fractional synthesizer control word was never set,
    // use whatever the current hardware is set to.
    //
    if (!FracSynthWord) {
        FracSynthWord = READ32 (pReg, FracSynthWord);
        return;
    }//end if nobody set the control word.

    //=====================
    // If the requested control word is the same as the current fractional synthesizer
    // control word, exit without writing the control word so that we don't cause a "bump"
    // in the gate outputs.
    //
    if (READ32(pReg, FracSynthWord) == FracSynthWord)
        return;

    //=====================
    // Disable incomming event link receiver violation interrupts while we set
    // the fractional synthesizer control word.
    //
    CardMutex->lock();
    Key = epicsInterruptLock();
    EnableMask = READ32 (pReg, InterruptEnable);
    WRITE32 (pReg, InterruptEnable, (EnableMask & ~(EVG_IRQ_RXVIO)));
    epicsInterruptUnlock (Key);

    //=====================
    // Set the new fractional synthesizer control word and wait for it to synch up.
    //
    WRITE32 (pReg, FracSynthWord, FracSynthWord);
    epicsThreadSleep (0.5);

    //=====================
    // Reset the "Receiver Violation" error that this probably caused.
    //
    Key = epicsInterruptLock();
    BITCLR32 (pReg, InterruptFlag, EVG_IRQ_RXVIO);

    //=====================
    // Reset the interrupt enable mask.
    //
    if (EnableMask & EVG_IRQ_RXVIO)
        WRITE32 (pReg, InterruptEnable, EnableMask);

    //=====================
    // Release the card lock, and return
    //
    epicsInterruptUnlock (Key);
    CardMutex->unlock();

}//end SetFracSynth()

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
