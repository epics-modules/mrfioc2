/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef PLX9030_H
#define PLX9030_H

/*
 * A selection of registers for the PLX PCI9030
 *
 * This device is exposed as BAR #0 on PCI and PMC
 * versions of the EVR
 */

/* Address space #0 is exposed as BAR #2 */
#define U32_LAS0BRD  0x28
/* Set for big endian, clear for little endian (swapped) */
#  define LAS0BRD_ENDIAN 0x01000000

/* Interrupt control */
#define U16_INTCSR   0x4c
#  define INTCSR_INT1_Enable   0x01
#  define INTCSR_INT1_Polarity 0x02
#  define INTCSR_INT1_Status   0x04
#  define INTCSR_INT2_Enable   0x08
#  define INTCSR_INT2_Polarity 0x10
#  define INTCSR_INT2_Status   0x20
#  define INTCSR_PCI_Enable    0x40
#  define INTCSR_SW_INTR       0x80

#define PCI_VENDOR_ID_PLX             0x10b5   /* PCI Vendor ID for PLX Technology, Inc.          */

#define PCI_DEVICE_ID_PLX_9030        0x9030   /* PCI Device ID for PLX-9030 bridge chip          */
#define PCI_VENDOR_ID_MRF             0x1a3e   /* PCI Vendor ID for Micro Research Finland, Oy    */
#define PCI_DEVICE_ID_MRF_PXIEVG230   0x20E6

#endif /* PLX9030_H */
