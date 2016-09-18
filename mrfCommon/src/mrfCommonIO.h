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
 |* 22 Jan 2009  E.Bjorklund     Defined macros for use with all MRF hardware.
 |*
 |*--------------------------------------------------------------------------------------------------
 |* MODULE DESCRIPTION:
 |*
 |* This header file contains macro definitions for performing basic I/O operations on the hardware
 |* registers of the MRF Event System VME and PMC cards.
 |*
 |* The following operations are supported:
 |*   o 8-bit, 16-bit, and 32-bit scalar reads and writes.
 |*   o 8-bit, 16-bit, and 32-bit scalar bit set operations (read/modify/write).
 |*   o 8-bit, 16-bit, and 32-bit scalar bit clear operations (read/modify/write).
 |*   o 8-bit, 16-bit, and 32-bit scalar bit "flip" operations (read/modify/write),
 |*
 |* All I/O operations use underlying operating system routines to ensure that hardware pipelines
 |* are flushed so that the operations are executed in the order in which they were specified.
 |* The file "epicsMMIO.h", located in the OS-dependent directories (e.g., ./os/vxWorks/mrfIoOps.h,
 |* or ./os/RTEMS/epicsMMIO.h) maps synchronous I/O operations onto their OS-dependent routines or
 |* macros.
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
 |* The macros defined in this file can be classified within the following 6 catagories:
 |*
 |* Read Operations:
 |*     READ8  (base,offset)
 |*     READ16 (base,offset)
 |*     READ32 (base,offset)
 |*
 |* Write Operations:
 |*     WRITE8  (base,offset,value)
 |*     WRITE16 (base,offset,value)
 |*     WRITE32 (base,offset,value)
 |*
 |* Bit Set Operations:
 |*     BITSET8  (base,offset,mask)
 |*     BITSET16 (base,offset,mask)
 |*     BITSET32 (base,offset,mask)
 |*
 |* Bit Clear Operations:
 |*     BITCLR8  (base,offset,mask)
 |*     BITCLR16 (base,offset,mask)
 |*     BITCLR32 (base,offset,mask)
 |*
 |* Bit Flip Operations:
 |*     BITFLIP8  (base,offset,mask)
 |*     BITFLIP16 (base,offset,mask)
 |*     BITFLIP32 (base,offset,mask)
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

#include <epicsEndian.h>        /* OS-independent macros for system endianness checking           */
#include <epicsMMIO.h>          /* OS-dependent synchronous I/O routines                          */
#include <mrfBitOps.h>          /* Generic bit operations                                         */
#include <stdexcept>

/**************************************************************************************************/
/*                            Macros For Accessing MRF Timing Modules                             */
/*            (Note that MRF timing modules are always accessed using native mode I/O             */
/**************************************************************************************************/

/*---------------------
 * Synchronous Read Operations
 */
#define READ32(base,offset) NAT_READ32(base,offset)

/*---------------------
 * Synchronous Write Operations
 */
#define WRITE32(base,offset,value) NAT_WRITE32(base,offset,value)

/*---------------------
 * Bit Set Operations
 */
#define BITSET32(base,offset,mask)  BITSET(NAT,32,base,offset,mask)

/*---------------------
 * Bit Clear Operations
 */
#define BITCLR32(base,offset,mask)  BITCLR(NAT,32,base,offset,mask)

/*---------------------
 * Bit Flip Operations
 */
#define BITFLIP32(base,offset,mask)  BITFLIP(NAT,32,base,offset,mask)

/**************************************************************************************************/
/*                                 Macros For Native Order I/O                                    */
/*                                                                                                */

/*================================================================================================*/
/* Define the macros for synchronous I/O operations.                                              */
/* These will ultimately resolve into operating system dependent function or macro calls.         */
/*================================================================================================*/

#define NAT_READ32(base,offset) \
        nat_ioread32 ((epicsUInt8 *)(base) + U32_ ## offset)

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
#define BE_READ32(base,offset) \
        be_ioread32 ((epicsUInt8 *)(base) + U32_ ## offset)

/*---------------------
 * Synchronous Write Operations
 */
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
#define LE_READ32(base,offset) \
        le_ioread32 ((epicsUInt8 *)(base) + U32_ ## offset)

/*---------------------
 * Synchronous Write Operations
 */
#define LE_WRITE8(base,offset,value) \
        iowrite8  (((epicsUInt8 *)(base) + U8_  ## offset),  value)
#define LE_WRITE32(base,offset,value) \
        le_iowrite32 (((epicsUInt8 *)(base) + U32_ ## offset), value)

#endif
