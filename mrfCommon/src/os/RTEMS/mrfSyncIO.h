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
|* corresponding RTEMS synchronous I/O routines.
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

#include  <basicIoOps.h>                        /* RTEMS Synchronized I/O Operation Definitions   */
#include  <epicsTypes.h>                        /* EPICS Common Type Definitions                  */

/**************************************************************************************************/
/*  Map the OSI Synchronous I/O Routines to Their RTEMS Counterparts                              */
/**************************************************************************************************/

#define mrf_osi_read8_sync(address)  in_8   ((volatile void *)(address))
#define mrf_osi_read16_sync(address) in_be16((volatile void *)(address))
#define mrf_osi_read32_sync(address) in_be32((volatile void *)(address))

#define mrf_osi_write8_sync(address,value)  \
        out_8   ((volatile void *)(address),  (epicsUInt8)(value))
#define mrf_osi_write16_sync(address,value) \
        out_be16((volatile void *)(address), (epicsUInt16)(value))
#define mrf_osi_write32_sync(address,value) \
        out_be32((volatile void *)(address), (epicsUInt32)(value))

#endif
