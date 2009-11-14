/***************************************************************************************************
|* mrfBusInterface.h -- Class Declaration for the Bus Interface Class
|*
|*--------------------------------------------------------------------------------------------------
|* Authors:  E.Bjorklund (LANSCE)
|* Date:     23 October 2009
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 23 Oct 2009  E.Bjorklund     Original.
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This header file contains the class declarations for the mrfBusInterface class.
|*   This is a pure virtual class that defines the bus-specific interface routines for
|*   the MRF Event Generator and Event Receiver cards.
|*
|*--------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator and Event Receiver Cards
|*     APS Register Mask
|*     Modular Register Mask
|*
|*--------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   Any
|*
\**************************************************************************************************/

/***************************************************************************************************
|*                           COPYRIGHT NOTIFICATION
|***************************************************************************************************
|*
|* THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
|* AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
|* AND IN ALL SOURCE LISTINGS OF THE CODE.
|*
|***************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included with this distribution.
|*
\**************************************************************************************************/

#ifndef MRF_BUS_INT_INC
#define MRF_BUS_INT_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include <mrfCommon.h>          // MRF Common Definitions

/**************************************************************************************************/
/*                             mrfBusInterface Class Definition                                   */
/*                                                                                                */

class mrfBusInterface {

/**************************************************************************************************/
/*  Public Methods                                                                                */
/**************************************************************************************************/

public:

    //=====================
    // Getter functions
    //
    virtual epicsInt32   GetBusType      () const = 0; // Return the card's bus type (VME, PCI, etc)
    virtual epicsInt32   GetCardNum      () const = 0; // Return the logical card number
    virtual epicsInt32   GetCardType     () const = 0; // Return the card type (EVG, EVR, etc.)
    virtual epicsInt32   GetSubUnit      () const = 0; // Return the sub-unit value
    virtual const char  *GetSubUnitName  () const = 0; // Return sub-unit name (Slot, Index, etc.)
    virtual const char  *GetDescription  () const = 0; // Return pointer to card description text
    virtual const char  *GetErrorText    () const = 0; // Return pointer to text from last error
    virtual const char  *GetSerialNumber () const = 0; // Return pointer to card's serial number

    //=====================
    // Bus address configuration routine
    //   Called early in the initialization phase to determine if the
    //   hardware actually exists and will talk to us.
    //
    virtual epicsUInt32
    ConfigBusAddress (epicsInt32 RegMapSize) = 0;

    //=====================
    // Bus interrupt configuration routine
    //   Called during initialization after the card's interrupt sources
    //   have been disabled.  Connects the interrupt handler routine to
    //   the interrupt vector.
    //
    virtual epicsStatus
    ConfigBusInterrupt (EPICS_ISR_FUNC IntHandler, void *IntParm) = 0;

    //=====================
    // Bus interrupt enable routine
    //   Called after the IOC initialization has reached a state where it is
    //   safe to accept interrupts (typically during the "AfterInterruptAccept"
    //   stage of iocInit().
    //
    virtual void
    BusInterruptEnable () const = 0;

    //=====================
    // Bus-specific hardware report routine
    //
    virtual void
    BusHwReport () const = 0;

    //=====================
    // Class Destructor
    //
    virtual ~mrfBusInterface () = 0;

};// end class mrfBusInterface //

#endif // MRF_BUS_INT_INC //
