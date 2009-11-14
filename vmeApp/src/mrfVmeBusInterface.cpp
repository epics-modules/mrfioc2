/**************************************************************************************************
|* $(TIMING)/vmeApp/src/mrfVmeBusInterface.cpp -- VME Bus Interface Class
|*
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     3 November 2009
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 03 Nov 2009  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This module contains the VME bus implementations for the following
|*   mrfBusInterface class methods:
|*
|*     mrfVmeBusInterface    Class constructor
|*
|*     ConfigBusAddress      Called before iocInit() during card configuration
|*                           Configures the bus address and maps it to a cpu address.
|*
|*     ConfigBusInterrupt    Called before iocInit() during card configuration
|*                           Sets the interrupt vector and request levels and connects
|*                           the interrupt handler routine to the interrupt vector.
|*
|*     BusInterruptEnable    Called during the "AfterInterruptAccept" of iocInit()
|*                           Enables VME interrupts at the requested IRQ level. 
|*
|*     ~mrfVmeBusInterface   Class destructor
|*
|*--------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator and Event Receiver Cards
|*     APS Register Mask
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

#include <epicsStdlib.h>        // EPICS Standard C library support routines
#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include <epicsInterrupt.h>     // EPICS Interrupt context support routines

#include <devLib.h>             // EPICS Device-independant hardware addressing support library
#include <osdVME.h>             // EPICS VME address mode definitions

#include <mrfVme64x.h>          // VME-64X CR/CSR routines and definitions (with MRF extensions)
#include <mrfVmeBusInterface.h> // Class definition file


/**************************************************************************************************/
/*  Local Structure Definitions                                                                   */
/**************************************************************************************************/

//=====================
// Supported Card and Card Type Structure
//
struct SupportedCardStruct {
    epicsUInt32  CardID;        // Card ID of the supported card
    epicsInt32   CardType;      // Card type (EVG, EVR, etc. of the supported card
};//end SupportedCardStruct


/**************************************************************************************************/
/*  Lookup Tables                                                                                 */
/**************************************************************************************************/

//=====================
// Table of supported card types
//
static
SupportedCardStruct SupportedCards [] = {
    {MRF_VME_EVG_BID,    MRF_CARD_TYPE_EVG},    // VME Event Generator Card
    {MRF_VME_EVR_BID,    MRF_CARD_TYPE_EVR},    // VME Event Receiver Card
    {MRF_VME_EVR_RF_BID, MRF_CARD_TYPE_EVR}     // VME Event Receiver Card With RF Recovery
};

const epicsInt32  NUM_SUPPORTED_CARDS (sizeof(SupportedCards) / sizeof(SupportedCardStruct));


//=====================
// Table of supported card type names
//
// Note: The table is ordered based on the values of the MRF_CARD_TYPE_xxx macros
//       which are, in turn, based on the card type values read from the
//       FPGA Firmware Version Register
//
static
char*  CardTypeName [] = {
    "Unsupported Card Type",                    // 0 = Not used 
    "Event Receiver",                           // 1 = Event Receiver
    "Event Generator"                           // 2 = Event Generator
};

const epicsInt32  NUM_CARD_TYPE_NAMES (sizeof(CardTypeName) / sizeof(char*));

/**************************************************************************************************/
/*                              mrfVmeBusInterface Methods                                        */
/*                                                                                                */


/**************************************************************************************************
|* mrfVmeBusInterface () -- Class Constructor
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   Constructs a VME bus interface object and initializes its data members
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      mrfVmeBusInterface (Card, CardType, VmeSlot, VmeAddress, IrqVector, IrqLevel);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUTS:
|*      Card        = (epicsInt32)  Logical card number
|*      CardType    = (epicsInt32)  Card type (EVR, EVG, etc.)
|*      VmeSlot     = (epicsInt32)  Card's VME slot number
|*      VmeAddress  = (epicsUInt32) VME bus address
|*      IrqVector   = (epicsInt32)  Interrupt vector number
|*      IrqLevel    = (epicsInt32)  Interrupt request level
|*
\**************************************************************************************************/

