/**************************************************************************************************
|* $(MRF)/evgApp/src/drvSequence.cpp -- EPICS Generic Driver Support for EVG Sequences
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     23 November 2009
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 23 Nov 2009  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*    This module contains EPICS driver support for event generator Sequence objects.
|*
|*    An event generator sequence is an abstract object that has no hardware implementation.
|*    Its purpose is to provide the event and timestamp lists used by the EVG Sequence RAM
|*    objects.
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

//==================================================================================================
//  Sequencer Group Definition
//==================================================================================================
//! @defgroup   Sequencer Event Generator Sequence Control Libraries
//! @brief      Libraries for implementing event generator sequences.
//!
//! An "Event Sequence" is a method of transmitting sequences of events stored in a "Sequence RAM".
//! An event generator card typically contains two sequence RAMs. However, an event generator may
//! have any number of defined sequences.  A sequence becomes active by assigning it to a sequence
//! RAM and starting it.
//!
//! Several types of sequences are possible.  This software implements three sequence libraries:
//!  - \b libBasicSequence - Implements the "Basic" sequence in which each "Sequence Event" has
//!                          an event code, a timestamp (to determine when the event should occur
//!                          relative to the start of the sequence), an enable/disable record,
//!                          and a priority (used for resolving timestamp conflicts).  Basic
//!                          Sequences are useful for machines with single (or a relative few)
//!                          timelines that have no relationships between the individual events.
//!  - \b libDAGSequence   - (not yet implemented) Implements the "Directed Acyclic Graph" sequence.
//!                          A DAG sequence is like the Basic sequence but with the addition of
//!                          optional "Time Base" records.  "Time Base" records declare that the
//!                          event's "Timestamp" record is relative to the timestamp of another
//!                          event rather than the start of the sequence.  The "Time Base" records
//!                          implement a directed acylic graph which is traversed whenever a
//!                          DAG sequence is loaded or updated.  DAG sequences are useful for
//!                          machines with timelines that contain sub-sequences.
//!  - \b libWFSequence    - (not yet implemented) Implements the "Waveform" sequence. A Waveform
//!                          sequence contains two waveform records -- an "Event Waveform" and
//!                          a "Timestamp" waveform.  Waveform sequences are useful for machines
//!                          with timelines that need to be set from external sources such as
//!                          operator interface screens.
//! 
//! @{
//!
//==================================================================================================

//==================================================================================================
//  drvSequence File Description
//==================================================================================================
//! @file       drvSequence.cpp
//! @brief      EPICS Generic Driver Support for Event Generator Sequencers.
//!
//! @par Description:
//!    This file provides EPICS generic driver support for event generator Sequence objects.
//!
//!    Although sequences are associated with event generator cards, an event generator
//!    sequence is an abstract object that has no hardware implementation.
//!    Its purpose is to provide the event and timestamp lists used by the EVG Sequence RAM
//!    objects.
//!
//==================================================================================================

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <stdexcept>           // Standard C++ exception definitions
#include  <string>              // Standard C++ sring class
#include  <map>                 // Standard C++ map template

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions
#include  <devSup.h>            // EPICS Device support definitions
#include  <initHooks.h>         // EPICS IOC Initialization hooks support library
#include  <iocsh.h>             // EPICS IOC shell support library
#include  <registryFunction.h>  // EPICS Registry support library

#include  <evg/Sequence.h>      // MRF Sequence Base Class
#include  <drvSequence.h>       // MRF Sequencer driver support declarations
#include  <drvEvg.h>            // MRF Event Generator driver infrastructure routines

#include  <epicsExport.h>       // EPICS Symbol exporting macro definitions

/**************************************************************************************************/
/*  Type Definitions                                                                              */
/**************************************************************************************************/

//=====================
// SequenceList:  Associates sequence numbers with Sequence objects
//
typedef std::map <epicsInt32, Sequence*>      SequenceList;

//=====================
// CardList:  Associates card numbers with sequence lists
//
typedef std::map <epicsInt32, SequenceList*>  CardList;

