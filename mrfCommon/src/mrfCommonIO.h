/***************************************************************************************************
|* mrfCommonIO.h -- Macros for Performing I/O Operations
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund (LANSCE)
|* Date:     8 January 2007
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 08 Jan 2007  E.Bjorklund     Original
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This header file contains macro definitions for performing basic I/O operations on the hardware
|* registers of the MRF Event System VME and PMC cards.  The following operations are supported:
|*   o 8-bit, 16-bit, and 32-bit scalar reads and writes.
|*   o 8-bit, 16-bit, and 32-bit register clear operations.
|*   o 8-bit, 16-bit, and 32-bit array element reads and writes.
|*   o 8-bit, 16-bit, and 32-bit scalar bit set operations (read/modify/write).
|*   o 8-bit, 16-bit, and 32-bit scalar bit clear operations (read/modify/write).
|*
|* Synchronous versions of each of the above operations are also provided.  Synchronous I/O
|* operations rely on underlying operating system routines to ensure that hardware pipelines
|* are flushed so that the operations are executed in the order in which they were specified.
|* The file "mrfSyncIO.h", located in the OS-dependent directories (e.g., ./os/vxWorks/mrfSyncIO,
|* or ./os/RTEMS/mrfSyncIO) maps synchronous I/O operations onto their OS-dependent routines or
|* macros.
|*
|* Usually, synchronous I/O is not required and using synchronous I/O operations can hurt
|* the performance of your processor. Synchronous I/O is required, however, in the following cases:
|*   o In an interrupt service routine (ISR), synchronous I/O is required to ensure that the
|*     interrupt is acknowledged (and the interrupt line de-asserted) before the ISR exits.
|*   o In a situation where a value is read from one register by first writing a value
|*     (such as an address or an index) to a different register, the write operation must be
|*     synchronous in order to ensure that it occurs before the "value register" is read.
|*
|* If desired, all I/O operations can be forced to be synchronous by specifying SYNC_IO=YES in
|* the MRF_CONRIG_SITE files, or by defining the symbol, MRF_SYNC_IO, before including this header
|* file for the first time.
|*
|* The I/O macros in this file are designed to be easily understood and to minimize errors.
|* Each macro begins with a "base" and an "offset" parameter.  The "base" parameter specifies
|* the address (in local space) of the card's register map. The "offset" parameter must be a
|* defined symbol of the form:
|*         "OffsetName_mrfRegNN"   for scalar offsets, and:
|*         "OffsetName_mrfArrNN"   for array offsets.
|* The "NN" part of the name specifies the size of the register in bits (8, 16, or 32) and provides
|* a form of "type declaration" for the symbol.
|*
|* Only the "OffsetName" part of the offset symbol should be specified when invoking one of the
|* I/O macros.  The macro itself will append the appropriate "_mrfRegNN" string.  This will
|* automatically produce a compile-time error if you try to use the wrong sized macro for the
|* desired register (e.g., using an 8-bit read macro to access a 16-bit register).
|*
|*--------------------------------------------------------------------------------------------------
|* DEFINED MACROS:
|*
|* The macros defined in this file can be classified within the following 8 catagories:
|*                                              |
|* Scalar Read Operations:                      | Synchronous Scalar Read Operations:
|*     MRF_READ8  (base,offset)                 |     MRF_READ8_SYNC  (base,offset)
|*     MRF_READ16 (base,offset)                 |     MRF_READ16_SYNC (base,offset)
|*     MRF_READ32 (base,offset)                 |     MRF_READ32_SYNC (base,offset)
|*                                              |
|* Array Element Read Operations:               | Synchronous Array Element Read Operations:
|*     MRF_READ_ARR8  (base,offset,index)       |     MRF_READ_ARR8_SYNC  (base,offset,index)
|*     MRF_READ_ARR16 (base,offset,index)       |     MRF_READ_ARR16_SYNC (base,offset,index)
|*     MRF_READ_ARR32 (base,offset,index)       |     MRF_READ_ARR32_SYNC (base,offset,index)
|*                                              |
|* Scalar Write Operations:                     | Synchronous Scalar Write Operations:
|*     MRF_WRITE8  (base,offset,value)          |     MRF_WRITE8_SYNC  (base,offset,value)
|*     MRF_WRITE16 (base,offset,value)          |     MRF_WRITE16_SYNC (base,offset,value)
|*     MRF_WRITE32 (base,offset,value)          |     MRF_WRITE32_SYNC (base,offset,value)
|*                                              |
|*     MRF_CLEAR8  (base,offset)                |     MRF_CLEAR8_SYNC  (base,offset)
|*     MRF_CLEAR16 (base,offset)                |     MRF_CLEAR16_SYNC (base,offset)
|*     MRF_CLEAR32 (base,offset)                |     MRF_CLEAR32_SYNC (base,offset)
|*                                              |
|*     MRF_BITCLR8  (base,offset,mask)          |     MRF_BITCLR8_SYNC  (base,offset,mask)
|*     MRF_BITCLR16 (base,offset,mask)          |     MRF_BITCLR16_SYNC (base,offset,mask)
|*     MRF_BITCLR32 (base,offset,mask)          |     MRF_BITCLR32_SYNC (base,offset,mask)
|*                                              |
|*     MRF_BITSET8  (base,offset,mask)          |     MRF_BITSET8_SYNC  (base,offset,mask)
|*     MRF_BITSET16 (base,offset,mask)          |     MRF_BITSET16_SYNC (base,offset,mask)
|*     MRF_BITSET32 (base,offset,mask)          |     MRF_BITSET32_SYNC (base,offset,mask)
|*                                              |
|* Array Element Write Operations:              | Synchronous Array Element Write Operations:
|*     MRF_WRITE_ARR8  (base,offset,index,value)|     MRF_WRITE_ARR8_SYNC  (base,offset,index,value)
|*     MRF_WRITE_ARR16 (base,offset,index,value)|     MRF_WRITE_ARR16_SYNC (base,offset,index,value)
|*     MRF_WRITE_ARR32 (base,offset,index,value)|     MRF_WRITE_ARR32_SYNC (base,offset,index,value)
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

#ifndef MRF_COMMON_IO_H
#define MRF_COMMON_IO_H

/**************************************************************************************************/
/*  Header Files Needed by This Module                                                            */
/**************************************************************************************************/

