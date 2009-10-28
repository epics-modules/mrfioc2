/***************************************************************************************************
|* vmeCSRMemProbeA.c -- RTEMS-Specific Routine to Probe VME CR/CSR Space
|*                      For Use When the BSP Does Not Support CR/CSR Addressing
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund (LANSCE)
|*
|* Date:     30 May 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 30 May 2006  E.Bjorklund     Original.
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This module contains the top-level RTEMS-specific code for reading and writing to VME CR/CSR
|* address space when the Board-Support Package (BSP) does not support CR/CSR addressing.  When
|* the BSP does not directly support CR/CSR addressing, our strategy is to expropriate the A24
|* target window in the VME bridge chip (because it is the same size as CR/CSR space), make it a
|* CR/CSR target window while we are executing the read or write probe, and then return it to an
|* A24 window.
|*
|* All this module does is select the appropriate code file based on the target architecture
|* and cause it to be included in the compilation of this module.  It does contain a default
|* probe routine in case the caller has selected a BSP that is not yet supported.
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
|* Copyright (c) 2006 The University of Chicago,
|* as Operator of Argonne National Laboratory.
|*
|* Copyright (c) 2006 The Regents of the University of California,
|* as Operator of Los Alamos National Laboratory.
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


/**************************************************************************************************/
/*  Imported Header Files Common To All RTEMS Board-Support Packages                              */
/**************************************************************************************************/

#include <epicsTypes.h>         /* EPICS architecture-independent type definitions                */
#include <errlog.h>             /* EPICS error logging routines                                   */

#include <mrfVme64x.h>          /* VME-64X CR/CSR definitions                                     */


/**************************************************************************************************/
/*  Select the BSP-Specific Code For Our Target Architecture                                      */
/**************************************************************************************************/

/*--------------------           No RTEMS BSPs Currently Supported        ------------------------*/
/*--------------------- BSP Must Be Modified To Support CR/CSR Addressing ------------------------*/


#ifndef BSP_SUPPORTED

/**************************************************************************************************
|* vmeCSRMemProbe () -- Probe VME CR/CSR Address Space
|*-------------------------------------------------------------------------------------------------
|*
|* This is a default routine which is only included if the specified BSP is not supported.  If it
|* is called, it will log an error message ane return ERROR as its status code.
|*
|*-------------------------------------------------------------------------------------------------
|* CALLING SEQUENCE:
|*      status = vmeCSRMemProbe (csrAddress, mode, length, &pVal);
|*
|*-------------------------------------------------------------------------------------------------
|* INPUT PARAMETERS:
|*      csrAddress  = (epicsUInt32)   Starting CR/CSR address to probe
|*      mode        = (epicsInt32)    CSR_READ or CSR_WRITE
|*      length      = (epicsInt32)    Number of bytes to read/write
|*
|*-------------------------------------------------------------------------------------------------
|* OUTUT PARAMETERS:
|*      pVal        = (epicsUInt32 *) Pointer to longword array which will contain either
|*                                    the data to be written or the data to be read.
|*                                    Actual data is read or written from the low-order byte
|*                                    of each element in the longword array.
|*
|*-------------------------------------------------------------------------------------------------
|* RETURNS:
|*      status      = (epicsStatus)  Always returns ERROR.
|*
\**************************************************************************************************/


epicsStatus vmeCSRMemProbe (
    epicsUInt32   csrAddress,        /* Address in CR/CSR space to be probed.            */
    epicsInt32    mode,              /* Probe direction (read or write)                  */
    epicsInt32    length,            /* Number of bytes to read/write                    */
    epicsUInt32  *pVal)              /* Data source (write) or sink (read)               */
{

   /*---------------------
    * Log an error message and return.
    */
    errPrintf (
        -1, __FILE__, __LINE__,
        "  This Board Support Package does not currently support CR/CSR address probing.\n");

    return ERROR;

}/*end vmeCSRMemProbe()*/

#endif
