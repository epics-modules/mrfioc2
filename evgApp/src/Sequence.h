#ifndef EVG_SEQUENCE_H_INC
#define EVG_SEQUENCE_H_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <string>               // Standard C++ string class
#include <epicsTypes.h>         // EPICS Architecture-independent type definitions

#include <mrfCommon.h>          // MRF Common definitions

#include <SequenceEvent.h>      // SequenceEvent class definition

/**************************************************************************************************/
/*  Configuration Parameters                                                                      */
/**************************************************************************************************/

#define MAX_SEQUENCE_CHARS  10  //Maximimum number of characters in a sequence number

/**************************************************************************************************/
/*                                  Sequence Class Definition                                     */
/*                                                                                                */

class Sequence
{

/**************************************************************************************************/
/*  Public Methods                                                                                */
/**************************************************************************************************/

public:

    //=====================
    // Class Constructor
    //
    Sequence (epicsInt32 Number);

    //=====================
    // Get the sequence number as an ASCII string
    //
    inline const char *GetSequenceString() {
        return (SeqNumString);
    }//end GetSequenceNumber()

    //=====================
    // Create a new event and add it to the sequence
    //
    SequenceEvent *DeclareEvent (const std::string &Name);

    //=====================
    // Get an event by name
    //
    SequenceEvent *GetEvent (const std::string &Name);


/**************************************************************************************************/
/*  Private Data                                                                                  */
/**************************************************************************************************/

private:

    epicsInt32      SequenceNumber;                      // Unique sequence number for this sequence
    char            SeqNumString [MAX_SEQUENCE_CHARS];   // Sequence number as an ASCII string

    //=====================
    // Event List
    //
    epicsInt32      NumEvents;                           // Number of events in the array
    SequenceEvent  *EventList [MRF_MAX_SEQUENCE_EVENTS]; // Array of sequence event pointers

};// end class Sequence //

#endif // EVG_SEQUENCE_H_INC //
