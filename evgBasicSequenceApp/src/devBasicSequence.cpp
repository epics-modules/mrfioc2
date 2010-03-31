/**************************************************************************************************
|* $(MRF)/evgBasicSequenceApp/src/devBasicSequence.cpp --  EPICS Device Support for Basic Sequences
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
|*    This module contains EPICS device support for event generator Basic Sequence Event objects.
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
/*  devSequenceEvent File Description                                                             */
/**************************************************************************************************/

//==================================================================================================
//! @addtogroup Sequencer
//! @{
//!
//==================================================================================================
//! @file       devBasicSequence.cpp
//! @brief      EPICS Device Suppport for Basic Sequences
//!
//! @par Description:
//!   This file contains EPICS device support for event generator Basic Sequence objects.
//!
//!   @par
//!   A Basic Sequence can be represented in the EPICS data base by up to three optional records
//!   with the following function codes:
//!   - <b> Update Mode </b>   (mbbo, optional) Determines whether sequence updates should occur
//!                             immediately upon the update of any of the sequence's event records
//!                             or whether updates should accumulate and not effect the sequence
//!                             until an "Update Trigger" reccord is activated.
//!   - <b> Update Trigger </b> (bo, optional) Causes the sequence to be immediately updated.
//!   - <b> Max Time </b>       (ao, optional) Sets the maximum timestamp value for this sequence.
//!
//!   @par
//!   Note that it is not necessary for the Basic Sequence to have any records in the database.
//!   The existence of BasicSequenceEvent records are sufficient to declare the Basic Sequence.
//!
//!   @par
//!   Every sequence must have an event generator and a sequence number associated with it.
//!   The sequence number must be unique to the event generator.  Sequences belonging to different
//!   event generator cards may have the same sequence number (although this practice could be
//!   confusing).  Sequences are not allowed to be shared between event generators.
//!
//! @par Device Type Field:
//!   A basic sequence record will have the "Device Type" field (DTYP) set to:<br>
//!      <b> "EVG Basic Seq" </b>
//!
//! @par Link Format:
//!   Basic sequence records use the INST_IO I/O link format. The INP or OUT links have the
//!   following format:<br>
//!      <b> \@C=n; Seq=m; Fn=\<function\> </b>
//!
//!   @par
//!   Where:
//!   - \b C     = Logical card number for the event generator card this sequence belongs to.
//!   - \b Seq   = Specifies the ID number of the sequence
//!   - \b Fn    = The record's function name (see list above)
//!
//==================================================================================================


/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <stdexcept>           // Standard C++ exception definitions
#include  <string>              // Standard C++ string class
#include  <cstring>             // Standard C string library

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions
#include  <alarm.h>             // EPICS Alarm status and severity definitions
#include  <dbAccess.h>          // EPICS Database access messages and definitions
#include  <devSup.h>            // EPICS Device support messages and definitions
#include  <link.h>              // EPICS Database link definitions
#include  <recGbl.h>            // EPICS Global record support routines

#include  <mrfCommon.h>         // MRF Common definitions
#include  <mrfIoLink.h>         // MRF I/O link field parser
#include  <drvSequence.h>       // MRF Sequence driver support declarations
#include  <devSequence.h>       // MRF Sequence device support declarations
#include  <evg/Sequence.h>      // MRF Sequence base class
#include  <BasicSequence.h>     // MRF BasicSequence class
#include  <BasicSequenceEvent.h>// MRF BasicSequenceEvent class

#include  <epicsExport.h>       // EPICS Symbol exporting macro definitions

/**************************************************************************************************/
/*  Structure And Type Definitions                                                                */
/**************************************************************************************************/

//=====================
// BasicSequence Class ID String
//
const std::string BasicSequenceClassID(BASIC_SEQ_CLASS_ID);

//=====================
// Common I/O link parameter definitions used by all BasicSequence records
//
static const
mrfParmNameList BasicSeqParmNames = {
    "C",        // Logical card number
    "Seq",      // Sequence number
    "Fn"        // Record function code
};//end parameter name table

static const
epicsInt32  BasicSeqNumParms mrfParmNameListSize(BasicSeqParmNames);

/**************************************************************************************************/
/*                                Global Utility Routines                                         */
/*                                                                                                */


//**************************************************************************************************
//  EgDeclareBasicSequence () -- Declare The Existence Of A BasicSequence
//**************************************************************************************************
//! @par Description:
//!   Declare the existence of a basic sequence and return a pointer to its
//!   BasicSequence object.
//!
//! @par Function:
//!   First check to see if the sequence number already exists for the specified event generator
//!   card.  If so, make sure it is a BasicSequence object and return its pointer.
//!   If the sequence was not found, create a new  BasicSequence object, add it to the
//!   sequence list for this card, and return a pointer to its object.
//!
//! @param      Card    = (input) Logical card number of the event generator that this sequence
//!                       will belong to.
//! @param      SeqNum  = (input) Sequence number ID for this sequence.
//!
//! @return     Returns a pointer to the BasicSequence object.
//!
//! @throw      runtime_error is thrown if the sequence could not be created.
//!
//**************************************************************************************************

