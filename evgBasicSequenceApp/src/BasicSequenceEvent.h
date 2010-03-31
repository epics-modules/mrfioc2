/**************************************************************************************************
|* $(MRF)/evgBasicSequenceApp/src/BasicSequenceEvent.h -- BasicSequenceEvent Class Definition
|*
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     14 January 2010
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 14 Jan 2010  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This header file contains the definition of a BasicSequenceEvent class.
|*
|*   The BasicSequenceEvent class uses individual EPICS records to define a sequence event's
|*   properties such as event code, timestamp, enable/disable status, and collision priority.
|*
|*   BasicSequenceEvent objects define the sequence events in a BasicSequence object.
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

#ifndef EVG_BASIC_SEQ_EVENT_INC
#define EVG_BASIC_SEQ_EVENT_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <string>               // Standard C++ string class

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include <dbCommon.h>           // EPICS Common record structure definitions

#include <BasicSequence.h>      // BasicSequence class definition

/**************************************************************************************************/
/*  Forward Declarations                                                                          */
/**************************************************************************************************/

class BasicSequence;            // BasicSequence class definition

/**************************************************************************************************/
/*                            BasicSequenceEvent Class Definition                                 */
/*                                                                                                */

class BasicSequenceEvent
{

/**************************************************************************************************/
/*  Public Methods                                                                                */
/**************************************************************************************************/

public:

    //=====================
    // Class Constructor
    //
    BasicSequenceEvent (const std::string&  Name, BasicSequence *pSequence);

    //=====================
    // Comparison Operator
    //
    const bool operator < (const BasicSequenceEvent& Event) const;

    //=============================================================================================
    // Required Device Support Routines
    //=============================================================================================

    void            Report       () const;                      // Report Function
    bool            FinishUpdate ();                            // Sequence Update Completion Rtn.
    epicsStatus     SanityCheck  ();                            // Event Sanity Check Function

    //=============================================================================================
    // General Getter Routines
    //=============================================================================================

    inline const std::string& GetName() const {                 // Get event name
        return (EventName);
    }//end GetName()

    //=============================================================================================
    //  Support for "Event Code" Records
    //=============================================================================================

    void            RegisterCodeRecord (dbCommon *pRec);        // Register an "Event Code" record
    epicsStatus     SetEventCode       (epicsInt32 Code);       // Set the event code

    //=============================================================================================
    //  Support for "Enable" Records
    //=============================================================================================

    void            RegisterEnableRecord (dbCommon *pRec);      // Register an "Enable" record
    epicsStatus     SetEventEnable       (bool Value);          // Set the enable/disable status

    inline const bool Enabled () const {                       // Determine if event is enabled
        return (Enable);
    }//end TimeStampLegal()

    //=============================================================================================
    //  Support for "Priority" Records
    //=============================================================================================

    void            RegisterPriorityRecord (dbCommon *pRec);    // Register a "Priority" record
    epicsStatus     SetEventPriority       (epicsInt32 Level);  // Set the event's jostle priority

    //=============================================================================================
    //  Support for "Time" Records
    //=============================================================================================

    void            RegisterTimeRecord (dbCommon *pRec);        // Register a "Time" record
    SequenceStatus  SetEventTime       (epicsFloat64 Ticks);    // Set the desired event time
    bool            SetActualTime      (epicsFloat64 Ticks);    // Set the actual event time

    inline const epicsFloat64 GetEventTime  () const {          // Get the requested timestamp
        return (RequestedTime);
    }//end GetEventTime()

    inline const epicsFloat64 GetActualTime () const {          // Get the actual timestamp
        return (ActualTime);
    }//end GetActualTime()

    inline const bool TimeStampLegal        () const {          // Determine if timestamp is legal
        return (ActualTimeLegal);
    }//end TimeStampLegal()


/**************************************************************************************************/
/*  Private Data                                                                                  */
/**************************************************************************************************/

private:

    //=====================
    // Event name
    //
    const
    std::string     EventName;

    //=====================
    // Pointer to Sequence
    //
    BasicSequence*  pSequence;

    //=====================
    // Event code variables
    //
    epicsUInt8     EventCode;           // Event code
    dbCommon*      EventCodeRecord;     // Pointer to the event code record
    bool           EventCodeChanged;    // True if event code has changed

    //=====================
    // Event timestamp variables
    //
    epicsFloat64   RequestedTime;       // Requested timestamp (in ticks) for this event
    epicsFloat64   ActualTime;          // Timestamp (in ticks) actually assigned to this event
    dbCommon*      TimeRecord;          // Pointer to the timestamp record
    bool           TimeChanged;         // True if timestamp has changed
    bool           ActualTimeLegal;     // True if actual timestamp is inside the legal range

    //=====================
    // Event enable/disable variables
    //
    bool           Enable;              // Event enable/disable flag
    dbCommon*      EnableRecord;        // Pointer to the enable/disable record
    bool           EnableChanged;       // True if enable state has changed

    //=====================
    // Event priority variables
    //
    epicsInt32     Priority;            // Event's relative priority (to resolve sorting conflicts)
    dbCommon*      PriorityRecord;      // Pointer to the event priority record
    bool           PriorityChanged;     // True if the jostle priority has changed

};// end class BasicSequenceEvent //

#endif // EVG_BASIC_SEQ_EVENT_INC //
