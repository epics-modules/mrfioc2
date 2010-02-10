/**************************************************************************************************
|* $(MRF)/evgBasicSequenceApp/src/BasicSequence.cpp -- Event Generator Basic Sequence Class
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     19 November 2009
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 18 Nov 2009  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*    This module contains the implementation of the BasicSequence class
|*
|*   The BasicSequence class uses BasicSequenceEvent objects --collections of individual
|*   records that define a sequence event's properties -- to construct an event sequence
|*   that can be loaded into an event generator's sequence RAMs.
|*
|*   BasicSequences are useful for machines with single (or relatively few) timelines that
|*   have no relationships between the individual events.
|*
\**************************************************************************************************/

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
/*  BasicSequence Class Description                                                               */
/**************************************************************************************************/

//==================================================================================================
//! @addtogroup Sequencer
//! @{
//!
//==================================================================================================
//! @class      BasicSequence
//! @brief      Event Generator BasicSequence Class.
//!
//! @par Description:
//!    A \b BasicSequence object represents a list of events and timestamps.  The timestamp
//!    determines when the event should be delivered relative to the start of the sequence.
//!
//!    Although sequences are associated with event generator cards, A sequence is an abstract
//!    concept and does not in itself reference any hardware.   An event generator may "posses"
//!    mulitple sequences.  Each BasicSequence must have a unique ID number.  The ID number is
//!    assigned when the BasicSequence object is created. A BasicSequence object only becomes
//!    active when it is assigned to a "Sequence RAM". The same BasicSequence object may not be
//!    assigned to more than one Sequence RAM.
//!
//!    BasicSequences are useful for machines with single (or relatively few) timelines that
//!    have no relationships between the individual events.
//!
//==================================================================================================

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <stdexcept>            // Standard C++ exception class
#include <string>               // Standard C++ string class

#include <stdio.h>              // Standard C I/O library

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions

#include <mrfCommon.h>          // MRF Common definitions
#include  <drvEvg.h>            // MRF Event Generator driver infrastructure routines
#include <BasicSequence.h>      // MRF BasicSequence class definition
#include <BasicSequenceEvent.h> // MRF BasicSequenceEvent class definition


/**************************************************************************************************/
/*  Forward Declarations                                                                          */
/**************************************************************************************************/

static void Sort   (BasicSequenceEvent* EventArray[], const epicsInt32 Size);
static void SiftUp (BasicSequenceEvent* EventArray[], const epicsInt32 Start);

/**************************************************************************************************/
/*                               BasicSequence Class Definition                                   */
/*                                                                                                */


//**************************************************************************************************
//  BasicSequence () -- Class Constructor
//**************************************************************************************************
//! @par Description:
//!   Class Constructor
//!
//! @par Function:
//!   Create an event generator BasicSequence object and assign it to the specified event generator
//!   card using the specified sequence ID number.
//!
//! @param Card    = (input) Event generator logical card number
//! @param Number  = (input) Sequence ID number.
//!
//! @throw         runtime_error is thrown if the event generator card was not configured.
//!
//**************************************************************************************************

BasicSequence::BasicSequence (epicsInt32 Card, epicsInt32  Number):
    ClassID(BASIC_SEQ_CLASS_ID),
    CardNumber(Card),
    SequenceNumber(Number),
    NumEvents(0)
{
    //=====================
    // Create the sequence ID string
    //
    sprintf (SeqID, "Card %d, Seq %d", Card, Number);

    //=====================
    // Initialize the event array
    //
    for (int i = 0;  i < MRF_MAX_SEQUENCE_EVENTS;  i++)
        EventList[i] = NULL;

    //=====================
    // Try to get the EVG object for the card we belong to
    //
    if (NULL == (pEvg = EgGetCard(Card)))
        throw std::runtime_error (std::string(SeqID) + ": Event generator card not configured");

}//end Constructor

//**************************************************************************************************
//  DeclareEvent () -- Declare The Existence Of A Basic Sequence Event
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a basic sequence event and return a pointer to its
//!   BasicSequenceEvent object.
//!
//! @par Function:
//!   First check to see if the named event is already in this sequence.  If so, just return a
//!   pointer to the BasicSequenceEvent object.  If the event was not found, create a new
//!   BasicSequenceEvent object, add it to the sequence, and return a pointer to its object.
//!
//! @param      Name  = (input) Name of the event to create
//!
//! @return     Returns a pointer to the named BasicSequenceEvent object.
//!
//! @throw      runtime_error is thrown if the event was not in the list and we could not
//!             create a new event.
//!
//! @par Member Variables Referenced:
//! - \e        NumEvents = (modified) Number of events in this sequence
//! - \e        EventList = (modified) List of BasicSequenceEvent objects in this sequence
//!
//**************************************************************************************************

