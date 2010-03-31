/**************************************************************************************************
|* $(MRF)/evgApp/src/devSequence.cpp -- EPICS Generic Device Support for EVG Sequences
|*-------------------------------------------------------------------------------------------------
|* Authors:  Eric Bjorklund (LANSCE)
|* Date:     3 March 2010
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 03 Mar 2010  E.Bjorklund     Original
|*
|*-------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*    This module contains generic EPICS device support for event generator Sequence objects.
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
//  devSequence File Description
//==================================================================================================

//==================================================================================================
//! @addtogroup Sequencer
//! @{
//!
//==================================================================================================
//! @file       devSequence.cpp
//! @brief      EPICS Generic Device Support for Event Generator Sequence Objects.
//!
//! @par Description:
//!   This file contains generic EPICS device support for event generator sequences.
//!
//!   @par
//!   A sequence can be represented in the EPICS data base by up to three optional records with
//!   the following function codes:
//!   - <b> Update Mode </b>   (mbbo, optional) Determines whether sequence updates should occur
//!                             immediately upon the update of any of the sequence's event records
//!                             or whether updates should accumulate and not effect the sequence
//!                             until an "Update Trigger" reccord is activated.
//!   - <b> Update Trigger </b> (bo, optional) Causes the sequence to be immediately updated.
//!   - <b> Max Time </b>       (ao, optional) Sets the maximum timestamp value for this sequence.
//!
//!   @par
//!   Note that it is not necessary for the sequence itself to have any records in the database.
//!   The existence of sequence event records are sufficient to declare the sequence.
//!
//!   @par
//!   Every sequence must have an event generator and a sequence number associated with it.
//!   The sequence number must be unique to the event generator.  Sequences belonging to different
//!   event generator cards may have the same sequence number (although this practice could be
//!   confusing).  Sequences are not allowed to be shared between event generators.
//!
//! @par Link Format:
//!   Sequence records use the INST_IO I/O link format. The INP or OUT links have the
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

#include  <aoRecord.h>          // EPICS Analog output record definition
#include  <boRecord.h>          // EPICS Binary output record definition
#include  <mbboRecord.h>        // EPICS Multi-Bit Binary output record definition
#include  <menuConvert.h>       // EPICS conversion field menu items

#include  <mrfCommon.h>         // MRF Common definitions
#include  <mrfIoLink.h>         // MRF I/O link field parser
#include  <devSequence.h>       // MRF Sequence device support declarations
#include  <evg/Sequence.h>      // MRF Sequence base class

/**************************************************************************************************/
/*  Structure And Type Definitions                                                                */
/**************************************************************************************************/

//=====================
// Common I/O link parameter definitions used by all Sequence records
//
static const
mrfParmNameList SequenceParmNames = {
    "C",        // Logical card number
    "Seq",      // Sequence number
    "Fn"        // Record function code
};//end parameter name table

static const
epicsInt32  SequenceNumParms mrfParmNameListSize(SequenceParmNames);

//=====================
// Common device information structure used by all Sequence records
//
struct devInfoStruct {
    Sequence*  pSequence;       // Pointer to the Sequence object
    bool       waiting;         // True if we are waiting on the Sequence object
};//end devInfoStruct

/**************************************************************************************************/
/*                                Common Utility Routines                                         */
/*                                                                                                */


/**************************************************************************************************
|* parseLink () -- Parse An EPICS Database Link Structure
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o Parse the record's EPICS Database input (INP) or output (OUT) field.
|*    o Lookup the Sequence object that belongs to this record. If it does not already exist,
|*      it will be created.
|*    o Create a device information structure and initialize it with pointers to the
|*      Sequence object associated with this record.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    pDevInfo = parseLink (dbLink, DeclareSequence, Function);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    dbLink          = (DBLINK &)          Reference to the record's INP or OUT link field.
|*    DeclareSequence = (EgDeclareSequence) Pointer to the function we should use to look up or
|*                                          declare the correct type of sequence.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS:
|*    Function        = (string &)          The record function string
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    pDevInfo = (devInfoStruct *) Pointer to partially filled in device information structure
|*
|*-------------------------------------------------------------------------------------------------
|* EXCEPTIONS:
|*    A runtime_error is thrown if there was an error parsing the I/O link field
|*
\**************************************************************************************************/

