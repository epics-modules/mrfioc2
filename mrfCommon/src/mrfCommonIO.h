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
|* 21 Oct 2009  E.Bjorklund     Removed array macros.
|*                              Reorganized register "typing" mechanism.
|*                              Expanded to handle both big-endian and little-endian busses
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This header file contains macro definitions for performing basic I/O operations on the hardware
|* registers of the MRF Event System VME and PMC cards.  Both big-endian and little-endian busses
|* are supported:
|*
|* The following operations are supported:
|*   o 8-bit, 16-bit, and 32-bit scalar reads and writes.
|*   o 8-bit, 16-bit, and 32-bit register clear operations.
|*   o 8-bit, 16-bit, and 32-bit scalar bit set operations (read/modify/write).
|*   o 8-bit, 16-bit, and 32-bit scalar bit clear operations (read/modify/write).
|*
|* Synchronous versions of each of the above operations are also provided.  Synchronous I/O
|* operations rely on underlying operating system routines to ensure that hardware pipelines
|* are flushed so that the operations are executed in the order in which they were specified.
|* The file "osiSyncIO.h", located in the OS-dependent directories (e.g., ./os/vxWorks/osiSyncIO,
|* or ./os/RTEMS/osiSyncIO) maps synchronous I/O operations onto their OS-dependent routines or
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
|* the MRF_CONFIG_SITE files, or by defining the symbol, MRF_SYNC_IO, before including this header
|* file for the first time.
|*
|* The I/O macros in this file are designed to be easily understood and to minimize errors.
|* Each macro begins with a "base" and an "offset" parameter.  The "base" parameter specifies
|* the address (in local space) of the card's register map. The "offset" parameter must be a
|* defined symbol of the form:
|*         "Uxx_OffsetName"
|* The "Uxx" part of the name specifies the size of the register in bits (U8, U16, or U32) and
|*  provides a form of "type declaration" for the symbol.
|*
|* Only the "OffsetName" part of the offset symbol should be specified when invoking one of the
|* I/O macros.  The macro itself will prepend the appropriate "Uxx_" string.  This will
|* automatically produce a compile-time error if you try to use the wrong sized macro for the
|* desired register (e.g., using an 8-bit read macro to access a 16-bit register).
|*
|*--------------------------------------------------------------------------------------------------
|* DEFINED MACROS:
|*
|* The macros defined in this file can be classified within the following 4 catagories:
|*
|* Big-Endian Read Operations:
|*     BE_READ8  (base,offset)
|*     BE_READ16 (base,offset)
|*     BE_READ32 (base,offset)
|*
|* Big-Endian Write Operations:
|*     BE_WRITE8  (base,offset,value)
|*     BE_WRITE16 (base,offset,value)
|*     BE_WRITE32 (base,offset,value)
|*
|* Little-Endian Read Operations:
|*     LE_READ8  (base,offset)
|*     LE_READ16 (base,offset)
|*     LE_READ32 (base,offset)
|*
|* Little-Endian Write Operations:
|*     LE_WRITE8  (base,offset,value)
|*     LE_WRITE16 (base,offset,value)
|*     LE_WRITE32 (base,offset,value)
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

#ifndef MRF_COMMON_IO_H
#define MRF_COMMON_IO_H

/**************************************************************************************************/
/*  Include Other Header Files Needed by This Module                                              */
/**************************************************************************************************/

#include <mrfIoOps.h>
#include <mrfBitOps.h>


/**************************************************************************************************/
/*                                 Macros For Native Order I/O                                    */
/*                                                                                                */


/*================================================================================================*/
/* Define the macros for synchronous I/O operations.                                              */
/* These will ultimately resolve into operating system dependent function or macro calls.         */
/*================================================================================================*/

/*---------------------
 * Synchronous Read Operations
 */
#define NAT_READ8(base,offset)  \
        ioread8  ((epicsUInt8 *)(base) + U8_  ## offset)
#define NAT_READ16(base,offset) \
        nat_ioread16 ((epicsUInt8 *)(base) + U16_ ## offset)
#define NAT_READ32(base,offset) \
        nat_ioread32 ((epicsUInt8 *)(base) + U32_ ## offset)

/*---------------------
 * Synchronous Write Operations
 */
#define NAT_WRITE8(base,offset,value) \
        iowrite8  (((epicsUInt8 *)(base) + U8_  ## offset),  value)
#define NAT_WRITE16(base,offset,value) \
        nat_iowrite16 (((epicsUInt8 *)(base) + U16_ ## offset), value)
#define NAT_WRITE32(base,offset,value) \
        nat_iowrite32 (((epicsUInt8 *)(base) + U32_ ## offset), value)

/**************************************************************************************************/
/*                             Macros For Big-Endian Bus I/O                                      */
/*                                                                                                */


/*================================================================================================*/
/* Define the macros for synchronous I/O operations.                                              */
/* These will ultimately resolve into operating system dependent function or macro calls.         */
/*================================================================================================*/

/*---------------------
 * Synchronous Read Operations
 */
#define BE_READ8(base,offset)  \
        ioread8  ((epicsUInt8 *)(base) + U8_  ## offset)
#define BE_READ16(base,offset) \
        be_ioread16 ((epicsUInt8 *)(base) + U16_ ## offset)
#define BE_READ32(base,offset) \
        be_ioread32 ((epicsUInt8 *)(base) + U32_ ## offset)

/*---------------------
 * Synchronous Write Operations
 */
#define BE_WRITE8(base,offset,value) \
        iowrite8  (((epicsUInt8 *)(base) + U8_  ## offset),  value)
#define BE_WRITE16(base,offset,value) \
        be_iowrite16 (((epicsUInt8 *)(base) + U16_ ## offset), value)
#define BE_WRITE32(base,offset,value) \
        be_iowrite32 (((epicsUInt8 *)(base) + U32_ ## offset), value)


/**************************************************************************************************/
/*                            Macros For Little-Endian Bus I/O                                    */
/*                                                                                                */


/*================================================================================================*/
/* Define the macros for synchronous I/O operations.                                              */
/* These will ultimately resolve into operating system dependent function or macro calls.         */
/*================================================================================================*/

/*---------------------
 * Synchronous Read Operations
 */
#define LE_READ8(base,offset)  \
        ioread8  ((epicsUInt8 *)(base) + U8_  ## offset)
#define LE_READ16(base,offset) \
        le_ioread16 ((epicsUInt8 *)(base) + U16_ ## offset)
#define LE_READ32(base,offset) \
        le_ioread32 ((epicsUInt8 *)(base) + U32_ ## offset)

/*---------------------
 * Synchronous Write Operations
 */
#define LE_WRITE8(base,offset,value) \
        iowrite8  (((epicsUInt8 *)(base) + U8_  ## offset),  value)
#define LE_WRITE16(base,offset,value) \
        le_iowrite16 (((epicsUInt8 *)(base) + U16_ ## offset), value)
#define LE_WRITE32(base,offset,value) \
        le_iowrite32 (((epicsUInt8 *)(base) + U32_ ## offset), value)


#endif
