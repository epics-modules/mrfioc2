
#ifndef EVG_HPP_INC
#define EVG_HPP_INC

#include <epicsTypes.h>

class EVG
{

/**************************************************************************************************/
/*  Public Methods                                                                                */
/**************************************************************************************************/

public:

    //=====================
    // Getter Routines
    //
    virtual epicsInt32    GetBusType     () const = 0; // Return card's bus type (VME, PCI, etc.)
    virtual epicsInt32    GetCardNum     () const = 0; // Return the logical card number
    virtual epicsInt32    GetSubUnit     () const = 0; // Return the sub-unit value
    virtual const char   *GetSubUnitName () const = 0; // Return the sub-unit name
    virtual epicsFloat64  GetSecsPerTick () const = 0; // Return number of seconds per tick

    //=====================
    // Setter Routines
    //
    virtual void         SetDebugLevel (epicsInt32 level) = 0;

    //=====================
    // Event Link Clock Setters
    //
    virtual epicsStatus SetOutLinkClockSource (epicsInt16 ClockSource)  = 0;
    virtual epicsStatus SetOutLinkClockSpeed  (epicsFloat64 ClockSpeed) = 0;
    virtual epicsStatus SetInLinkClockSpeed   (epicsFloat64 ClockSpeed) = 0;

    //=====================
    // Card Configuration and Initialization Routines
    //
    virtual void         Configure      () = 0;       // Initial card configuration
    virtual void         RebootInit     () = 0;       // Reboot initialization (before dev sup init
    virtual void         IntEnable      () = 0;       // Enable card interrupts

    //=====================
    // Event Generator Card Interrupt Routine
    //
    virtual void         Interrupt      () = 0;

    //=====================
    // Event Generator Card Report Routine
    //
    virtual epicsStatus  Report    (epicsInt32 level) const = 0;

    //=====================
    // Sequence RAM Control Routines
    //
    virtual epicsStatus  SetSeqEvent (epicsUInt32 RamNum, 
                                      epicsUInt32 event,
                                      epicsUInt32 timestamp) = 0;

    //=====================
    // Class Destructor
    //
    virtual ~EVG () = 0;

};// end class EVG //

#endif // EVG_HPP_INC //
