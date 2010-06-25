/**************************************************************************************************
|* $(MRF)/evgBasicSequenceApp/src/devBasicSequenceEvent.cpp
|* EPICS Device Support for Basic Sequence Events
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
/*  devBasicSequenceEvent File Description                                                        */
/**************************************************************************************************/

//==================================================================================================
//! @addtogroup Sequencer
//! @{
//!
//==================================================================================================
//! @file       devBasicSequenceEvent.cpp
//! @brief      EPICS Device Suppport for Basic Sequence Events.
//!
//! @par Description:
//!   This file contains EPICS device support for event generator sequence events.
//!   A sequence event can be represented in the EPICS data base by two required records and
//!   up to three more optional records.  The records are distinguished by function codes specified
//!   in their I/O link fields (see below).  Basic Sequence Event records, may have the following
//!   function codes:
//!   - <b> Event Code </b> (longout, required) Which event code to transmit.
//!   - <b> Time </b>       (ao, required) Specifies when in the sequence the event
//!                         should be transmitted.
//!   - <b> Time </b>       (ai, optional) Displays the actual timestamp assigned to the event.
//!                         This could differ from the requested timestamp because of event clock
//!                         quantization or because another event was assigned to that time.
//!   - <b> Enable </b>     (bo, optional) Enables or disables event transmission.
//!   - <b> Priority </b>   (longout, optional) When two sequence events end up with the same
//!                         timestamp, the relative priorities will determine which one gets
//!                         "jostled".
//!   @par
//!   Every event in a sequence must have a unique name associated with it -- even if the
//!   event code is duplicated.  The unique name is defined in the I/O link (see below) and
//!   assigned when the BasicSequenceEvent object is created.
//!
//! @par Device Type Field:
//!   A basic sequence event record will have the "Device Type" field (DTYP) set to:<br>
//!      <b> "EVG Basic Seq Event" </b>
//!
//! @par Link Format:
//!   Basic sequence event records use the INST_IO I/O link format. The INP or OUT links have the
//!   following format:<br>
//!      <b> \@Name=\<name\>; C=n; Seq=m; Fn=\<function\> </b>
//!
//!   @par
//!   Where:
//!   - \b Name  = The sequence event name (note that the name may contain blanks).
//!   - \b C     = Logical card number for the event generator card this sequence event belongs to.
//!   - \b Seq   = Specifies the ID number of the sequence that this event belongs to
//!   - \b Fn    = The record's function (see list above)
//!
//==================================================================================================


/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <stdexcept>           // Standard C++ exception definitions
#include  <string>              // Standard C++ string class

#include  <cstdlib>             // Standard C library
#include  <cstring>             // Standard C string library

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions
#include  <alarm.h>             // EPICS Alarm status and severity definitions
#include  <dbAccess.h>          // EPICS Database access messages and definitions
#include  <devSup.h>            // EPICS Device support messages and definitions
#include  <iocsh.h>             // EPICS IOC shell support library
#include  <link.h>              // EPICS Database link definitions
#include  <recGbl.h>            // EPICS Global record support routines

#include  <aiRecord.h>          // EPICS Analog input record definition
#include  <aoRecord.h>          // EPICS Analog output record definition
#include  <boRecord.h>          // EPICS Binary output record definition
#include  <longoutRecord.h>     // EPICS Long output record definition
#include  <menuConvert.h>       // EPICS conversion field menu items

#include  <mrfCommon.h>         // MRF Common definitions
#include  <mrfIoLink.h>         // MRF I/O link field parser
#include  <BasicSequence.h>     // MRF Basic Sequence class
#include  <BasicSequenceEvent.h>// MRF Basic Sequence Event class
#include  <drvSequence.h>       // MRF Generic Sequence driver support declarations
#include  <drvEvg.h>            // MRF Event Generator driver infrastructure routines
#include  <devBasicSequence.h>  // MRF EPICS device support for the Basic Sequence class

#include  <epicsExport.h>       // EPICS Symbol exporting macro definitions

/**************************************************************************************************/
/*  Structure And Type Definitions                                                                */
/**************************************************************************************************/