BasicSequence*
EgDeclareBasicSequence (epicsInt32 Card, epicsInt32 SeqNum) {

    //=====================
    // Local Variables
    //
    Sequence*       pSeq;       // Pointer to generic Sequence object
    BasicSequence*  pNewSeq;    // Pointer to new BasicSequence object

    //=====================
    // See if we already have a sequence for this card and sequence number
    //
    if (NULL != (pSeq = EgGetSequence (Card, SeqNum))) {

        //=====================
        // We found a sequence.  Make sure it is of the correct type
        //
        if (BasicSequenceClassID != pSeq->GetClassID()) {
            throw std::runtime_error (
                  std::string(pSeq->GetSeqID()) +
                  " is a " + pSeq->GetClassID() +
                  " object.  Expected a " + BasicSequenceClassID);
        }//end if this was the wrong type of sequence

        //=====================
        // A basic sequence already exists for this card and sequence number.
        // Return the pointer to it.
        //
        return (static_cast<BasicSequence*>(pSeq));

    }//end if sequence exists.

    //=====================
    // Sequence does not exist.
    // Create a new basic sequence for this card
    //
    pNewSeq = new BasicSequence (Card, SeqNum);

    //=====================
    // Try to add the sequence to the list of known sequences for this card
    //
    try {EgAddSequence(pNewSeq);}
    catch (std::exception& e) {
        delete pNewSeq;
        throw std::runtime_error(e.what());
    }//end if could not add the sequence to the list

    //=====================
    // If there were no errors, return the pointer to the new BasicSequence object
    //
    return (pNewSeq);

}//end EgDeclareBasicSequence()

/**************************************************************************************************/
/*                    Device Support for Basic Sequence Binary Output Records                     */
/*                                                                                                */


/**************************************************************************************************/
/*  Device Support Entry Table (DSET) For Analog Output Records                                   */
/**************************************************************************************************/

extern "C" {
static
SeqAnalogDSET devAoBasicSequence = {
    DSET_SEQ_ANALOG_NUM,                        // Number of entries in the table
    NULL,                                       // -- No device report routine
    NULL,                                       // -- No device support initialization routine
    (DEVSUPFUN)EgSeqAoInitRecord,               // Use generic record initialization routine
    NULL,                                       // -- No get I/O interrupt information routine
    (DEVSUPFUN)EgSeqAoWrite,                    // Use generic write routine
    NULL,                                       // -- No special linear-conversion routine
    (SEQ_DECLARE_FUN)EgDeclareBasicSequence     // Use BasicSequence declaration routine
};

epicsExportAddress (dset, devAoBasicSequence);

};//end extern "C"


/**************************************************************************************************/
/*  Device Support Entry Table (DSET) For Binary Output Records                                   */
/**************************************************************************************************/

extern "C" {
static
SeqBinaryDSET devBoBasicSequence = {
    DSET_SEQ_BINARY_NUM,                        // Number of entries in the table
    NULL,                                       // -- No device report routine
    NULL,                                       // -- No device support initialization routine
    (DEVSUPFUN)EgSeqBoInitRecord,               // Use generic record initialization routine
    NULL,                                       // -- No get I/O interrupt information routine
    (DEVSUPFUN)EgSeqBoWrite,                    // Use generic write routine
    (SEQ_DECLARE_FUN)EgDeclareBasicSequence     // Use BasicSequence declaration routine
};

epicsExportAddress (dset, devBoBasicSequence);

};//end extern "C"


/**************************************************************************************************/
/*               Device Support for Basic Sequence Multi-Bit Binary Output Records                */
/*                                                                                                */


/**************************************************************************************************/
/*  Device Support Entry Table (DSET) For Multi-Bit Binary Output Records                         */
/**************************************************************************************************/

extern "C" {
static
SeqMbbDSET devMbboBasicSequence = {
    DSET_SEQ_MBB_NUM,                           // Number of entries in the table
    NULL,                                       // -- No device report routine
    NULL,                                       // -- No device support initialization routine
    (DEVSUPFUN)EgSeqMbboInitRecord,             // Use generic record initialization routine
    NULL,                                       // -- No get I/O interrupt information routine
    (DEVSUPFUN)EgSeqMbboWrite,                  // Use generic write routine
    (SEQ_DECLARE_FUN)EgDeclareBasicSequence     // Use BasicSequence declaration routine
};

epicsExportAddress (dset, devMbboBasicSequence);

};//end extern "C"


//!
//| @}
//end group Sequencer
