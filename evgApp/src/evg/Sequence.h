/**************************************************************************************************
|* $(MRF)/evgApp/src/evg/Sequence.h -- Event Generator Sequence Base Class Definition
|*
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     1 March 2010
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 01 Mar 2010  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This header file contains the virtual function definitions required to implement an
|*   MRF event generator sequence class.
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

#ifndef EVG_SEQUENCE_H_INC
#define EVG_SEQUENCE_H_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <string>              // Standard C++ string class

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions
#include  <dbCommon.h>          // EPICS Common record fields


/**************************************************************************************************/
/*  Type Definitions                                                                              */
/**************************************************************************************************/

//=====================
// State values for the Sequence Update status
//
enum SequenceStatus {
    SeqStat_Idle,       // Sequence is not updating
    SeqStat_Start,	// Sequence update started
    SeqStat_Busy        // Accumulate update is in progress
};//end SequenceUpdateMode

//=====================
// State values for the Sequence Update Mode
//
enum SequenceUpdateMode {
    SeqUpdateModeImmediate = 0, // Update sequence on each change
    SeqUpdateModeOnTrigger = 1  // Accumulate changes and update on trigger
};//end SequenceUpdateMode

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
    // Sequence exclusive access routines
    //
    virtual void  lock   () = 0;  // Acquire exclusive access to the Sequence
    virtual void  unlock () = 0;  // Release exclusive access to the Sequence

    //=====================
    // Getter Routines
    //
    virtual const std::string&  GetClassID        () const = 0; // Return the class ID string
    virtual const char*         GetSeqID          () const = 0; // Return the sequence ID string
    virtual epicsInt32          GetCardNum        () const = 0; // Return the logical card number
    virtual epicsInt32          GetSeqNum         () const = 0; // Return sequence number
    virtual epicsInt32          GetNumEvents      () const = 0; // Return num events in sequence
    virtual const epicsInt32   *GetEventArray     () const = 0; // Return ptr to array of events
    virtual const epicsUInt32  *GetTimeStampArray () const = 0; // Return ptr to array of timestamps

    //=====================
    // Support for "Update Mode" records
    //
    virtual void               RegisterUpdateModeRecord (dbCommon *pRec)          = 0;
    virtual SequenceStatus     SetUpdateMode            (SequenceUpdateMode Mode) = 0;
    virtual SequenceUpdateMode GetUpdateMode            ()                  const = 0;

    //=====================
    // Support for "Update Trigger" records
    //
    virtual void             RegisterUpdateTriggerRecord (dbCommon *pRec) = 0;
    virtual SequenceStatus   StartUpdate                 ()               = 0;

    //=====================
    // Sequence Finalizer
    //
    virtual void  Finalize () = 0;

    //=====================
    // Sequence Report
    //
    virtual void Report (epicsInt32 Level) const = 0;

    //=====================
    // Class Destructor
    //
    virtual ~Sequence () = 0;

};// end class Sequence //

#endif // EVG_SEQUENCE_H_INC //