static
devInfoStruct* parseLink (
    const DBLINK&        dbLink,             // Reference to the EPICS database I/O link field
    SEQ_DECLARE_FUN      DeclareSequence,    // Function to declare this sequence to the EVG.
    std::string&         Function)           // Reference to returned function name string
{
    //=====================
    // Local variables
    //
    epicsInt32           Card;               // Logical EVG card number
    epicsInt32           SeqNum;             // Sequence ID number
    mrfIoLink*           ioLink = NULL;      // I/O link parsing object
    devInfoStruct*       pDevInfo = NULL;    // Pointer to device-specific information structure.
    Sequence*            pSequence;          // Pointer to the Sequence object

    //=====================
    // Make sure the link type is correct
    //
    if (INST_IO != dbLink.type)
        throw std::runtime_error("I/O link type is not INST_IO");

    //=====================
    // Try to parse the link field
    //
    try {

        //=====================
        // Parse the link field
        //
        ioLink   = new mrfIoLink (dbLink.value.instio.string, SequenceParmNames, SequenceNumParms);
        Card     = ioLink->getInteger ("C"   );
        SeqNum   = ioLink->getInteger ("Seq" );
        Function = ioLink->getString  ("Fn"  );
        delete ioLink;

        //=====================
        // Declare the sequence number.
        //
        pSequence = DeclareSequence (Card, SeqNum);

        //=====================
        // Create and initialize the device information structure
        //
        pDevInfo = new devInfoStruct;
        pDevInfo->pSequence = pSequence;
        pDevInfo->waiting = false;

        //=====================
        // Return the address of the device information structure
        //
        return pDevInfo;

    }//end try to parse the I/O link field

    //=====================
    // Catch any link parsing errors:
    //  - Delete the ioLink object
    //  - Delete the device information structure
    //  - Rethrow the error
    //
    catch (std::exception& e) {
        if (NULL != ioLink)   delete ioLink;
        if (NULL != pDevInfo) delete pDevInfo;
        throw std::runtime_error(e.what());
    }//end if there was an error parsing the link
    
}//end parseLink()

/**************************************************************************************************/
/*                    Device Support for Basic Sequence Analog Output Records                     */
/*                                                                                                */


//**************************************************************************************************
//  EgSeqAoInitRecord () -- Initialize An Analog Output Record
//**************************************************************************************************
//! @par Description:
//!   This routine is called during IOC initialization to initialize a Sequence
//!   analog output (ao) record. The ao record is used to set the maximum timestamp for a Sequence.
//!
//!   @par
//!   To use this routine, place its address in the "init_record" field of the ao DSET.
//!
//! @par Function:
//!   - Parse the OUT field.
//!   - Register the record as this Sequence's "Max TimeStamp" record.
//!
//! @par Implemented Function Codes:
//! - <b> Max Time </b> Sets the maximum timestamp value for the sequence
//!
//! @param      pRec  = (input) Pointer to the record we wish to initialize
//!
//! @return     Returns OK if the record was successfully initialized.<br>
//!             Returns a failure code if the record could not be initialized.
//!
//! @note
//!   This routine expects an "augmented" ao device support entry table (DSET) which contains
//!   an entry for a sequence declaration function.
//!
//**************************************************************************************************