/**************************************************************************************************/
/*  Variables Global To This Module                                                               */
/**************************************************************************************************/

//=====================
// List of sequence objects for each event generator card
//
static
CardList   CardSequences;

/**************************************************************************************************/
/*  Sequence Base Class Destructor                                                                */
/**************************************************************************************************/

Sequence::~Sequence() {}

//**************************************************************************************************
//  EgAddSequence () -- Add A Sequence To The List Of Known Sequences For Each Card
//**************************************************************************************************
//! @par Description:
//!   Add a Sequence object to the list of known sequences for the EVG card it belongs to.
//!
//! @par Function:
//!  -  Check for various error conditions such as card not initilialized and card/seq pair
//!     already on list.
//!  -  Create a sequence list for the card, if one does not already exist.
//!  -  Add the Sequence object to the sequence list for its EVG card.
//!
//! @param      pSeq  = (input) Address of Sequence object to add to the list.
//!
//! @throw      runtime_error is thrown if we needed to create a new Sequence object but couldn't.
//!
//! @par External Data Referenced:
//! - \e        CardSequences = (modified) List of sequences for each EVG card.
//!
//**************************************************************************************************

void
EgAddSequence (Sequence*  pSeq)
{
    //=====================
    // Local variables
    //
    SequenceList*   List;       // Reference to the sequence list for the specified EVG card

    //=====================
    // Extract the EVG card and sequence numbers
    // Note that a sequence can not be created if the event generator card was not configured
    //
    epicsInt32 CardNum = pSeq->GetCardNum();
    epicsInt32 SeqNum  = pSeq->GetSeqNum();

    //=====================
    // Try to add this Sequence to the sequence list for its EVG card
    //
    try {

        //=====================
        // Get the sequence list for this card
        //
        CardList::iterator card = CardSequences.find(CardNum);
        if (card != CardSequences.end())
            List = card->second;

        //=====================
        // If we don't have a sequence list for this EVG card, create one
        //
        else {
            try {List = new SequenceList();}

            /* Abort if we could not create the sequence list */
            catch (std::exception& e) {
                throw std::runtime_error(std::string("Can't create sequence list: ") + e.what());
            }//end if we could not create a sequence list

            /* Add the sequence list to this card */
            CardSequences[CardNum] = List;

        }//end if we didn't already have a sequence list for this card

        //=====================
        // See if we already have a sequence with this number for this card.
        // 
        SequenceList::iterator index = List->find(SeqNum);
        if (index != List->end())
            throw std::runtime_error("Sequence already exists");

        //=====================
        // Add the Sequence object to the list
        //
        (*List)[SeqNum] = pSeq;

    }//end try

    //=====================
    // Catch any errors and rethrow them with a standard message context
    //
    catch (std::exception& e) {
        throw std::runtime_error(
              std::string("Can't create sequence for ") +
              pSeq->GetSeqID() + ": " + e.what());
    }//end rethrow error

}//end EgAddSequence()

//**************************************************************************************************
//  EgGetSequence () -- Retrieve A Sequence Object
//**************************************************************************************************
//! @par Description:
//!   Retrieve the requested sequence object
//!
//! @par Function:
//!   Searches the sequence table a sequence whose number matches the number specified in the input
//!   parameter.  Returns a pointer to the Sequence object if it is found.
//!
//! @param      CardNum  = (input) Card number of the event generator this sequence belongs to.
//! @param      SeqNum   = (input) Sequence ID number of the sequence to retrieve
//!
//! @return     Returns a pointer to the Sequence object.<br>
//!             Returns NULL if the requested object was not found
//!
//! @par External Data Referenced:
//! - \e        CardSequences = (input) List of sequences for each EVG card.
//!
//**************************************************************************************************