//=====================
// Common I/O link parameter definitions used by all BasicSequenceEvent records
//
static const
mrfParmNameList SeqEventParmNames = {
    "C",        // Logical card number
    "Name",     // Event name
    "Fn",       // Record function code
    "Seq"       // Sequence number that this event belongs to
};//end parameter name table

static const
epicsInt32  SeqEventNumParms mrfParmNameListSize(SeqEventParmNames);

//=====================
// Generic BasicSequenceEvent setter function type definition
//
typedef  epicsStatus (BasicSequenceEvent::*SetFunction) (epicsInt32);

//=====================
// Common device information structure used by all BasicSequenceEvent records
//
struct devInfoStruct {
    BasicSequence*       pSequence;       // Pointer to the BasicSequence object
    BasicSequenceEvent*  pEvent;          // Pointer to the BasicSequenceEvent object
    SetFunction          Set;             // Setter Function
    epicsFloat64         LastActualTime;  // Used by TimeStamp record
    bool                 ValueSet;        // Lets asynchronous completion know if the value was set
};//end devInfoStruct

/**************************************************************************************************/
/*                                Common Utility Routines                                         */
/*                                                                                                */


/**************************************************************************************************
|* parseLink () -- Parse An EPICS Database Link Structure
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o Parse the record's EPICS Database input (INP) or output (OUT) field.
|*      The following sanity checks are performed on the link field:
|*       - Make sure the link type is INST_IO
|*       - Make sure the parameter field is not empty
|*       - Make sure the record function field is not empty
|*       - Make sure the event name field is not empty.
|*
|*    o Lookup the BasicSequenceEvent and BasicSequence objects that belong to this record.
|*      If these objects do not already exist, they will be created.
|*
|*    o Create a device information structure and initialize it with pointers to the
|*      BasicSequenceEvent object and the BasicSequenceEvent object associated with
|*      this record.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    pDevInfo = parseLink (dbLink, Function);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    dbLink   = (DBLINK &)     Reference to the record's INP or OUT link field.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS:
|*    Function = (string &)     The record function string
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
    std::string&         Function)           // Reference to returned function name string
{
    //=====================
    // Local variables
    //
    epicsInt32           Card;               // Logical EVG card number
    std::string          Name;               // Sequence event name
    epicsInt32           SeqNum;             // Sequence ID number for this event
    mrfIoLink*           ioLink = NULL;      // I/O link parsing object
    devInfoStruct*       pDevInfo = NULL;    // Pointer to device-specific information structure.
    BasicSequenceEvent*  pEvent;             // Pointer to our BasicSequenceEvent object
    BasicSequence*       pSequence;          // Pointer to the BasicSequence object for this event

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
        ioLink   = new mrfIoLink (dbLink.value.instio.string, SeqEventParmNames, SeqEventNumParms);
        Name     = ioLink->getString  ("Name");
        Function = ioLink->getString  ("Fn"  );
        Card     = ioLink->getInteger ("C"   );
        SeqNum   = ioLink->getInteger ("Seq" );
        delete ioLink;
        ioLink = NULL;

        //=====================
        // Declare the sequence number and the event name
        //
        pSequence = EgDeclareBasicSequence (Card, SeqNum);
        pEvent = pSequence->DeclareEvent (Name);

        //=====================
        // Create and initialize the device information structure
        //
        pDevInfo = new devInfoStruct;
        pDevInfo->pSequence = pSequence;
        pDevInfo->pEvent = pEvent;

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
/*                 Device Support for Basic Sequence Event Analog Input Records                   */
/*                                                                                                */


/**************************************************************************************************/
/*  Analog Input Device Support Routines                                                          */
/**************************************************************************************************/

static epicsStatus aiInitRecord (aiRecord* pRec);
static epicsStatus aiRead       (aiRecord* pRec);
static epicsStatus aiLinConv    (aiRecord* pRec, bool AfterChange);


/**************************************************************************************************/
/*  Device Support Entry Table (DSET) For Analog Input Records                                    */
/**************************************************************************************************/

extern "C" {
static
AnalogDSET devAiBasicSeqEvent = {
    DSET_ANALOG_NUM,                    // Number of entries in the table
    NULL,                               // -- No device report routine
    NULL,                               // -- No device support initialization routine
    (DEVSUPFUN)aiInitRecord,            // Record initialization routine
    NULL,                               // -- No get I/O interrupt information routine
    (DEVSUPFUN)aiRead,                  // Read routine
    (DEVSUPFUN)aiLinConv                // Special linear conversion routine
};

epicsExportAddress (dset, devAiBasicSeqEvent);

};//end extern "C"