epicsStatus EgSeqAoInitRecord (aoRecord *pRec)
{
    //=====================
    // Local variables
    //
    SeqAnalogDSET*       pDset;              // Pointer to record's DSET
    std::string          Function;           // Record function name (from OUT link)
    devInfoStruct*       pDevInfo = NULL;    // Pointer to device-specific information structure.
    epicsStatus          status;             // Anticipated error status

    //=====================
    // Try to initialize the record
    //
    try {

        //=====================
        // Make sure the record's DSET is valid
        //
        status = S_dev_missingSup;
        pDset = (SeqAnalogDSET *)pRec->dset;
        if ((pDset->number < DSET_SEQ_ANALOG_NUM) || (NULL == pDset->declare_sequence))
            throw std::runtime_error("'DeclareSequence' routine missing from DSET");

        //=====================
        // Parse the OUT link
        //
        status = S_dev_badOutType;
        pDevInfo = parseLink (pRec->out, pDset->declare_sequence, Function);

        //=====================
        // Make sure the function code is correct
        //
        if ("Max Time" != Function)
            throw std::runtime_error("Invalid record function (" + Function + ")");

        //=====================
        // Register the "Max Time" record
        //
        status = S_dev_Conflict;
        pDevInfo->pSequence->RegisterMaxTimeStampRecord ((dbCommon *)pRec);

        //=====================
        // Disable automatic value conversion by record support.
        // We have to do our own conversion because the raw timestamp could
        // exceed 32 bits.
        //
        pRec->linr = menuConvertNO_CONVERSION;

        //=====================
        // Compute the slope.
        // If EGUF is 0, it means we want event clock ticks.
        //
        if (0.0 == pRec->eguf)
            pRec->eslo = 1.0;
        else
            pRec->eslo = pDevInfo->pSequence->GetSecsPerTick() / pRec->eguf;

        //=====================
        // Attach the device information structure to the record and return
        // Do not attempt a raw value conversion
        //
        pRec->dpvt = (void *)pDevInfo;
        return NO_CONVERT;

    }//end try

    //=====================
    // If record initialization failed, report the error,
    // disable the record, and return the error status code
    //
    catch (std::exception& e) {
        recGblRecordError (status, pRec, (std::string("\nReason: ") + e.what()).c_str());
        mrfDisableRecord ((dbCommon *)pRec);
        if (NULL != pDevInfo) delete pDevInfo;
        return (status);
    }//end if record initialization failed

}//end EgSeqAoInitRecord()

//*************************************************************************************************
// EgSeqAoWrite () -- Write the Maximum Timestamp Value to the BasicSequenceEvent Object.
//*************************************************************************************************
//! @par Description:
//!   This routine is called from the aoRecord's "process()" routine.  It converts the maximum
//!   timestamp value in the VAL field from engineering units to event clock ticks and writes it
//!   to the BasicSequence object.
//!
//!   @par
//!   This routine can be called either as a result of standard record processing, or as
//!   an asynchronous record callback.  If the Sequence was updating at the time this routine
//!   was called, asynchronous record completion will be invoked to postpone the write until
//!   the Sequence update is complete.
//!
//!   @par
//!   Note that the "raw" value (ticks) is represented by a 64-bit floating point number. This is
//!   because a sequence event timestamp can be as big as 2**44 ticks.  This means that we can't
//!   use the ao record's built in linear, slope, or breakpoint table conversion features.
//!   Consequently, the LINR field should be set to "NO CONVERSION".  This will disable the
//!   special linear conversion routine, so we recompute ESLO every time we are called (just in
//!   case EGUF was changed).
//!
//! @par Implemented Function Codes:
//! - <b> Max Time </b> Sets the maximum timestamp value for the sequence
//!
//! @param      pRec  = (input) Pointer to the ao record structure
//!
//! @return     Always returns OK
//!
//! @note
//!   This routine expects an "augmented" ao device support entry table (DSET) which contains
//!   an entry for a sequence declaration function.
//!
//**************************************************************************************************

epicsStatus EgSeqAoWrite (aoRecord* pRec) {

    //=====================
    // Extract the BasicSequence object from the DPVT structure
    //
    devInfoStruct*  pDevInfo  = static_cast<devInfoStruct*>(pRec->dpvt);
    Sequence*       pSequence = pDevInfo->pSequence;

    //=====================
    // Re-compute the slope (in case EGUF has changed)
    // If EGUF is 0, it means we want event clock ticks.
    //
    if (0.0 == pRec->eguf) pRec->eslo = 1.0;
    else  pRec->eslo = pSequence->GetSecsPerTick() / pRec->eguf;

    //=====================
    // Lock access to the sequence while we try to update the maximum timestamp value
    //
    pSequence->lock();
    pRec->pact = false;

    //=====================
    // Convert the VAL field to "Event Clock Ticks" and write
    // it to the BasicSequence object.
    //
    SequenceStatus status = pSequence->SetMaxTimeStamp (pRec->val / pRec->eslo);

    //=====================
    // Switch on the returned Sequence status
    //
    switch (status) {

        //=====================
        // Put the record in an alarm state if we tried to set it to
        // an invalid value.
        //
        case SeqStat_Error:
            recGblSetSevr ((dbCommon *)pRec, WRITE_ALARM, INVALID_ALARM);
            break;

        //=====================
        // If the Sequence is currently updating, schedule an asynchronous completion
        // to postpone the write until after the update completes.
        //
        case SeqStat_Busy:
            pRec->pact = true;
            break;

        //=====================
        // If the value was written, set the readback value and
        // do a standard synchronous completion.
        //
        case SeqStat_Start:
        case SeqStat_Idle:
            pRec->udf = false;
            pRec->rbv = (epicsInt32)pSequence->GetMaxTimeStamp();

    }//end switch on Sequence status

    //=====================
    // Unlock the Sequence object and return
    //
    pSequence->unlock();
    return OK;

}//end EgSeqAoWrite()

