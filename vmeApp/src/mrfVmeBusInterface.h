/**************************************************************************************************
|* $(TIMING)/vmeApp/src/mrfVmeBusInterface.h -- VME Bus Interface Class Definition
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
|*   This header file contains the member function and data definitions for the VME bus interface
|*   class, which implements the abstract mrfBusInterface class.
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

#ifndef VME_BUS_INT_INC
#define VME_BUS_INT_INC


/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include <mrfCommon.h>          // MRF Common definitions
#include <mrfBusInterface.h>    // MRF Bus interface base class

/**************************************************************************************************/
/*                            mrfVmeBusInterface Class Definition                                 */
/*                                                                                                */

class mrfVmeBusInterface: public mrfBusInterface
{

/**************************************************************************************************/
/*  Public Methods                                                                                */
/**************************************************************************************************/

public:

    //=====================
    // Constructor
    //
    mrfVmeBusInterface (
        epicsInt32   Card,               // Logical Card Number
        epicsInt32   CardType,           // Type of card (EVG, EVR, etc.)
        epicsInt32   VmeSlot,            // VME Slot Number
        epicsUInt32  VmeAddress,         // Requested VME Bus Address for Card's Register Map
        epicsInt32   IrqVector = 0,      // Interrupt vector
        epicsInt32   IrqLevel  = 0       // Interrupt request level
    );//end constructor

    //=====================
    // Return card's bus type (VME, PMC, PCI, etc.)
    //
    inline epicsInt32 GetBusType () const {
        return (BusType);
    }//end GetBusType()

    //=====================
    // Return the logical card number
    //
    inline epicsInt32 GetCardNum () const {
        return (CardNum);
    }//end GetCardNum()

    //=====================
    // Return card type (EVG, EVR, etc.)
    //
    inline epicsInt32 GetCardType () const {
        return (CardType);
    }//end GetCardType()

    //=====================
    // Return sub-unit value
    //
    inline epicsInt32 GetSubUnit () const {
        return (Slot);
    }//end GetSubUnit()

    //=====================
    // Return the sub-unit name (Slot, Index, etc.)
    //
    inline const char *GetSubUnitName () const {
        return ("VME Slot");
    }//end GetSubUnitName()

    //=====================
    // Return pointer to card description text
    //
    inline const char *GetDescription () const {
        return (Description);
    }//end GetDescription()

    //=====================
    // Return pointer to text for most recent error
    //
    inline const char *GetErrorText () const {
        return (ErrorText);
    }//end GetErrorText()

    //=====================
    // Return pointer to card's serial number
    //
    inline const char *GetSerialNumber () const {
        return (SerialNumber);
    }//end GetSerialNumber()

    //=====================
    // Bus address configuration routine
    //
    virtual volatile epicsUInt8* ConfigBusAddress (epicsInt32 RegMapSize);

    //=====================
    // Bus interrupt configuration routine
    //
    virtual epicsStatus ConfigBusInterrupt (EPICS_ISR_FUNC IntHandler, void *IntParm);

    //=====================
    // Bus interrupt enable routine
    //
    virtual void BusInterruptEnable () const;

    //=====================
    // Bus-specific hardware report routine
    //
    virtual void BusHwReport () const;

    //=====================
    // Class Destructor
    //
    ~mrfVmeBusInterface ();

/**************************************************************************************************/
/*  Private Data                                                                                  */
/**************************************************************************************************/

private:

    bool             AddressRegistered;     // True if bus address is registered with devLib
    bool             IntVecConnected;       // True if interrupt vector and level have been set
    bool             IntRtnConnected;       // True if interrupt service routine is connected
    EPICS_ISR_FUNC   ISR;                   // Address of interrupt service routine

    epicsUInt32      BusAddress;            // Card's bus address
    epicsInt32       BusType;               // Card's bus type (VME,PMC,PCI...)

    epicsInt32       CardNum;               // Logical card number
    epicsInt32       CardType;              // Card type (EVR, EVG,...)

    volatile epicsUInt8* CpuAddress;        // Card's CPU address
    volatile epicsUInt8* CpuCSRAddress;     // Mapped Slot CSR base address

    epicsInt32       IrqVector;             // Card's interrupt vector
    epicsInt32       IrqLevel;              // Card's interrupt request level

    epicsInt32       Slot;                  // Card's VME slot number
    
    char             Description  [MRF_DESCRIPTION_SIZE];  // Card description (mostly for devLib)
    char             ErrorText    [128];                   // Text from latest error
    char             SerialNumber [MRF_SN_STRING_SIZE];    // Card's serial number
    
};// end class mrfVmeBusInterface //


#endif // VME_BUS_INT_INC //