/**************************************************************************************************
|* aiInitRecord () -- Initialize an Analog Input Record
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called during IOC initialization to initialize a BasicSequenceEvent
|*    analog input record.  It performs the following functions:
|*      - Parse the INP field.
|*      - Initialize the LINR and ESLO fields.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTION CODES:
|*    Time  = Reads the actual time this event is scheduled to occur.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = aiInitRecord (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (aiRecord *)     Address of the ai record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    NO_CONVERT:  Successful initialization, do not perform
|*                                           value conversion
|*                              Other        Failure code
|*
\**************************************************************************************************/

static
epicsStatus aiInitRecord (aiRecord *pRec)
{
    //=====================
    // Local variables
    //
    std::string          Function;           // Record function name (from INP link)
    devInfoStruct*       pDevInfo = NULL;    // Pointer to device-specific information structure.
    epicsStatus          status;             // Anticipated error status

    //=====================
    // Try to initialize the record
    //
    try {

        //=====================
        // Parse the INP link
        //
        status = S_dev_badInpType;
        pDevInfo = parseLink (pRec->inp, Function);

        //=====================
        // Make sure the function code is correct
        //
        if ("Time" != Function)
            throw std::runtime_error("Invalid record function (" + Function + ")");

        //=====================
        // Finish record initialization.
        //
        pRec->dpvt = (void *)pDevInfo;
        aiLinConv (pRec, true);
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

}//end aiInitRecord()

/**************************************************************************************************
|* aiRead () -- Read the Timestamp from the BasicSequenceEvent Object.
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called from the aiRecord's "process()" routine.
|*
|*    It reads the assigned timestamp for this event (in ticks) from the BasicSequenceEvent object,
|*    converts the timestamp into engineering units, and stores it in the record's VAL field.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTION CODES:
|*    Time  = Reads the actual time this event is scheduled to occur.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = aiRead (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (aiRecord *)     Address of the ai record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    Always returns NO_CONVERT.
|*
\**************************************************************************************************/

static
epicsStatus aiRead (aiRecord* pRec) {

    //=====================
    // Extract the BasicSequenceEvent object from the DPVT structure
    //
    devInfoStruct*       pDevInfo  = static_cast<devInfoStruct*>(pRec->dpvt);
    BasicSequenceEvent*  pEvent    = pDevInfo->pEvent;

    //=====================
    // Get the actual time value (in ticks) from the BasicSequenceEvent object and
    // convert to the units of the ai record.
    //
    pRec->val = pEvent->GetActualTime() * pRec->eslo;
    pRec->udf = 0;

    //=====================
    // If the timestamp was set to an illegal value, put the record into an alarm state.
    //
    if (!pEvent->TimeStampLegal())
        recGblSetSevr ((dbCommon *)pRec, READ_ALARM, INVALID_ALARM);

    //=====================
    // Don't use the ai record's conversion
    //
    return NO_CONVERT;

}//end aiRead()

/**************************************************************************************************
|* aiLinConv () -- Special Linear Conversion Routine for TimeStamp Record
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called by EPICS database processing whenever the LINR, EGUF, or EGUL
|*    fields are changed.
|*
|*    It computes a new value for the ESLO field based on the value of the EGUF field and
|*    the time (in seconds) between event clock ticks, which it gets from the Sequence object.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTION CODES:
|*    Time  = Reads the actual time this event is scheduled to occur.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = aiLinConv (pRec, AfterChange);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec        = (aiRecord *) Address of the ai record structure
|*    AfterChange = (bool)       True if this is the second call to the special linear conversion
|*                               routine (after the new value has been written)
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUT:
|*    pRec->eguf  = (double)      New value for EGUF
|*                                EGUF = 0:  No conversion (units are event clock ticks)
|*                                EGUF > 0:  EGUF represents the number of seconds per
|*                                           engineering unit.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT OUTPUTS:
|*    pRec->eslo  = (double)      New computed slope.  ESLO represents the smallest engineering
|*                                unit of change for this record.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status      = (epicsStatus) Always returns OK
|*
\**************************************************************************************************/

static
epicsStatus aiLinConv (aiRecord *pRec, bool AfterChange)
{
    //=====================
    // Ignore this call if the new value hasn't been changed yet.
    //
    if (!AfterChange) return OK;

    //=====================
    // If EGUF is greater than zero, it represents seconds per engineering unit.
    //
    if (pRec->eguf > 0.0) {
        devInfoStruct* pDevInfo  = static_cast<devInfoStruct*>(pRec->dpvt);
        pRec->eslo = pDevInfo->pSequence->GetSecsPerTick() / pRec->eguf;
    }//end if EGUF is greater than 0

    //=====================
    // If EGUF is 0 or negative, it means we want event clock ticks
    //
    else pRec->eslo = 1.0;

    return OK;

}//end aiLinConv()

/**************************************************************************************************/
/*                 Device Support for Basic Sequence Event Analog Output Records                  */
/*                                                                                                */


/**************************************************************************************************/
/*  Analog Output Device Support Routines                                                         */
/**************************************************************************************************/

static epicsStatus aoInitRecord (aoRecord* pRec);
static epicsStatus aoWrite      (aoRecord* pRec);
static epicsStatus aoLinConv    (aoRecord* pRec, bool AfterChange);

/**************************************************************************************************/
/*  Device Support Entry Table (DSET) For Analog Output Records                                   */
/**************************************************************************************************/

extern "C" {
static
AnalogDSET devAoBasicSeqEvent = {
    DSET_ANALOG_NUM,                    // Number of entries in the table
    NULL,                               // -- No device report routine
    NULL,                               // -- No device support initialization routine
    (DEVSUPFUN)aoInitRecord,            // Record initialization routine
    NULL,                               // -- No get I/O interrupt information routine
    (DEVSUPFUN)aoWrite,                 // Write routine
    (DEVSUPFUN)aoLinConv                // Special linear conversion routine
};

epicsExportAddress (dset, devAoBasicSeqEvent);

};//end extern "C"

/**************************************************************************************************
|* aoInitRecord () -- Initialize an Analog Output Record
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called during IOC initialization to initialize a BasicSequenceEvent
|*    analog output record.  It performs the following functions:
|*      - Parse the OUT field.
|*      - Register the record as this event's timestamp record.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTION CODES:
|*    Time  = Sets the requested time for this event to occur.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = aoInitRecord (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (aoRecord *)     Address of the ao record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    NO_CONVERT:  Successful initialization, do not perform
|*                                           value conversion
|*                              Other        Failure code
|*
\**************************************************************************************************/

static
epicsStatus aoInitRecord (aoRecord *pRec)
{
    //=====================
    // Local variables
    //
    std::string          Function;           // Record function name (from OUT link)
    devInfoStruct*       pDevInfo = NULL;    // Pointer to device-specific information structure.
    epicsStatus          status;             // Anticipated error status

    //=====================
    // Try to initialize the record
    //
    try {

        //=====================
        // Parse the OUT link
        //
        status = S_dev_badOutType;
        pDevInfo = parseLink (pRec->out, Function);

        //=====================
        // Make sure the function code is correct
        //
        if ("Time" != Function)
            throw std::runtime_error("Invalid record function (" + Function + ")");

        //=====================
        // Register the timestamp record
        //
        status = S_dev_Conflict;
        pDevInfo->pEvent->RegisterTimeRecord ((dbCommon *)pRec);

        //=====================
        // Finish initializing the device information structure and return
        // Do not attempt a raw value conversion
        //
        pDevInfo->LastActualTime = -1.0;
        pDevInfo->ValueSet = false;
        pRec->dpvt = (void *)pDevInfo;
        aoLinConv (pRec, true);
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

}//end aoInitRecord()

/**************************************************************************************************
|* aoWrite () -- Write the Timestamp to the BasicSequenceEvent Object.
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called from the aoRecord's "process()" routine.
|*
|*    It converts the timestamp in the VAL field from engineering units to event clock ticks
|*    and writes it to the BasicSequenceEvent object.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTION CODES:
|*    Time  = Sets the requested time for this event to occur.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = aoWrite (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (aoRecord *)     Address of the ao record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    Always returns OK
|*
\**************************************************************************************************/

static
epicsStatus aoWrite (aoRecord* pRec) {

    //=====================
    // Extract the BasicSequence and BasicSequenceEvent objects from the DPVT structure
    //
    devInfoStruct*       pDevInfo  = static_cast<devInfoStruct*>(pRec->dpvt);
    BasicSequence*       pSequence = pDevInfo->pSequence;
    BasicSequenceEvent*  pEvent    = pDevInfo->pEvent;

    //=====================
    // Check to see if this is an asynchronous completion
    //
    if (pRec->pact) {
        pRec->pact = false;

        //=====================
        // If we've already written the new value,
        // complete the asynchronous write operation now.
        //
        if (pDevInfo->ValueSet) {
            if (!pEvent->TimeStampLegal())
                recGblSetSevr ((dbCommon *)pRec, WRITE_ALARM, INVALID_ALARM);

            pRec->udf = false;
            return OK;
        }//end if we don't need to do a delayed write

    }//end if this is an asynchronous completion

    //==============================================================================================
    // If we get here, then this is either the start of a new write, or it is the
    // the asynchronous completion of a write that was not able to set the new value
    // because the Sequence was busy updating. In either case, we will treat it like a
    // new write.
    //==============================================================================================

    //=====================
    // Lock access to the sequence while we try to update this event's timestamp
    //
    pSequence->lock();

    //=====================
    // Convert the VAL field to "Event Clock Ticks" and write it to the BasicSequenceEvent object.
    //
    pDevInfo->ValueSet = true;
    SequenceStatus status = pEvent->SetEventTime (pRec->val / pRec->eslo);

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
            pDevInfo->ValueSet = false;
            break;

        //=====================
        // If the value was written and it started a sequence update,
        // do a standard asynchronous completion.
        //
        case SeqStat_Start:
            pRec->pact = true;
            break;

        //=====================
        // If the value was written but did not start a sequence update,
        // make this a synchronous completion (this will happen if the
        // Sequence update mode is set to "On Trigger").
        //
        case SeqStat_Idle:
            pRec->udf = false;

    }//end switch on Sequence status

    //=====================
    // Unlock the Sequence object and return
    //
    pSequence->unlock();
    return OK;

}//end aoWrite()

/**************************************************************************************************
|* aoLinConv () -- Special Linear Conversion Routine for TimeStamp Record
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called by EPICS database processing whenever the LINR, EGUF, or EGUL
|*    fields are changed.
|*
|*    It computes a new value for the ESLO field based on the value of the EGUF field and
|*    the time (in seconds) between event clock ticks, which it gets from the Sequence object.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTION CODES:
|*    Time  = Sets the requested time for this event to occur.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = aoLinConv (pRec, AfterChange);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec        = (aoRecord *) Address of the ao record structure
|*    AfterChange = (bool)       True if this is the second call to the special linear conversion
|*                               routine (after the new value has been written)
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT INPUT:
|*    pRec->eguf  = (double)      New value for EGUF
|*                                EGUF = 0:  No conversion (units are event clock ticks)
|*                                EGUF > 0:  EGUF represents the number of seconds per
|*                                           engineering unit.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLICIT OUTPUTS:
|*    pRec->eslo  = (double)      New computed slope.  ESLO represents the smallest engineering
|*                                unit of change for this record.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status      = (epicsStatus) Always returns OK
|*
\**************************************************************************************************/

static
epicsStatus aoLinConv (aoRecord *pRec, bool AfterChange)
{
    //=====================
    // Ignore this call if the new value hasn't been changed yet.
    //
    if (!AfterChange) return OK;

    //=====================
    // If EGUF is greater than zero, it represents seconds per engineering unit.
    //
    if (pRec->eguf > 0.0) {
        devInfoStruct* pDevInfo  = static_cast<devInfoStruct*>(pRec->dpvt);
        pRec->eslo = pDevInfo->pSequence->GetSecsPerTick() / pRec->eguf;
    }//end if EGUF is greater than 0

    //=====================
    // If EGUF is 0 or negative, it means we want event clock ticks
    //
    else pRec->eslo = 1.0;

    return OK;

}//end aoLinConv()

/**************************************************************************************************/
/*                 Device Support for Basic Sequence Event Binary Output Records                  */
/*                                                                                                */


/**************************************************************************************************/
/*  Binary Output Device Support Routines                                                         */
/**************************************************************************************************/

static epicsStatus boInitRecord (boRecord* pRec);
static epicsStatus boWrite      (boRecord* pRec);


/**************************************************************************************************/
/*  Device Support Entry Table (DSET) For Binary Output Records                                   */
/**************************************************************************************************/

extern "C" {
static
BinaryDSET devBoBasicSeqEvent = {
    DSET_BINARY_NUM,                    // Number of entries in the table
    NULL,                               // -- No device report routine
    NULL,                               // -- No device support initialization routine
    (DEVSUPFUN)boInitRecord,            // Record initialization routine
    NULL,                               // -- No get I/O interrupt information routine
    (DEVSUPFUN)boWrite                  // Write routine
};

epicsExportAddress (dset, devBoBasicSeqEvent);

};//end extern "C"

/**************************************************************************************************
|* boInitRecord () -- Initialize a Binary Output Record
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called during IOC initialization to initialize a BasicSequenceEvent
|*    binary output record.  It performs the following functions:
|*      - Parse the OUT field.
|*      - Register the record as this event's enable record.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTION CODES:
|*    Enable  = Enables or disables this event.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = boInitRecord (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (boRecord *)     Address of the bo record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    OK:     Successful initialization.
|*                              Other:  Failure code
|*
\**************************************************************************************************/

static
epicsStatus boInitRecord (boRecord *pRec)
{
    //=====================
    // Local variables
    //
    std::string          Function;           // Record function name (from OUT link)
    devInfoStruct*       pDevInfo = NULL;    // Pointer to device-specific information structure.
    epicsStatus          status;             // Anticipated error status

    //=====================
    // Try to initialize the record
    //
    try {

        //=====================
        // Parse the OUT link
        //
        status = S_dev_badOutType;
        pDevInfo = parseLink (pRec->out, Function);

        //=====================
        // Make sure the function code is correct
        //
        if ("Enable" != Function)
            throw std::runtime_error("Invalid record function (" + Function + ")");

        //=====================
        // Register the enable record
        //
        status = S_dev_Conflict;
        pDevInfo->pEvent->RegisterEnableRecord ((dbCommon *)pRec);

        //=====================
        // Attach the device information structure to the record.
        //
        pRec->dpvt = (void *)pDevInfo;

        //=====================
        // Set RVAL and return success
        //
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

}//end boInitRecord()

/**************************************************************************************************
|* boWrite () -- Write the Enable/Disable State to the BasicSequenceEvent Object.
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called from the boRecord's "process()" routine to enable or disable
|*    the BasicSequenceEvent object.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTION CODES:
|*    Enable  = Enables or disables this event.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = boWrite (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (boRecord *)     Address of the bo record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    Always returns OK
|*
\**************************************************************************************************/

static
epicsStatus boWrite (boRecord* pRec) {

    //=====================
    // Extract the BasicSequenceEvent object from the DPVT structure
    //
    devInfoStruct*       pDevInfo  = static_cast<devInfoStruct*>(pRec->dpvt);
    BasicSequenceEvent*  pEvent    = pDevInfo->pEvent;

    //=====================
    // Convert the VAL field to a boolean and
    //write it to the BasicSequenceEvent object.
    //
    pEvent->SetEventEnable (0 != pRec->val);

    return OK;

}//end boWrite()

/**************************************************************************************************/
/*                  Device Support for Basic Sequence Event Long Output Records                   */
/*                                                                                                */


/**************************************************************************************************/
/*  Long Output Device Support Routines                                                           */
/**************************************************************************************************/

static epicsStatus loInitRecord (longoutRecord* pRec);
static epicsStatus loWrite      (longoutRecord* pRec);


/**************************************************************************************************/
/*  Device Support Entry Table (DSET) For Long Output Records                                     */
/**************************************************************************************************/

extern "C" {
static
LongDSET devLoBasicSeqEvent = {
    DSET_LONG_NUM,                      // Number of entries in the table
    NULL,                               // -- No device report routine
    NULL,                               // -- No device support initialization routine
    (DEVSUPFUN)loInitRecord,            // Record initialization routine
    NULL,                               // -- No get I/O interrupt information routine
    (DEVSUPFUN)loWrite,                 // Write routine
};

epicsExportAddress (dset, devLoBasicSeqEvent);

};//end extern "C"

/**************************************************************************************************
|* loInitRecord () -- Initialize an Long Output Record
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called during IOC initialization to initialize a BasicSequenceEvent
|*    longout record.  Longout records are used to set the event code and the event priority.
|*    This routine performs the following functions:
|*      - Parse the OUT field.
|*      - Register the record as the event code or the priority record for this event.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTION CODES:
|*    Event Code  = Sets the event code for this event.
|*    Priority    = Sets the jostle priority for this event
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = loInitRecord (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (loRecord *)     Address of the lo record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    NO_CONVERT:  Successful initialization, do not perform
|*                                           value conversion
|*                              Other        Failure code
|*
\**************************************************************************************************/

static
epicsStatus loInitRecord (longoutRecord *pRec)
{
    //=====================
    // Local variables
    //
    std::string          Function;           // Record function name (from OUT link)
    devInfoStruct*       pDevInfo = NULL;    // Pointer to device-specific information structure.
    epicsStatus          status;             // Anticipated error status

    //=====================
    // Try to initialize the record
    //
    try {

        //=====================
        // Parse the OUT link
        //
        status = S_dev_badOutType;
        pDevInfo = parseLink (pRec->out, Function);

        //=====================
        // Initialize an Event Code record
        //
        if ("Event Code" == Function) {
            status = S_dev_Conflict;
            pDevInfo->Set = &BasicSequenceEvent::SetEventCode;
            pDevInfo->pEvent->RegisterCodeRecord ((dbCommon *)pRec);
        }//end if this is an event code record

        //=====================
        // Initialize an Event Priority record
        //
        else if ("Priority" == Function) {
            status = S_dev_Conflict;
            pDevInfo->Set = &BasicSequenceEvent::SetEventPriority;
            pDevInfo->pEvent->RegisterPriorityRecord ((dbCommon *)pRec);
        }//end if this is an event code record

        //=====================
        // Throw an error if we did not recognize the function code.
        //
        else
            throw std::runtime_error("Invalid record function (" + Function + ")");

        //=====================
        // Record is successfully initialized.
        //
        pRec->dpvt = (void *)pDevInfo;
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

}//end loInitRecord()

/**************************************************************************************************
|* loWrite () -- Write the Timestamp to the BasicSequenceEvent Object.
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    This routine is called from the longoutRecord's "process()" routine.
|*
|*    It invokes the appropriate BasicSequenceEvent "setter" routine (as determined from the
|*    information in the device private structure).  If the setter routine returns an error,
|*    the record is placed in "WRITE_ALARM" state.
|*
|*-------------------------------------------------------------------------------------------------
|* IMPLEMENTED FUNCTION CODES:
|*    Event Code  = Sets the event code for this event.
|*    Priority    = Sets the jostle priority for this event
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    status = loWrite (pRec);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec   = (longoutRecord *)     Address of the longout record structure
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*    status = (epicsStatus)    Always returns OK
|*
\**************************************************************************************************/

static
epicsStatus loWrite (longoutRecord* pRec) {

    //=====================
    // Local variables
    //
    devInfoStruct*       pDevInfo;      // Device private information structure
    BasicSequenceEvent*  pEvent;        // BasicSequenceEvent object
    epicsStatus          status;        // Local status variable

    //=====================
    // Extract the SequenceEvent object from the DPVT structure
    //
    pDevInfo  = static_cast<devInfoStruct*>(pRec->dpvt);
    pEvent    = pDevInfo->pEvent;

    //=====================
    // Invoke the appropriate BasicSequenceEvent setter routine
    //
    status = (pEvent->*(pDevInfo->Set)) (pRec->val);

    //=====================
    // If the write failed, put the record in WRITE_ALARM state
    //
    if (OK != status) {
        recGblSetSevr ((dbCommon *)pRec, WRITE_ALARM, INVALID_ALARM);
    }//end if write failed

    //=====================
    // Return the status code
    //
    return status;

}//end loWrite()


//!
//! @}
//end group Sequencer