#include <mrfSyncIO.h>                         /* Synchronous I/O read/write instructions         */


/*================================================================================================*/
/* Define the macros for synchronous I/O operations.                                              */
/* These will ultimately resolve into operating system dependent function or macro calls.         */
/*================================================================================================*/

/*---------------------
 * Synchronous Scalar Read Operations
 */
#define MRF_READ8_SYNC(base,offset)  \
        mrf_osi_read8_sync  ((epicsUInt8 *)(base) + offset ## _MrfReg8)
#define MRF_READ16_SYNC(base,offset) \
        mrf_osi_read16_sync ((epicsUInt8 *)(base) + offset ## _MrfReg16)
#define MRF_READ32_SYNC(base,offset) \
        mrf_osi_read32_sync ((epicsUInt8 *)(base) + offset ## _MrfReg32)

/*---------------------
 * Synchronous Array Element Read Operations
 */
#define MRF_READ_ARR8_SYNC(base,offset,index) \
        mrf_osi_read8_sync (&((epicsUInt8 *)((epicsUInt8 *)(base)  + offset ## _MrfArr8))[index])
#define MRF_READ_ARR16_SYNC(base,offset,index) \
        mrf_osi_read16_sync(&((epicsUInt16 *)((epicsUInt8 *)(base) + offset ## _MrfArr16))[index])
#define MRF_READ_ARR32_SYNC(base,offset,index) \
        mrf_osi_read32_sync(&((epicsUInt32 *)((epicsUInt8 *)(base) + offset ## _MrfArr32))[index])

/*---------------------
 * Synchronous Scalar Write Operations
 */
#define MRF_WRITE8_SYNC(base,offset,value) \
        mrf_osi_write8_sync  (((epicsUInt8 *)(base) + offset ## _MrfReg8),  value)
#define MRF_WRITE16_SYNC(base,offset,value) \
        mrf_osi_write16_sync (((epicsUInt8 *)(base) + offset ## _MrfReg16), value)
#define MRF_WRITE32_SYNC(base,offset,value) \
        mrf_osi_write32_sync (((epicsUInt8 *)(base) + offset ## _MrfReg32), value)

/*---------------------
 * Synchronous Array Element Write Operations
 */
#define MRF_WRITE_ARR8_SYNC(base,offset,index,value)                                               \
       mrf_osi_read8_sync ((&((epicsUInt8  *)((epicsUInt8 *)(base) + offset ## _MrfArr8)) [index]),\
                            value)
#define MRF_WRITE_ARR16_SYNC(base,offset,index,value)                                              \
       mrf_osi_read16_sync((&((epicsUInt16 *)((epicsUInt8 *)(base) + offset ## _MrfArr16))[index]),\
                            value)
#define MRF_WRITE_ARR32_SYNC(base,offset,index,value)                                              \
       mrf_osi_read32_sync((&((epicsUInt32 *)((epicsUInt8 *)(base) + offset ## _MrfArr32))[index]),\
                            value)

/*---------------------
 * Synchronous Register Clear Operations (Scalars Only)
 */
#define MRF_CLEAR8_SYNC(base,offset)  MRF_WRITE8_SYNC (base, offset, 0)
#define MRF_CLEAR16_SYNC(base,offset) MRF_WRITE16_SYNC(base, offset, 0)
#define MRF_CLEAR32_SYNC(base,offset) MRF_WRITE32_SYNC(base, offset, 0)

/*---------------------
 * Synchronous Bit Clear Operations (Scalars Only)
 */
#define MRF_BITCLR8_SYNC(base,offset,mask) \
        MRF_WRITE8_SYNC (base, offset, (MRF_READ8_SYNC(base, offset)  & (epicsUInt8)~(mask)))
#define MRF_BITCLR16_SYNC(base,offset,mask) \
        MRF_WRITE16_SYNC(base, offset, (MRF_READ16_SYNC(base, offset) & (epicsUInt16)~(mask)))
#define MRF_BITCLR32_SYNC(base,offset,mask) \
        MRF_WRITE32_SYNC(base, offset, (MRF_READ32_SYNC(base, offset) & (epicsUInt32)~(mask)))

/*---------------------
 * Synchronous Bit Set Operations
 */
#define MRF_BITSET8_SYNC(base,offset,mask) \
        MRF_WRITE8_SYNC (base, offset, (MRF_READ8_SYNC(base, offset)  | (epicsUInt8)(mask)))
#define MRF_BITSET16_SYNC(base,offset,mask) \
        MRF_WRITE16_SYNC(base, offset, (MRF_READ16_SYNC(base, offset) | (epicsUInt16)(mask)))
#define MRF_BITSET32_SYNC(base,offset,mask) \
        MRF_WRITE32_SYNC(base, offset, (MRF_READ32_SYNC(base, offset) | (epicsUInt32)(mask)))

/*================================================================================================*/
/* If SYNC_IO=YES is specified in the MRF_CONFIG_SITE files, or if MRF_SYNC_IO is defined         */
/* prior to including this header file for the first time, make all I/O synchronous.              */
/*================================================================================================*/

#ifdef MRF_SYNC_IO

/*---------------------
 * Make All Scalar Read Operations Synchronous
 */
#define MRF_READ8(base,offset)  MRF_READ8_SYNC (base,offset)
#define MRF_READ16(base,offset) MRF_READ16_SYNC(base,offset)
#define MRF_READ32(base,offset) MRF_READ32_SYNC(base,offset)

/*---------------------
 * Make All Array Element Read Operations Synchronous
 */
#define MRF_READ_ARR8(base,offset,index)  MRF_READ_ARR8_SYNC (base,offset,index)
#define MRF_READ_ARR16(base,offset,index) MRF_READ_ARR16_SYNC(base,offset,index)
#define MRF_READ_ARR32(base,offset,index) MRF_READ_ARR32_SYNC(base,offset,index)

/*---------------------
 * Make All Scalar Write Operations Synchronous
 */
#define MRF_WRITE8(base,offset,value)  MRF_WRITE8_SYNC (base,offset,value)
#define MRF_WRITE16(base,offset,value) MRF_WRITE16_SYNC(base,offset,value)
#define MRF_WRITE32(base,offset,value) MRF_WRITE32_SYNC(base,offset,value)

/*---------------------
 * Make All Array Element Write Operations Synchronous
 */
#define MRF_WRITE_ARR8(base,offset,index,value)  MRF_WRITE_ARR8_SYNC (base,offset,index,value)
#define MRF_WRITE_ARR16(base,offset,index,value) MRF_WRITE_ARR16_SYNC(base,offset,index,value)
#define MRF_WRITE_ARR32(base,offset,index,value) MRF_WRITE_ARR32_SYNC(base,offset,index,value)


#else

/*================================================================================================*/
/* Define macros for regular (non-synchronous) I/O operations                                     */
/*================================================================================================*/

/*---------------------
 * Regular Scalar Read Operations
 */
#define MRF_READ8(base,offset)  \
        *((volatile epicsUInt8 *) ((epicsUInt8 *)(base) + offset ## _MrfReg8))
#define MRF_READ16(base,offset) \
        *((volatile epicsUInt16 *)((epicsUInt8 *)(base) + offset ## _MrfReg16))
#define MRF_READ32(base,offset) \
        *((volatile epicsUInt32 *)((epicsUInt8 *)(base) + offset ## _MrfReg32))

/*---------------------
 * Regular Array Element Read Operations
 */
#define MRF_READ_ARR8(base,offset,index) \
        ((volatile epicsUInt8 *)((epicsUInt8 *)(base)  + offset ## _MrfArr8))[index]
#define MRF_READ_ARR16(base,offset,index) \
        ((volatile epicsUInt16 *)((epicsUInt8 *)(base) + offset ## _MrfArr16))[index]
#define MRF_READ_ARR32(base,offset,index) \
        ((volatile epicsUInt32 *)((epicsUInt8 *)(base) + offset ## _MrfArr32))[index]

/*---------------------
 * Regular Scalar Write Operations
 */
#define MRF_WRITE8(base,offset,value) \
        *(volatile epicsUInt8 *)((epicsUInt8 *)(base)  + offset ## _MrfReg8)  = value
#define MRF_WRITE16(base,offset,value) \
        *(volatile epicsUInt16 *)((epicsUInt8 *)(base) + offset ## _MrfReg16) = value
#define MRF_WRITE32(base,offset,value) \
        *(volatile epicsUInt32 *)((epicsUInt8 *)(base) + offset ## _MrfReg32) = value

/*---------------------
 * Regular Array Element Write Operations
 */
#define MRF_WRITE_ARR8(base,offset,index,value) \
        ((volatile epicsUInt8 *) ((epicsUInt8 *)(base) + offset ## _MrfArr8))[index]  = value
#define MRF_WRITE_ARR16(base,offset,index,value) \
        ((volatile epicsUInt16 *)((epicsUInt8 *)(base) + offset ## _MrfArr16))[index] = value
#define MRF_WRITE_ARR32(base,offset,index,value) \
        ((volatile epicsUInt32 *)((epicsUInt8 *)(base) + offset ## _MrfArr32))[index] = value

#endif

/*================================================================================================*/
/* Define macros for derived I/O operations.                                                      */
/* These will resolve into either regular or synchronous operations, depending on how             */
/* the SYNC_IO option was defined in the MRF_CONFIG_SITE files.                                   */
/*================================================================================================*/

/*---------------------
 * Register Clear Operations
 */
#define MRF_CLEAR8(base,offset)  MRF_WRITE8 (base, offset, 0)
#define MRF_CLEAR16(base,offset) MRF_WRITE16(base, offset, 0)
#define MRF_CLEAR32(base,offset) MRF_WRITE32(base, offset, 0)

/*---------------------
 * Bit Clear Operations
 */
#define MRF_BITCLR8(base,offset,mask) \
        MRF_WRITE8 (base, offset, (MRF_READ8(base, offset)  & (epicsUInt8)~(mask)))
#define MRF_BITCLR16(base,offset,mask) \
        MRF_WRITE16(base, offset, (MRF_READ16(base, offset) & (epicsUInt16)~(mask)))
#define MRF_BITCLR32(base,offset,mask) \
        MRF_WRITE32(base, offset, (MRF_READ32(base, offset) & (epicsUInt32)~(mask)))

/*---------------------
 * Bit Set Operations
 */
#define MRF_BITSET8(base,offset,mask) \
        MRF_WRITE8 (base, offset, (MRF_READ8(base, offset)  | (epicsUInt8)(mask)))
#define MRF_BITSET16(base,offset,mask) \
        MRF_WRITE16(base, offset, (MRF_READ16(base, offset) | (epicsUInt16)(mask)))
#define MRF_BITSET32(base,offset,mask) \
        MRF_WRITE32(base, offset, (MRF_READ32(base, offset) | (epicsUInt32)(mask)))

#endif
