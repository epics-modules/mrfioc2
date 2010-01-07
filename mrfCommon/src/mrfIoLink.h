/***************************************************************************************************
|* mrfIoLink.h -- Class Declaration for the MRF I/O Link Parser Class
|*
|*--------------------------------------------------------------------------------------------------
|* Authors:  E.Bjorklund (LANSCE)
|* Date:     15 December 2009
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 15 Dec 2009  E.Bjorklund     Original.
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This header file contains the class declarations for the mrfIoLink class.
|*   It parses the contents of the INP or OUT fields and returns the parameter values via
|*   "getter" functions.
|*
|*--------------------------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator and Event Receiver Cards
|*     APS Register Mask
|*     Modular Register Mask
|*
|*--------------------------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   Any
|*
\**************************************************************************************************/

/***************************************************************************************************
|*                           COPYRIGHT NOTIFICATION
|***************************************************************************************************
|*
|* THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
|* AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
|* AND IN ALL SOURCE LISTINGS OF THE CODE.
|*
|***************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included with this distribution.
|*
\**************************************************************************************************/

#ifndef MRF_IO_LINK_INC
#define MRF_IO_LINK_INC

/**************************************************************************************************/
/*  Imported Header Files                                                                         */
/**************************************************************************************************/

#include <string>               // Standard C++ string class
#include <map>                  // Standard C++ map template

#include <epicsTypes.h>         // EPICS Architecture-independent type definitions
#include <mrfCommon.h>          // MRF Common Definitions


/**************************************************************************************************/
/*  Exported Type Definitions                                                                     */
/**************************************************************************************************/

//=====================
// mrfParmNameList:
// Type definition for table of legal parameter names
//
typedef std::string  mrfParmNameList[];

//=====================
// mrfParmNameListSize:
// Macro for computing the number of elements in the table of legal parameter names
//
#define mrfParmNameListSize(list) (sizeof(list) / sizeof(std::string))

//=====================
// mrfParmMap:
// Maps parameter names to their values
//
typedef std::map <std::string, std::string>  mrfParmMap;

/**************************************************************************************************/
/*                                 mrfIoLink Class Definition                                     */
/*                                                                                                */

class mrfIoLink {

/**************************************************************************************************/
/*  Public Methods                                                                                */
/**************************************************************************************************/

public:

    //=====================
    // Constructor
    //
    mrfIoLink (const char*  parmString, const mrfParmNameList nameList, epicsInt32 numNames);

    //=====================
    // Tester functions
    //
    bool has_a (const char* parm);
    
    //=====================
    // Getter functions
    //
    std::string&   getString  (const char* parm);
    epicsInt32     getInteger (const char* parm);

    //=====================
    // Destructor
    //
    ~mrfIoLink ();

/**************************************************************************************************/
/*  Private Methods                                                                               */
/**************************************************************************************************/
private:

    void parseParameter (const std::string& parmString);


/**************************************************************************************************/
/*  Private Data                                                                                  */
/**************************************************************************************************/

private:

    //=====================
    // Local reference to the legal name table
    //
    const std::string*   LegalNames;            // Pointer to name table
    const epicsInt32     NumLegalNames;         // Number of elements in name table

    //=====================
    // Maps parameter names to values
    //
    mrfParmMap           ParmMap;

};//end class mrfIoLink

#endif // MRF_IO_LINK_INC //