/**************************************************************************************************/
/*                    Device Support for Basic Sequence Binary Output Records                     */
/*                                                                                                */


//**************************************************************************************************
//  EgSeqBoInitRecord () -- Initialize A Binary Output Record
//**************************************************************************************************
//! @par Description:
//!   This routine is called during IOC initialization to initialize a Sequence
//!   binary output (bo) record. The bo record is used to trigger a Sequence update.
//!
//!   @par
//!   To use this routine, place its address in the "init_record" field of the bo DSET.
//!
//! @par Function:
//!   - Parse the OUT field.
//!   - Register the record as this Sequence's "Update Trigger" record.
//!
//! @par Implemented Function Codes:
//! - <b> Update Trigger </b> Causes the Sequence object to be immediately updated.
//!
//! @param      pRec  = (input) Pointer to the record we wish to initialize
//!
//! @return     Returns OK if the record was successfully initialized.<br>
//!             Returns a failure code if the record could not be initialized.
//!
//! @note
//!   This routine expects an "augmented" bo device support entry table (DSET) which contains
//!   an entry for a sequence declaration function.
//!
//**************************************************************************************************

epicsStatus EgSeqBoInitRecord (boRecord *pRec)
{
    //=====================
    // Local variables
    //
    SeqBinaryDSET*       pDset;              // Pointer to record's DSET
    std::string          Function;           // Record function name (from OUT link)
    devInfoStruct*       pDevInfo = NULL;    // Pointer to device-specific information structure.
    epicsStatus          status;             // Anticipated error status

    //=====================
    // Try to initialize the record
    //
    try {

        //=====================
        // Make sure the record's DSET is valid
        //
        status = S_dev_missingSup;
        pDset = (SeqBinaryDSET *)pRec->dset;
        if ((pDset->number < DSET_SEQ_BINARY_NUM) || (NULL == pDset->declare_sequence))
            throw std::runtime_error("'DeclareSequence' routine missing from DSET");

        //=====================
        // Parse the OUT link
        //
        status = S_dev_badOutType;
        pDevInfo = parseLink (pRec->out, pDset->declare_sequence, Function);

        //=====================
        // Make sure the function code is correct
        //
        if ("Update Trigger" != Function)
            throw std::runtime_error("Invalid record function (" + Function + ")");

        //=====================
        // Register the "Update Trigger" record
        //
        status = S_dev_Conflict;
        pDevInfo->pSequence->RegisterUpdateTriggerRecord ((dbCommon *)pRec);

        //=====================
        // Record is successfully initialized
        // Set its initial state to zero.
        //
        pRec->dpvt = (void *)pDevInfo;
        pRec->val  = 0;
        pRec->rval = 0;
        pRec->udf  = false;
        pRec->stat = epicsAlarmNone;
        pRec->sevr = pRec->zsv;
        recGblGetTimeStamp((dbCommon *)pRec);
        return OK;

    }//end try

    //=====================
    // If record initialization failed, report the error,
    // disable the record, and return the error status code
    //
    catch (std::exception& e) {
        recGblRecordError (status, pRec, (std::string("\nReason: ") + e.what()).c_str());
        mrfDisableRecord ((dbCommon *)pRec);
        if (NULL != pDevInfo) delete pDevInfo;
        return (status);
    }//end if record initialization failed

}//end EgSeqBoInitRecord()

//**************************************************************************************************
//  EgSeqBoWrite () -- Start a Sequence Update
//**************************************************************************************************
//! @par Description:
//!   This routine is called from the boRecord's "process()" routine to start a Sequence
//!   update. If the Sequence is already executing a previous update, the current request will
//!   be delayed until the current update has completed.  Either way, the write operation does
//!   not complete until the sequence has been updated.
//!
//!   @par
//!   To use this routine, place its address in the "perform_io" field of the bo DSET.
//!
//! @par Implemented Function Codes:
//! - <b> Update Trigger </b> Causes the Sequence object to be immediately updated.
//!
//! @param      pRec  = (input) Pointer to the bo record.
//!
//! @return     Always returns OK.
//!
//**************************************************************************************************

