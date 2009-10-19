/***************************************************************************************************
|* mrfSyncIO.h -- Operating System Independent Synchronous I/O Routine Defintions
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   E.Bjorklund (LANSCE)
|* Date:     11 December 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 31 Dec 2006  E.Bjorklund	Original
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This file maps the MRF operating system independent synchronous I/O routines to their
|* corresponding vxWorks synchronous I/O routines.
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

#ifndef MRF_SYNC_OPS_H
#define MRF_SYNC_OPS_H

/**************************************************************************************************/
/*  Required Header Files                                                                         */
/**************************************************************************************************/

#include  <vxWorks.h>                           /* vxWorks common definitions                     */
#include  <sysLib.h>                            /* vxWorks System Library Definitions             */
#include  <epicsTypes.h>                        /* EPICS Common Type Definitions                  */

/**************************************************************************************************/
/*  Function Prototypes for Routines Not Defined in sysLib.h                                      */
/**************************************************************************************************/

epicsUInt16 sysIn16    (void*);                 /* Synchronous 16 bit read                        */
epicsUInt32 sysIn32    (void*);                 /* Synchronous 32 bit read                        */
void        sysOut16   (void*, epicsUInt16);    /* Synchronous 16 bit write                       */
void        sysOut32   (void*, epicsUInt32);    /* Synchronous 32 bit write                       */

/**************************************************************************************************/
/*  Map the OSI Synchronous I/O Routines to Their vxWorks Counterparts                            */
/**************************************************************************************************/

#define mrf_osi_read8_sync(address)  sysInByte ((epicsUInt32)(address))
#define mrf_osi_read16_sync(address) sysIn16   ((address))
#define mrf_osi_read32_sync(address) sysIn32   ((address))

#define mrf_osi_write8_sync(address,data)  sysOutByte ((epicsUInt32)(address), (epicsUInt8)(data))
#define mrf_osi_write16_sync(address,data) sysOut16   ((address), (epicsUInt16)(data))
#define mrf_osi_write32_sync(address,data) sysOut32   ((address), (epicsUInt32)(data))

#endif