mrfVmeBusInterface::mrfVmeBusInterface (
    epicsInt32   Card,                  // Logical Card Number
    epicsInt32   CardType,              // Type of card (EVG, EVR, etc.)
    epicsInt32   VmeSlot,               // VME Slot Number
    epicsUInt32  VmeAddress,            // Requested VME Address for Card's Register Map
    epicsInt32   IrqVector,             // Interrupt vector
    epicsInt32   IrqLevel) :            // Interrupt request level

    //=====================
    // Initialize the member variables
    //
    AddressRegistered (false),          // VME Address not yet registered
    IntVecConnected   (false),          // Interrupt vector/level not yet set
    IntRtnConnected   (false),          // Interrupt handler routine not yet connected
    ISR               (NULL),           // No interrupt handler routine specified yet
    BusAddress        (VmeAddress),     // Initialize bus address from the input parameter
    BusType           (MRF_BUS_VME),    // Set the bus type to VME
    CardNum           (Card),           // Initialize the logical card number from the input parm.
    CardType          (CardType),       // Initialize the card type from the input parameter
    CpuAddress        (0),              // CPU Address not yet mapped
    IrqVector         (IrqVector),      // Initialize the interrupt vector from the input parameter
    IrqLevel          (IrqLevel),       // Initialize the interrupt request level from the input
    Slot              (VmeSlot)         // Initialize the slot number from the input parameter
{
    //=====================
    // Set the string member variables to "null"
    //
    Description[0]  = '\0';
    ErrorText[0]    = '\0';
    SerialNumber[0] = '\0';

};//end mrfVmeBusInterface Constructor

/**************************************************************************************************
|* ConfigBusAddress () -- Perform VME Bus-Specific Register Map Address Configuration
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   Registers the card's register map in VME A24 space and sets the appropriate function
|*   registers in the card's CR/CSR space so that it will respond to the requested bus addresses.
|*
|*   This routine performs the following actions:
|*   o Check to make sure the slot number is within the accepted range.
|*   o Read the card's board ID from CR/CSR space.  Use the board ID to verify that the card
|*     in the specified slot is the type of card we are attempting to initialize.
|*   o Register the requested VME address with the EPICS device support library.
|*   o Enable the card's function 0 or function 1 registers (in CR/CSR space) to map the
|*     card's register space to the requested bus address.
|*   o Test to make sure we can actually read the card's register space at the bus address we set.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      cpuAddress = ConfigBusAddress (RegMapSize);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      RegMapSize         = (epicsInt32)  Size of the card's register map (in bytes)
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS (member variables):
|*      BusAddress         = (epicsUInt32) Requested VME bus address
|*      CardNum            = (epicsInt32)  Logical card number
|*      CardType           = (epicsInt32)  Card type (EVR, EVG, etc.)
|*      Slot               = (epicsInt32)  Card's VME slot number
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT OUTPUTS (member variables):
|*      AddressRegistered  = (bool)        True if bus address was registered with devLib
|*      CpuAddress         = (epicsUInt32) CPU address for accessing the register map
|*      Description        = (char *)      Card description (used to register with devLib)
|*      ErrorText          = (char *)      Text from last error condition
|*      Serial Number      = (char *)      Card's serial number (read from CSR space)
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      cpuAddress         = (epicsUInt32) Returns 0 if the bus address configuration failed
|*                                         Returns the card's CPU address if the routine
|*                                         completed successfully.
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|*   This routine is called during card configuration prior to running iocInit().
|*   It is called early on in the configuration process so that we know that the
|*   requested hardware actually exists and can talk to us.
|*
\**************************************************************************************************/

