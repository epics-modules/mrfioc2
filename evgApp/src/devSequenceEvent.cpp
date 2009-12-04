/**************************************************************************************************
|* $(TIMING)/evgApp/src/devSequenceEvent.cpp -- EPICS Device Support for EVG Sequence Events
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
|*    This module contains EPICS device support for event generator Sequence Event objects.
|*
|*-------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator Cards
|*     Modular Register Mask
|*     APS Register Mask
|*
|*-------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   vxWorks
|*   RTEMS
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
//! @file       devSequenceEvent.cpp
//! @brief      EPICS Device Suppport for Sequence Events.
//!
//! @par Description:
//!   This file contains EPICS device support for event generator sequence events.
//!   A sequence event can be represented in the EPICS data base by two required records and
//!   up to three more optional records:
//!   - \b EVENT_CODE     (longout, required) Which event code to transmit.
//!   - \b EVENT_TIME     (ao, required) Specifies when in the sequence the event
//!                       should be transmitted.
//!   - \b EVENT_TIME     (ai, optional) Displays the actual timestamp assigned to the event.
//!                       This could differ from the requested timestamp because of event clock
//!                       quantization or because another event was assigned to that time.
//!   - \b EVENT_ENABLE   (bo, optional) Enables or disables event transmission.
//!   - \b EVENT_PRIORITY (longout, optional) When two sequence events end up with the same
//!                       timestamp, the relative priorities will determine which one gets
//!                       "jostled".
//!   @par
//!   Every event in a sequence must have a unique name associated with it -- even if the
//!   event code is duplicated.  The unique name is assigned when the SequenceEvent object is
//!   created.
//!
//! @par Device Type
//!   A sequence event record will have the "Device Type" field (DTYP) set to:
//!   @verbatim
//!      "EVG Sequence Event"
//!   @endverbatim
//!
//! @par Link Format
//!   For sequence event records, the INP or OUT link has the following format:
//!   @verbatim
//!      #C0 Ss @FUNCTION Name
//!   @endverbatim
//!   Where:
//!   - \b C0         = Not used
//!   - \b Ss         = Specifies the ID number of the sequence that this event belongs to
//!   - \b \@FUNCTION = The record's function name (see list above)
//!   - \b Name       = The sequence event name.
//!   @par
//!   Note that the sequence event name may contain blanks.
//!
//==================================================================================================


/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include  <stdexcept>           // Standard C++ exception definitions
#include  <string>              // Standard C++ string class
#include  <cstdlib>

#include  <epicsTypes.h>        // EPICS Architecture-independent type definitions
#include  <alarm.h>             // EPICS Alarm status and severity definitions
#include  <dbAccess.h>          // EPICS Database access messages and definitions
#include  <devSup.h>            // EPICS Device support messages and definitions
#include  <initHooks.h>         // EPICS IOC Initialization hooks support library
#include  <iocsh.h>             // EPICS IOC shell support library
#include  <link.h>              // EPICS Database link definitions
#include  <recGbl.h>            // EPICS Global record support routines
#include  <registryFunction.h>  // EPICS Registry support library

#include  <aiRecord.h>          // EPICS Analog input record definition
#include  <aoRecord.h>          // EPICS Analog output record definition
#include  <boRecord.h>          // EPICS Binary output record definition
#include  <longoutRecord.h>     // EPICS Long output record definition

#include  <Sequence.h>          // MRF Sequence Class
#include  <SequenceEvent.h>     // MRF Sequence Event Class
#include  <devSequence.h>       // MRF Sequence device support declarations
#include  <drvEvg.h>            // MRF Event Generator driver infrastructure routines

#include  <epicsExport.h>       // EPICS Symbol exporting macro definitions

/**************************************************************************************************/
/*  Structure Definitions                                                                         */
/**************************************************************************************************/

//=====================
// Common device information structure use by all SequenceEvent records
//
struct devInfoStruct {
    Sequence       *pSequence;  // Pointer to the Sequence object
    SequenceEvent  *pEvent;     // Pointer to the SequenceEvent object
    epicsInt32      Function;   // Function code
};//end devInfoStruct

/**************************************************************************************************/
/*                                Common Utility Routines                                         */
/*                                                                                                */


/**************************************************************************************************
|* disableRecord () -- Disable a Record From Ever Being Processed
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    o Set the "Processing Active" (PACT) field to "true"
|*    o Set the "Disable putFields" (DISP) field to "true"
|*    o Set the "Disable Value" (DISV) equal to the "Disable Link Value" (DISA)
|*    o Set the record status field (STAT) to "DISABLE_ALARM"
|*    o Set the record severity field (SEVR) to "INVALID_ALARM"
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    disableRecord (pRec)
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    pRec  = (dbCommon *)  Pointer to the record to disable.
|*
\**************************************************************************************************/

static
void disableRecord (dbCommon *pRec)
{
    pRec->pact = pRec->disp = true;
    pRec->disv = pRec->disa;
    pRec->stat = DISABLE_ALARM;
    pRec->sevr = pRec->diss = INVALID_ALARM;

}//end disableRecord()

