#ifndef EVG_SEQ_EVENT_INC
#define EVG_SEQ_EVENT_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <string>               // Standard C++ string class

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include <dbCommon.h>           // EPICS Common record structure definitions

#include <Sequence.h>           // Sequence class definition

/**************************************************************************************************/
/*  Forward Declarations                                                                          */
/**************************************************************************************************/

class Sequence;                // Sequence class definition

/**************************************************************************************************/
/*                               SequenceEvent Class Definition                                   */
/*                                                                                                */

class SequenceEvent
{

/**************************************************************************************************/
/*  Public Methods                                                                                */
/**************************************************************************************************/

public:

    //=====================
    // Class Constructor
    //
    SequenceEvent (const std::string&  Name, Sequence *pSequence);

    //=====================
    // Getter Functions
    //
    inline const std::string& GetName() const {
        return (EventName);
    }//end GetName()

    //=====================
    // Register the timestamp record
    //
    void RegisterTimeRecord (dbCommon *pRec);

/**************************************************************************************************/
/*  Private Data                                                                                  */
/**************************************************************************************************/

private:

    //=====================
    // Event name
    //
    const
    std::string    EventName;

    //=====================
    // Pointer to Sequence
    //
    Sequence      *pSequence;

    //=====================
    // Event code variables
    //
    epicsUInt8     EventCode;           // Event code
    dbCommon      *EventCodeRecord;     // Pointer to the event code record

    //=====================
    // Event timestamp variables
    //
    epicsFloat64   Time;                // Requested timestamp (in ticks) for this event
    epicsFloat64   TimeStamp;           // Timestamp (in ticks) actually assigned to this event
    dbCommon      *TimeRecord;          // Pointer to the timestamp record

    //=====================
    // Event enable/disable variables
    //
    bool           Enable;              // Event enable/disable flag
    dbCommon      *EnableRecord;        // Pointer to the enable/disable record

    //=====================
    // Event priority variables
    //
    epicsInt32     Priority;            // Event's relative priority (to resolve sorting conflicts)
    dbCommon      *PriorityRecord;      // Pointer to the event priority record

};// end class SequenceEvent //

#endif // EVG_SEQ_EVENT_INC //