epicsUInt32
mrfVmeBusInterface::ConfigBusAddress (epicsInt32 RegMapSize)
{
    //=====================
    // Local Variables
    //
    epicsInt32      ActualType;            // Card's type as read from the hardware
    epicsUInt32     BoardID;               // Card's board ID
    epicsUInt32     BoardType;             // Card's board type (extracted from the board ID)
    epicsUInt32     BoardSeries;           // Card's board series (extracted from the board ID)
    epicsUInt16     Junk;                  // Dummy variable for card read probe function
    epicsUInt32     status;                // Status return variable
    char            statusText [64];       // Status text string

    //=====================
    // Make sure the slot number is valid
    //
    if ((Slot < 1) || (Slot > MRF_MAX_VME_SLOT)) {
        sprintf (ErrorText, "Slot number %d is invalid, must be between 1 and %d",
                 Slot, MRF_MAX_VME_SLOT);
        return 0;
    }//end if slot number is invalid

    //=====================
    // Try to get the board ID of the card in this slot
    //
    status = vmeCRGetBID (Slot, &BoardID);
    if (OK != status) {
        sprintf (ErrorText, "Unable to access CR/CSR Space for slot %d.", Slot);
        return 0;
    }//end could not read the card's board ID

    //=====================
    // Look up the actual card type in the table of supported cards
    //
    ActualType  = 0;
    BoardType   = BoardID & MRF_BID_TYPE_MASK;
    BoardSeries = BoardID & MRF_BID_SERIES_MASK;

    for (epicsInt32 i = 0;  i < NUM_SUPPORTED_CARDS;  i++) {
        if (BoardType == SupportedCards[i].CardID) {
            ActualType = SupportedCards[i].CardType;
            break;
        };//end if we found the board ID in the list of supported cards
    }//end for each supported card type

    //=====================
    // Check to see if the card at this slot is the kind of card we expected
    //
    if (ActualType != CardType) {
        sprintf (ErrorText, "Card found in slot %d. is not an %s", Slot, CardTypeName[CardType]);
        return 0;
    }//end if the actual card type does not match the expected card type

    //=====================
    // Construct the card description that we will use when we try to register
    // the card with devLib.
    //
    sprintf (Description, "MRF Series %d %s Card", BoardSeries, CardTypeName[CardType]);

    //=====================
    // Try to register this card at the requested A24 address space
    //
    status = devRegisterAddress (
                 Description,                            // Event Generator Card name
                 atVMEA24,                               // A24 Address space
                 BusAddress,                             // Physical address of register space
                 RegMapSize,                             // Size of card's register space
                 (volatile void **)(void*)&CpuAddress);  // Local address of card's register map

    if (OK != status) {
        errSymLookup (status, statusText, 64);
        sprintf (ErrorText,
                 "Unable to register %s Card %d at VME/A24 address 0x%08X\n  %s",
                 CardTypeName[CardType], CardNum, BusAddress, statusText);
        return 0;
    }//end if devRegisterAddress() failed

    //=====================
    // Try to set the card's Function 0 or Function 1 address to the base address we just registered
    //
    status = mrfSetAddress (Slot, BusAddress, VME_AM_STD_SUP_DATA);
    if (OK != status) {
        sprintf (ErrorText, "Unable to set bus address for slot %d.", Slot);
        return 0;
    }//end could not set VME address in CR/CSR space

    //=====================
    // Test to see if we can actually read the card at the address we set it to.
    //
    AddressRegistered = true;
    status = devReadProbe (sizeof(epicsUInt16), (const volatile void*)CpuAddress, &Junk);
    if (OK != status) {
        sprintf (ErrorText,
                 "Unable to read Event Generator Card %d (slot %d) at VME/A24 address 0x%08X",
                 CardNum, Slot, BusAddress);
    }//end if could not read the Event Generator card

    //=====================
    // Abort if we failed to set the card's VME address, or if the card does not respond.
    //
    if (OK != status) {
        devUnregisterAddress (atVMEA24, BusAddress, Description);
        AddressRegistered = false;
        return 0;
    }//end if could not set VME address

    //=====================
    // Read the card's serial number
    //
    mrfGetSerialNumberVME (Slot, SerialNumber);

    //=====================
    // Return the CPU address for accessing the card's register map
    //
    return CpuAddress;

}//end ConfigBusAddress()