Sequence*
EgGetSequence (epicsInt32 CardNum, epicsInt32 SeqNum)
{
    //=====================
    // Local variables
    //
    SequenceList*   List;       // Reference to the sequence list for the specified EVG card

    //=====================
    // First, make sure the EVG card has been initialized
    //
    if (NULL == EgGetCard(CardNum))
        return NULL;

    //=====================
    // Next, see if any sequences have been defined for this card
    //
    CardList::iterator card = CardSequences.find(CardNum);
    if (card == CardSequences.end())
        return NULL;
    List = card->second;
        
    //=====================
    // Finally, look for the requested sequence in the card's sequence list
    //
    SequenceList::iterator index = List->find(SeqNum);
    if (index != List->end()) {
        return (index->second);
    }//end if the Sequence object is in the list

    //=====================
    // Return NULL if the sequence was not found
    //
    return NULL;

}//end EgGetSequence()

//**************************************************************************************************
//  EgFinalizeSequences () -- Finalize the Sequences Assigned to an Event Generator Card
//**************************************************************************************************
//! @par Description:
//!   This routine is called during the "After Interrupt Accept" phase of the iocInit() process.
//!   It performs the "finalization" process for all the sequences attached to a particular
//!   event generator card.  The "finalization" process involves different steps, depending on the
//!   type of sequence defined, but typically results in the construction of an event sequence
//!   that can be loaded into the event generator's sequence RAMS.
//!
//! @par Function:
//!   Invoke the Finalize() method for each sequence object connected to the specified
//!   event generator card.
//!
//! @param   CardNum       = (input) Card number of the event generator
//!
//! @par External Data Referenced:
//! - \e     CardSequences = (input) List of sequences for each EVG card.
//!
//**************************************************************************************************

void
EgFinalizeSequences (epicsInt32 CardNum)
{
    //=====================
    // Local variables
    //
    SequenceList*   List;       // Reference to the sequence list for the specified EVG card
    Sequence*       pSequence;  // Sequence object to be finalized.

    //=====================
    // Get the sequence list for this card.
    // Quit if no sequences have been defined for this card.
    //
    CardList::iterator Card = CardSequences.find(CardNum);
    if (Card == CardSequences.end())
        return;
        
    //=====================
    // Loop to finalize each defined sequence
    //
    List = Card->second;
    for (SequenceList::iterator index = List->begin();  index != List->end();  index++) {
        pSequence = index->second;
        pSequence->Finalize();
    }//end for each Sequence object in the list

}//end EgFinalizeSequences()

//**************************************************************************************************
//  EgReportSequences () -- Display Each Sequence Assigned to an Event Generator Card
//**************************************************************************************************
//! @par Description:
//!   This routine is called by the event generator driver report routine to report on the
//!   sequences defined for the specified event generator card.
//!
//! @par Function:
//!   Invoke the Report() method for each sequence object connected to the specified
//!   event generator card.
//!
//! @param   CardNum       = (input) Card number of the event generator
//! @param   Level         = (input) Report detail level<br>
//!                                  Level  = 0: No Report<br>
//!                                  Level >= 1: Display the sequence headers<br>
//!                                  Level >= 2: Display the sequence events
//!
//! @par External Data Referenced:
//! - \e     CardSequences = (input) List of sequences for each EVG card.
//!
//**************************************************************************************************

void
EgReportSequences (epicsInt32 CardNum, epicsInt32 Level)
{
    //=====================
    // Local variables
    //
    SequenceList*   List;       // Reference to the sequence list for the specified EVG card
    Sequence*       pSequence;  // Sequence object to be finalized.

    //=====================
    // Get the sequence list for this card.
    // Quit if no sequences have been defined for this card.
    //
    CardList::iterator Card = CardSequences.find(CardNum);
    if (Card == CardSequences.end())
        return;
        
    //=====================
    // Loop to report on each sequence defined for this card
    //
    List = Card->second;
    for (SequenceList::iterator index = List->begin();  index != List->end();  index++) {
        pSequence = index->second;
        pSequence->Report(Level);
    }//end for eac Sequence object in the list

}//end EgReportSequences()


//!
//| @}
//end group Sequencer