epicsStatus EgSeqBoWrite (boRecord* pRec) {

    //=====================
    // Local variables
    //
    devInfoStruct*  pDevInfo;   // Pointer to device information structure
    Sequence*       pSequence;  // Pointer to Sequence object
    SequenceStatus  status;     // Sequence status variable
    
    //=====================
    // Extract the Sequence object from the DPVT structure
    //
    pDevInfo  = static_cast<devInfoStruct*>(pRec->dpvt);
    pSequence = pDevInfo->pSequence;
    printf ("Update Trigger. VAL = %d, PACT=%d, Waiting = %d\n",
            pRec->val, pRec->pact, pDevInfo->waiting); /*~~~~*/
     
    //=====================
    // If this is an asynchronous completion callback, and we are not waiting to start another
    // update, perform an asynchronous completion without starting another update.
    //
    if (pRec->pact && !pDevInfo->waiting) {
        pRec->pact = false;
        pRec->val  = 0;
        pRec->rval = 0;
        return OK;
    }//end if asynchronous completion & we started the update

    //=====================
    // If someone just wrote a 0 to the record (i.e this is not a callback and we aren't
    // waiting to start an update), just exit without trying to start an update.
    //
    if (!pRec->pact && !pDevInfo->waiting && (0 == pRec->val)) {
        pRec->val  = pRec->mlst;
        pRec->rval = (epicsUInt32)pRec->val;
        return OK;
    }//end if VAL is 0

    //=====================
    // All other cases, lock the Sequence object and try to start an update
    //
    pRec->pact = true;
    pDevInfo->waiting = false;

    pSequence->lock();
    status = pSequence->StartUpdate();

    //=====================
    // Switch on the returned Sequence status
    //
    switch (status) {

        //=====================
        // We started a Sequence update
        // Now wait for asynchronous completion
        //
        case SeqStat_Start:
            break;

        //=====================
        // An update was already in progress
        // Wait for asynchronous completion, then try again.
        //
        case SeqStat_Busy:
            pDevInfo->waiting = true;
            break;

        //=====================
        // All other cases:
        // An error occured trying to start the update
        // Signal a write alarm condition and set the record to its "Idle" state
        //
        default:
            recGblSetSevr ((dbCommon *)pRec, WRITE_ALARM, INVALID_ALARM);
            pRec->pact = false;
            pRec->val  = 0;
            pRec->rval = 0;
            break;

    }//end switch on Sequence status

    //=====================
    // Unlock the Sequence object and return
    //
    pSequence->unlock();
    return OK;

}//end EgSeqBoWrite()

/**************************************************************************************************/
/*               Device Support for Basic Sequence Multi-Bit Binary Output Records                */
/*                                                                                                */


//**************************************************************************************************
//  EgSeqMbboInitRecord () -- Initialize A Multi-Bit Binary Output Record
//**************************************************************************************************
//! @par Description:
//!   This routine is called during IOC initialization to initialize a sequence
//!   multi-bit binary output (mbbo) record. A sequence mbbo record is used to set the
//!   sequence's update mode.
//!
//!   @par
//!   To use this routine, place its address in the "init_record" field of the mbbo DSET.
//!
//! @par Function:
//!   - Parse the OUT field.
//!   - Register the record as this sequence's "Update Mode" record.
//!
//! @par Implemented Function Codes:
//! - <b> Update Mode </b>  Determines whether sequence updates should occur immediately upon
//!                         the update of any of the sequence's event records or whether updates
//!                         should accumulate until an "Update Trigger" reccord is activated.
//!
//! @param      pRec  = (input) Pointer to the record we wish to initialize
//!
//! @return     Returns OK if the record was successfully initialized.<br>
//!             Returns a failure code if the record could not be initialized.
//!
//! @note
//!   This routine expects an "augmented" mbbo device support entry table (DSET) which contains
//!   an entry for a sequence declaration function.
//!
//**************************************************************************************************