/**************************************************************************************************
|* ConfigBusInterrupt () -- Perform VME Bus-Specific Interrupt Service Configuration
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   Sets the cards interrupt vector and interrupt request level in CR/CSR space and connects
|*   the interrupt service routine to the interrupt vector.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = ConfigBusInterrupt (IntHandler, IntParm);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      IntHandler       = (EPICS_ISR_FUNC) Address of the interrupt handler routine.
|*      IntParm          = (void *)         Parameter to pass to the interrupt handler routine
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS (member variables):
|*      CardNum          = (epicsInt32)     Logical card number
|*
|*      CardType         = (epicsInt32)     Card type (EVR, EVG, etc.)
|*      IrqLevel         = (epicsInt32)     Interrupt request level
|*                                            (note that this could be an implicit output for other
|*                                             bus architectures)
|*
|*      IrqVector        = (epicsInt32)     Interrupt vector number
|*                                            (note that this could be an implicit output for other
|*                                             bus architectures)
|*
|*      Slot             = (epicsInt32)     Card's VME slot number
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT OUTPUTS (member variables):
|*      ErrorText        = (char *)         Text from last error condition
|*      IntRtnConnected  = (bool)           True if bus address was registered with devLib
|*      IntVecConnected  = (bool)           True if the interrupt vector is set in CR/CSR space
|*      ISR              = (EPICS_ISR_FUNC) Address of interrupt handler routine
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status           = (epicsUInt32)    ERROR: Indicates interrupt service routine was not
|*                                                 connected.
|*                                          OK:    Indicates the interrupt vector and level were
|*                                                 set and the interrupt hander was connected
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|*   This routine is called during card configuration prior to running iocInit().
|*   It should not be called until the configuration process has disabled all interrupt
|*   sources from the hardware.
|*
\**************************************************************************************************/

epicsStatus
mrfVmeBusInterface::ConfigBusInterrupt (
    EPICS_ISR_FUNC   IntHandler,        // Interrupt handler routine
    void            *IntParm)           // Parameter to pass to interrupt handler routine
{
    //=====================
    // Local Variables
    //
    epicsStatus   status;               // Local status variable
    char          statusText [64];      // Status text string

    if (devInterruptInUseVME(IrqVector)) {
        sprintf (ErrorText, "Requested interrupt vector (0x%04X) for card %d is already in use",
                 IrqVector, CardNum);
        return ERROR;
    }//end if interrupt vector is already in use

    //=====================
    //Set the card's IRQ level and vector in CR/CSR space.
    //
    status = mrfSetIrq (Slot, IrqVector, IrqLevel);
    if (OK != status) {
        sprintf (ErrorText, "Unable to set IRQ vector and level for %s Card %d in slot %d",
                 CardTypeName[CardType], CardNum, Slot);
        return ERROR;
    }//end if we could not set the IRQ vector/level

    //=====================
    // Connect the interrupt handler
    //
    IntVecConnected = true;
    status = devConnectInterruptVME (IrqVector, IntHandler, IntParm);

    //=====================
    // If we could not connect to the interrupt vector, disable the card's interrupts
    //
    if (OK != status) {
        errSymLookup (status, statusText, 64);
        sprintf (ErrorText, "Unable to connect %s Card %d to interrupt vector 0x%04X\n  %s",
                 CardTypeName[CardType], CardNum, IrqVector, statusText);

        mrfSetIrq (Slot, 0, 0);
        IntVecConnected = false;
        return ERROR;
    }//end if failed to connect to interrupt vector

    //=====================
    // Return success if we were able to connect the card's interrupt routine
    //
    IntRtnConnected = true;
    ISR = IntHandler;

    return OK;

}//end ConfigBusInterrupt()

