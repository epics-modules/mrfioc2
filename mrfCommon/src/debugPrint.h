/**************************************************************************************************
|* debugPrint.h -- Debug Printing Macros
|*
|*--------------------------------------------------------------------------------------------------
|* Authors:  S.Allison (SLAC)
|*           K.Luchini (SLAC)
|*
|* Date:     9 December 2004
|*
|*-------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 09 Dec 2004  S.Allison       Original
|* 19 Jan 2005  D.Rogind        Added interest level definitions
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This file contains the SLAC debug printing mechanism.
|*
|* To enable debug printing, add the following line to the EPICS Makefile:
|*      #USR_CFLAGS += -DDEBUG_PRINT
|* When debug printout is desired, uncomment the line, clean, and rebuild.
|*
|* To use in a C file, add these lines to the appropriate places of <fileName>.c:
|*      #include "debugPrint.h"
|*      #ifdef  DEBUG_PRINT
|*         int <fileName>Flag = 0;
|*      #endif
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      DEBUGPRINT (DP_XXX, InterestFlag, (printfArgs));
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      DP_XXX          = Message severity.  The six defined levels, in order of increasing
|*                        verbosity are:
|*                          DP_NONE
|*                          DP_FATAL
|*                          DP_ERROR
|*                          DP_WARN
|*                          DP_INFO
|*                          DP_DEBUG
|*
|*      InterestFlag    = Interest level threshold.  Typically this has the form <fileName>Flag.
|*                        The debug message will be printed if the message severity code (DP_XXXX)
|*                        is less than or equal to the InterestFlag value.
|*      printfArgs      = Parameter list to the printf function.  This typically takes the
|*                        form:
|*                           ("text with %s %d etc\n", arg0, arg1, ...)
|*                        Note that the enclosing parenthesis are required.
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
|* Copyright (c) 2006 The Board of Trustees of the Leland Stanford Junior
|* University, as Operator of the Stanford Linear Accelerator Center.
|*
|**************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included with this distribution.
|*
\*************************************************************************************************/

#ifndef DEBUGPRINT_H
#define DEBUGPRINT_H

/**************************************************************************************************/
/*  Other Header Files Required By This File                                                      */
/**************************************************************************************************/

#include <stdio.h>	/* Standard C I/O routines  */

/**************************************************************************************************/
/*  Define Interest Levels                                                                        */
/**************************************************************************************************/

#define DP_NONE  0
#define DP_FATAL 1
#define DP_ERROR 2
#define DP_WARN  3
#define DP_INFO  4
#define DP_DEBUG 5

/**************************************************************************************************/
/*  Define the DEBUGPRINT Macro                                                                   */
/**************************************************************************************************/

#ifdef  DEBUG_PRINT
        #define DEBUGPRINT(interest, globalFlag, args) {if (interest <= globalFlag) printf args;}
#else
        #define DEBUGPRINT(interest, globalFlag, args)
#endif


#endif /* guard */