epicsStatus EgSeqMbboInitRecord (mbboRecord *pRec)
{
    //=====================
    // Local variables
    //
    SeqMbbDSET*          pDset;              // Pointer to record's DSET
    std::string          Function;           // Record function name (from OUT link)
    devInfoStruct*       pDevInfo = NULL;    // Pointer to device-specific information structure.
    epicsStatus          status;             // Anticipated error status

    //=====================
    // Try to initialize the record
    //
    try {

        //=====================
        // Make sure the record's DSET is valid
        //
        status = S_dev_missingSup;
        pDset = (SeqMbbDSET *)pRec->dset;
        if ((pDset->number < DSET_SEQ_MBB_NUM) || (NULL == pDset->declare_sequence))
            throw std::runtime_error("'DeclareSequence' routine missing from DSET");

        //=====================
        // Parse the OUT link
        //
        status = S_dev_badOutType;
        pDevInfo = parseLink (pRec->out, pDset->declare_sequence, Function);

        //=====================
        // Make sure the function code is correct
        //
        if ("Update Mode" != Function)
            throw std::runtime_error("Invalid record function (" + Function + ")");

        //=====================
        // Register the "Update Mode" record
        //
        status = S_dev_Conflict;
        pDevInfo->pSequence->RegisterUpdateModeRecord ((dbCommon *)pRec);

        //=====================
        // Record is successfully initialized
        //
        pRec->dpvt = (void *)pDevInfo;
        pRec->rval = (epicsUInt32)pRec->val;
        return OK;

    }//end try

    //=====================
    // If record initialization failed, report the error,
    // disable the record, and return the error status code
    //
    catch (std::exception& e) {
        recGblRecordError (status, pRec, (std::string("\nReason: ") + e.what()).c_str());
        mrfDisableRecord ((dbCommon *)pRec);
        if (NULL != pDevInfo) delete pDevInfo;
        return (status);
    }//end if record initialization failed

}//end EgSeqMbboInitRecord()

//**************************************************************************************************
//  EgSeqMbboWrite () -- Set The Update Mode On A Sequence Object
//**************************************************************************************************
//! @par Description:
//!   This routine is called from the mbboRecord's "process()" routine to change the Sequence
//!   update mode. To avoid confusion, the update mode will not be changed while a Sequence object
//!   is actively updating.  If the write occurs during a Sequence update, it will be deferred
//!   (via an asynchronous completion) until the update completes. Changing the update mode does
//!   not cause the Sequence object to start an update.
//!
//!   @par
//!   To use this routine, place its address in the "perform_io" field of the mbbo DSET.
//!
//! @par Implemented Function Codes:
//! - <b> Update Mode </b>  Determines whether sequence updates should occur immediately upon
//!                         the update of any of the sequence's event records or whether updates
//!                         should accumulate until an "Update Trigger" reccord is activated.
//!
//! @param      pRec  = (input) Pointer to the mbbo record.
//!
//! @return     Always returns OK.
//!
//**************************************************************************************************

epicsStatus EgSeqMbboWrite (mbboRecord* pRec) {

    //=====================
    // Local variables
    //
    devInfoStruct*  pDevInfo;   // Pointer to device information structure
    Sequence*       pSequence;  // Pointer to Sequence object
    SequenceStatus  status;     // Sequence status variable
    
    //=====================
    // Extract the Sequence object from the dpvt structure
    //
    pDevInfo  = static_cast<devInfoStruct*>(pRec->dpvt);
    pSequence = pDevInfo->pSequence;
     
    //=====================
    // Lock the Sequence object while we set its update mode
    //
    pRec->pact = false;
    pSequence->lock();
    status = pSequence->SetUpdateMode((SequenceUpdateMode)pRec->val);

    //=====================
    // Switch on the returned Sequence status
    //
    switch (status) {

        //=====================
        // Put the record in an alarm state if we tried to set it to
        // an invalid state.
        //
        case SeqStat_Error:
            recGblSetSevr ((dbCommon *)pRec, STATE_ALARM, INVALID_ALARM);
            break;

        //=====================
        // If the Sequence is currently updating, schedule an asynchronous completion
        // to postpone the write until after the update completes.
        //
        case SeqStat_Busy:
            pRec->pact = true;
            break;

        //=====================
        // If the Sequence was not updating, then the write completed.
        // Perform a synchronous completion.
        //
        default:
            pRec->pact = false;
            pRec->udf = false;

    }//end switch on Sequence status

    //=====================
    // Unlock the Sequence object and return
    //
    pSequence->unlock();
    return OK;

}//end EgSeqMbboWrite()


//!
//| @}
//end group Sequencer