/**************************************************************************************************
|* BusInterruptEnable () -- Enable the VME Interrupt Level
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   Enable VME interrupts for the requested IRQ level.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      BusInterruptEnable ();
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS (member variables):
|*      IrqLevel       = (epicsInt32)     Interrupt request level
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|*   This routine is called during the "AfterInterruptAccept" phase of iocInit(),
|*   after it is safe to accept interrupts.
|*
\**************************************************************************************************/

void
mrfVmeBusInterface::BusInterruptEnable () const {
    devEnableInterruptLevelVME (IrqLevel);
}//end BusInterruptEnable()

/**************************************************************************************************
|* BusHwReport () -- Display VME Bus-Specific Hardware Information About This Card
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   Display hardware information formatted for VME cards.  Information displayed includes:
|*     o Logical card number
|*     o VME slot number
|*     o Card serial number (MAC address for VME cards)
|*     o VME bus address in A24 space of the card's register map
|*     o CPU address of the card's register map
|*     o Interrupt vector
|*     o Interrupt service request level.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      BusHwReport ();
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS (member variables):
|*      BusAddress     = (epicsUInt32)    VME bus address
|*      CardNum        = (epicsInt32)     Logical card number
|*      CpuAddress     = (epicsUInt32)    CPU address for accessing the register map
|*      IrqLevel       = (epicsInt32)     Interrupt request level
|*      IrqVector      = (epicsInt32)     Interrupt vector number
|*      Serial Number  = (char *)         Card's serial number (read from CSR space)
|*      Slot           = (epicsInt32)     Card's VME slot number
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|*   This routine is called from the driver report routine.
|*
\**************************************************************************************************/

void
mrfVmeBusInterface::BusHwReport () const {
    printf ("  Card %d in VME slot %d.  Serial Number = %s.\n", CardNum, Slot, SerialNumber);
    printf ("       VME Address = %8.8X.  Local Address = %8.8X.", BusAddress, CpuAddress);
    printf ("   Vector = %3.3X.  Level = %d.\n", IrqVector, IrqLevel);
}//end BusHwReport()

/**************************************************************************************************
|* ~mrfVmeBusInterface () -- Class Destructor
|*
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*   Disconnect the card's VME bus interface and free up its resources.  This involves:
|*     o Unregister the card's VME address from devLib
|*     o Disable VME interrupts from the card
|*     o Disconnect the card's interrupt service routine
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      ~mrfVmeBusInterface ();
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUTS (member variables):
|*      AddressRegistered = (bool)           True if VME address was registered with devLib
|*      BusAddress        = (epicsUInt32)    Card's VME bus address
|*      Description       = (char *)         Card description (used to register with devLib)
|*      IntRtnConnected   = (bool)           True if the interrupt handler routine was connected
|*      IntVecConnected   = (bool)           True if the interrupt vector is set in CR/CSR space
|*      ISR               = (EPICS_ISR_FUNC) Address of interrupt handler routine
|*      Slot              = (epicsInt32)     Card's VME slot number
|*
|*-------------------------------------------------------------------------------------------------
|* NOTES:
|*   The bus interface destructor must run before the card destructor is executed.
|*
\**************************************************************************************************/

mrfVmeBusInterface::~mrfVmeBusInterface()
{
    //=====================
    // Un-register the bus address with devLib
    //
    if (AddressRegistered)
        devUnregisterAddress (atVMEA24, BusAddress, Description);

    //=====================
    // Disable the card's interrupt request level
    //
    if (IntVecConnected)
        mrfSetIrq (Slot, 0, 0);

    //=====================
    // Disconnect the interrupt service routine
    //
    if (IntRtnConnected)
        devDisconnectInterruptVME (IrqVector, ISR);

}//end destructor