BasicSequenceEvent*
BasicSequence::DeclareEvent (const std::string &Name)
{
    //=====================
    // If the event is already in the event list, just return a pointer to its object
    //
    BasicSequenceEvent  *Event = GetEvent(Name);
    if (NULL != Event) return Event;

    //=====================
    // If the event is not already in the list, see if there is room to create one.
    //
    if (NumEvents >= MRF_MAX_SEQUENCE_EVENTS)
        throw std::runtime_error("Sequence is full");

    //=====================
    // If there is still room, try and create a new BasicSequenceEvent object
    //
    try {Event = new BasicSequenceEvent (Name, this);}
    catch (std::exception& e) {
        throw std::runtime_error(
              std::string("Unable to create new BasicSequenceEvent object - ") + e.what());
    }//end if could not create a new BasicSequenceEvent object

    //=====================
    // Add the new BasicSequenceEvent to the list and return a pointer to it.
    //
    EventList[NumEvents++] = Event;
    return Event;

}//end DeclareEvent()

//**************************************************************************************************
//  GetEvent () -- Retrieve An Event By Name
//**************************************************************************************************
//! @par Description:
//!   Retrieve an event by name
//!
//! @par Function:
//!   Searches the sequence's event table for an event whose name matches the name in the input
//!   parameter.  Returns a pointer to the BasicSequenceEvent object if it is found.
//!
//! @param      Name  = (input) Name of the event to retrieve
//!
//! @return     Returns a pointer to the named BasicSequenceEvent object.<br>
//!             Returns NULL if the requested object was not found
//!
//! @par Member Variables Referenced:
//! - \e        NumEvents = (input) Number of events in this sequence
//! - \e        EventList = (input) List of BasicSequenceEvent objects in this sequence
//!
//**************************************************************************************************

BasicSequenceEvent*
BasicSequence::GetEvent (const std::string &Name)
{
    //=====================
    // Loop to look for the requested event
    //
    for (int i = 0;  i < NumEvents;  i++) {
        if (Name == EventList[i]->GetName())
            return EventList[i];
    }//end for each event in this sequence

    //=====================
    // Return NULL if the event was not found
    //
    return NULL;

}//end GetEvent()

//**************************************************************************************************
//  Finalize () -- Complete the BasicSequence Object Initialization
//**************************************************************************************************
//! @par Description:
//!   This routine is called during the "After Interrupt Accept" phase of the iocInit() process.
//!   It performs completes the the BasicSequence object initialization after all the sequence
//!   records and sequence event records have been initially processed.
//!
//! @par Function:
//! - Perform a "Sanity Check" on each of the BasicSequenceEvent objects in this sequence.
//!   Discard those events that don't pass the sanity check.  
//! - Sort the events by timestamp and construct a sequence.
//!
//! @par Member Variables Referenced:
//! - \e        NumEvents = (modified) Number of events in this sequence
//! - \e        EventList = (modified) List of BasicSequenceEvent objects in this sequence
//!
//**************************************************************************************************

void
BasicSequence::Finalize () {

    //=====================
    // Local Variables
    //
    epicsInt32  index;          // Local index variable
    epicsInt32  listEnd;        // Index of last element in the event list.

    //=====================
    // First, run a sanity check on each event in our list.
    // Discard the events that don't pass.
    //
    listEnd = NumEvents-1;
    for (index = 0;  index < NumEvents;) {
        if (OK == (EventList[index])->SanityCheck()) index++;

        //=====================
        // If the current event did not pass its sanity check,
        // replace it with the last event in the list and decrement
        // the event count.
        else {
            EventList[index] = EventList[listEnd];
            NumEvents--;
            listEnd--;
        }//end if we discarded an event

    }//end for each event in our list

    Sort (EventList, NumEvents);

}//end Finalize();