/**************************************************************************************************
|* parseLink () -- Parse An EPICS Database Link Structure
|*-------------------------------------------------------------------------------------------------
|* FUNCTION:
|*    Parse the record's EPICS Database input (INP) or output (OUT) field and return the
|*    sequence number this event is assigned to, the record function field, and the event name.
|*
|*    The following sanity checks are performed on the link field:
|*      - Make sure the link type is VME_IO
|*      - Make sure the parameter field is not empty
|*      - Make sure the record function field is not empty
|*      - Make sure the event name field is not empty.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*    parseLink (dbLink, Function, Name, SeqNum);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*    dbLink   = (DBLINK &)     Reference to the record's INP or OUT link field.
|*
|*-------------------------------------------------------------------------------------------------
|* OUTPUT PARAMETERS:
|*    Function = (string &)     The record function string (from the link's "parm" field)
|*    Name     = (string &)     The sequence event name (from the link's "parm" field)
|*    SeqNum   = (epicsInt32 &) The ID number of the sequence this event belongs to
|*                              (from the link's "signal" field)
|*
|*-------------------------------------------------------------------------------------------------
|* EXCEPTIONS:
|*    A runtime_error is thrown if there was an error parsing the database link
|*
\**************************************************************************************************/

static
void parseLink (
    const DBLINK  &dbLink,      // Reference to the EPICS database link field
    std::string   &Function,    // Reference to returned function name string
    std::string   &Name,        // Reference to the returned sequence event name string
    epicsInt32    &SeqNum)      // Reference to the returned sequence ID number
{
    //=====================
    // Make sure the link type is correct
    //
    if (VME_IO != dbLink.type)
        throw std::runtime_error("Link type is not VME_IO");

    //=====================
    // Extract the sequence number from the link
    //
    SeqNum = dbLink.value.vmeio.signal;

    //=====================
    // Extract the parameter information from the link
    //
    std::string parm(dbLink.value.vmeio.parm);
    std::string::size_type index = parm.find_first_not_of(" ");

    if (std::string::npos == index)
        throw std::runtime_error("Parameter field is empty");

    //=====================
    // Extract the record function field from the parameter string
    //
    parm = parm.substr(index);
    index = parm.find(" ");

    if (std::string::npos != index)
        Function = parm.substr(0,index);

    else throw std::runtime_error("Event name not specified");

    //=====================
    // Extract the sequence event name field from the parameter string
    //
    parm = parm.substr(index+1);
    index = parm.find_first_not_of(" ");

    if (std::string::npos != index)
        Name = parm.substr(index);

    else throw std::runtime_error("Event name not specified");

}//end parseLink()

/**************************************************************************************************/
/*                    Device Support for Sequence Event Analog Output Records                     */
/*                                                                                                */



static
epicsStatus aoInitRecord (aoRecord *pRec)
{
    //=====================
    // Local variables
    //
    std::string     Function;           // Record function name (from OUT link)
    std::string     Name;               // Sequence event name (from OUT link)
    devInfoStruct  *pDevInfo = NULL;    // Pointer to device-specific information structure.
    Sequence       *pSequence;          // Pointer to the Sequence object for this event
    SequenceEvent  *pEvent;             // Pointer to our SequenceEvent object
    epicsInt32      SeqNum;             // Sequence ID number for this event (from OUT link)
    epicsStatus     status;             // Anticipated error status

    //=====================
    // Try to initialize the record
    //
    try {

        //=====================
        // Parse the OUT link and extract the record function, event name, and sequence number.
        //
        status = S_dev_badOutType;
        parseLink (pRec->out, Function, Name, SeqNum);

        //=====================
        // Make sure the function code is correct
        //
        if ("EVENT_TIME" != Function)
            throw std::runtime_error("Invalid record function (" + Function + ")");

        //=====================
        // Declare the sequence number and the event name
        //
        status = -1;
        pSequence = EgDeclareSequence (SeqNum);
        pEvent = pSequence->DeclareEvent (Name);

        //=====================
        // Register the timestamp record
        //
        status = S_dev_Conflict;
        pEvent->RegisterTimeRecord ((dbCommon *)pRec);

        //=====================
        // Create the device information structure
        //
        status = S_db_noMemory;
        if (NULL == (pDevInfo = (devInfoStruct *)malloc(sizeof(devInfoStruct))))
            throw std::runtime_error("Unable to create device information structure");

        //=====================
        // Fill in the device information structure and attach it to the record.
        //
        pDevInfo->pSequence = pSequence;
        pDevInfo->pEvent = pEvent;
        pRec->dpvt = (void *)pDevInfo;

    }//end try

    //=====================
    // If record initialization failed, report the error,
    // disable the record, and return the error status code
    //
    catch (std::exception& e) {
        recGblRecordError (status, pRec, e.what());
        disableRecord ((dbCommon *)pRec);
        return (status);
    }//end if record initialization failed

    //=====================
    // If record initialization succeeded, return success code
    //
    return OK;

}//end aoInitRecord()




//!
//! @}
//end group Sequencer
