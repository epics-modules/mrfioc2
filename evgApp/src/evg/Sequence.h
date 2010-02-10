/**************************************************************************************************
|* $(MRF)/evgApp/src/evg/Sequence.h -- Event Generator Sequence Base Class Definition
|*
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     13 January 2010
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 13 Jan 2010  E.Bjorklund     Original
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

#include <string>               // Standard C++ string class
#include <epicsTypes.h>         // EPICS Architecture-independent type definitions

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
    // Getter Routines
    //
    virtual const std::string&  GetClassID        () const = 0; // Return the class ID string
    virtual const char*         GetSeqID          () const = 0; // Return the sequence ID string
    virtual epicsInt32          GetCardNum        () const = 0; // Return the logical card number
    virtual epicsInt32          GetSeqNum         () const = 0; // Return sequence number
    virtual epicsInt32          GetNumEvents      () const = 0; // Return num events in sequence
    virtual const epicsInt32   *GetEventArray     () const = 0; // Return ptr to array of events
    virtual const epicsUInt32  *GetTimestampArray () const = 0; // Return ptr to array of timestamps

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