//**************************************************************************************************
//  Report () -- Display Information About This Sequence
//**************************************************************************************************
//! @par Description:
//!   This routine displays information about a BasiceSequence object.  The information displayed
//!   is (depending on the requested display level):
//!   - Card number of the EVG the sequence belongs to.
//!   - Sequence number
//!   - Sequence type (e.g. BasicSequence)
//!   - Information on each event in the sequence
//!
//! @param   Level     = (input) Report detail level<br>
//!                              Level  = 0: No Report<br>
//!                              Level >= 1: Display the sequence number and type<br>
//!                              Level >= 2: Display the sequence events
//!
//! @par Member Variables Referenced:
//! - \e     ClassID   = (input) Sequence class ID string (e.g. "BasicSequence")
//! - \e     EventList = (input) List of BasicSequenceEvent objects in this sequence
//! - \e     NumEvents = (input) Number of events in this sequence
//! - \e     SeqID     = (input) Sequence ID string (contains card & sequence number)
//!
//**************************************************************************************************

void
BasicSequence::Report (epicsInt32 Level) const {

    //=====================
    // If Level >= 1, Display the sequence number and type
    //
    if (Level < 1) return;
    printf ("  %s (%s)\n", SeqID, ClassID.c_str());

    //=====================
    // If Level >= 2, Display the sequence events
    //
    if (Level < 2) return;

    printf ("    Event  Enable   Time Stamp (ticks)  Priority  Name\n");
    for (epicsInt32 i = 0;  i < NumEvents;  i++)
        EventList[i]->Report();

    printf ("\n");

}//end Report()

/**************************************************************************************************/
/*                             BasicSequence Non-Member Functions                                 */
/*                                                                                                */


/**************************************************************************************************
|* Sort () -- Sort the Sequence Event List
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    Sort the sequence event list in ascending order based on timestamp and priority.
|*    We use a simple "Bubble-Sort" variant since the event list is expected to be
|*    mostly ordered once it is initialized.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    Sort (EventArray, Size);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    Size        = (epicsInt32)          Number of elements in the event list.
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFIED PARAMETERS:
|*    EventArray  = (BasicSequenceEvent*) Array of pointers to this sequence's sequence event
|*                                        objects.
|*
\**************************************************************************************************/

static void
Sort (BasicSequenceEvent* EventArray[], const epicsInt32 Size) {

    //=====================
    // Local Variables
    //
    epicsInt32           i, j;          // Local index variables
    BasicSequenceEvent*  temp;          // Temorary storage for swapping

    //=====================
    // Loop to examine each element and put it in its proper place
    // Invariant:  The list is sorted between the first element and the i'th element.
    //
    for (i=0;  i < (Size-1);  i++) {
        j = i + 1;

        //=====================
        // An element is out of place, if its value is less than the element before it.
        // Swap the element with its predecessor, and then sift it up to its proper place
        // in the list.
        //
        if (*(EventArray[j]) < *(EventArray[i])) {
            temp = EventArray[i];
            EventArray[i] = EventArray[j];
            EventArray[j] = temp;
            SiftUp (EventArray, i);
        }//end if element j is in the wrong order

    }//end for each element in the array

}//end Sort();

/**************************************************************************************************
|* SiftUp () -- Move a Sequence Event To Its Proper Place In The List
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    Move the out-of-place element up the list until we find an element with a smaller value.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    SiftUp (EventArray, Start);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    Size        = (epicsInt32)          Index of the out-of-place element
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFIED PARAMETERS:
|*    EventArray  = (BasicSequenceEvent*) Array of pointers to this sequence's sequence event
|*                                        objects.
|*
\**************************************************************************************************/


static void
SiftUp (BasicSequenceEvent* EventArray[], const epicsInt32 Start) {

    //=====================
    // Local Variables
    //
    epicsInt32           i, j;          // Local index variables
    BasicSequenceEvent*  temp;          // Temorary storage for swapping

    //=====================
    // Loop to sift the out-of-place element up to its proper location:
    // Invariant:  The index "j" always points to the out-of-place element.
    //
    for (j = Start;  j > 0;  j--) {
        i = j - 1;

        //=====================
        // If element "j" is less than element "i", then element "j" is still out of order
        //
        if (*(EventArray[j]) < *( EventArray[i])) {
            temp = EventArray[i];
            EventArray[i] = EventArray[j];
            EventArray[j] = temp;
        }//end if element j is in the wrong order

        //=====================
        // If element "j" is not less than element "i", then quit
        // (we have found its proper location in the list)
        //
        else return;

    }//end for each element above the starting element

}//end SiftUp()


//!
//! @}
//end group Sequencer
