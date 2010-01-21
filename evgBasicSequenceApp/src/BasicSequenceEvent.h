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
    // Getter Functions
    //
    inline const std::string& GetName() const {
        return (EventName);
    }//end GetName()

    //=====================
    // Setter Functions
    //
    void SetEventTime (epicsFloat64 Ticks);

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

    //=====================
    // Event timestamp variables
    //
    epicsFloat64   RequestedTime;       // Requested timestamp (in ticks) for this event
    epicsFloat64   ActualTime;          // Timestamp (in ticks) actually assigned to this event
    dbCommon*      TimeRecord;          // Pointer to the timestamp record

    //=====================
    // Event enable/disable variables
    //
    bool           Enable;              // Event enable/disable flag
    dbCommon*      EnableRecord;        // Pointer to the enable/disable record

    //=====================
    // Event priority variables
    //
    epicsInt32     Priority;            // Event's relative priority (to resolve sorting conflicts)
    dbCommon*      PriorityRecord;      // Pointer to the event priority record

};// end class BasicSequenceEvent //

#endif // EVG_BASIC_SEQ_EVENT_INC //
