/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef EVGREGMAP_H
#define EVGREGMAP_H

#include "epicsTypes.h"

/*
 * Series 2xx Event Generator Modular Register Map
 *
 * For firmware version #8
 * as documented in EVR-MRM-008.doc
 * Jukka Pietarinen
 * 07 Dec 2015
 *
 * Important note about data width
 *
 * All registers can be accessed with 8, 16, or 32 width
 * however, to support transparent operation for both
 * VME and PCI bus it is necessary to use only 32 bit
 * access assuming MSB ordering.
 *
 * Bus bridge chips will transparently change the byte order.
 * VME bridges do this for any data width.  The PLX and lattice bridges
 * do this assuming 32-bit data width.
 */

//=====================
// Status Registers
//
#define  U32_Status             0x0000  // Status Register (full)
#define  U8_DBusRxValue         0x0000  // Distributed Data Bus Received Values
#define  U8_DBusTxValue         0x0001  // Distributed Data Bus Transmitted Values

//=====================
// General Control Register
//
#define  U32_Control            0x0004  // Control Register

//=====================
// Interrupt Control Registers
//
#define  U32_IrqFlag            0x0008  // Interrupt Flag Register
#define  U32_IrqEnable          0x000C  // Interrupt Enable Register

#define  EVG_IRQ_ENABLE         0x80000000  // Master Interrupt Enable Bit
#define  EVG_IRQ_PCIIE          0x40000000
#define  EVG_IRQ_STOP_RAM_BASE  0x00001000  // Sequence RAM Stop Interrupt Bit
#define  EVG_IRQ_STOP_RAM(N)    (EVG_IRQ_STOP_RAM_BASE<<N)
#define  EVG_IRQ_START_RAM_BASE 0x00000100  // Sequence RAM Start Interrupt Bit
#define  EVG_IRQ_START_RAM(N)   (EVG_IRQ_START_RAM_BASE<<N)
#define  EVG_IRQ_EXT_INP        0x00000040  // External Input Interrupt Bit
#define  EVG_IRQ_DBUFF          0x00000020  // Data Buffer Interrupt Bit
#define  EVG_IRQ_FIFO           0x00000002  // Event FIFO Full Interrupt Bit
#define  EVG_IRQ_RXVIO          0x00000001  // Receiver Violation Bit

// With Linux this bit should used by the kernel driver exclusively
#define U32_PCI_MIE             0x001C
#define EVG_MIE_ENABLE          0x40000000

//=====================
// AC Trigger Control Registers
//
#define  U32_AcTrigControl      0x0010

#define  AcTrigControl_Sync_MASK    0x000d0000
#define  AcTrigControl_Sync_SHIFT   16
#define  AcTrigControl_Bypass       0x00020000
#define  AcTrigControl_Divider_MASK 0x0000ff00
#define  AcTrigControl_Divider_SHIFT 8
#define  AcTrigControl_Phase_MASK   0x000000ff
#define  AcTrigControl_Phase_SHIFT   0

#define  U32_AcTrigMap          0x0014

#define  AcTrigMap_EvtMASK 0x000000ff
#define  AcTrigMap_EvtSHIFT 0

//=====================
// Software Event Control Registers
//
#define  U32_SwEvent            0x0018

#define  SwEvent_Ena            0x00000100
#define  SwEvent_Pend           0x00000200
#define  SwEvent_Code_MASK      0x000000ff
#define  SwEvent_Code_SHIFT     0

//=====================
// Data Buffer and Distributed Data Bus Control
//
#define  U32_DataBufferControl  0x0020  // Data Buffer Control Register
#define  U32_DBusSrc            0x0024  // Distributed Data Bus Mapping Register
#define  U32_DBusTSEvt          0x0028  // Timestamp events configure for the distributed Data BusD

#define  TSDBusEvt_MASK         0x000000E0
#define  TSDBusEvt_SHIFT        5

//=====================
// FPGA Firmware Version
//
#define  U32_FPGAVersion        0x002C  // FPGA Firmware Version

#define FPGAVersion_TYPE_MASK   0xF0000000
#define FPGAVersion_FORM_MASK   0x0F000000
#define FPGAVersion_FORM_SHIFT  24
#define FPGAVersion_TYPE_SHIFT  28
#define FPGAVersion_VER_MASK    0x000000FF

//=====================
// Timestamp Control
//
#define  U32_TSControl          0x0034  // TS Control register
#define  TSGenerator_ena_MASK   0x01
#define  TSGenerator_ena_SHIFT  0
#define  TSValuse_load_MASK     0x02
#define  TSValuse_load_SHIFT    1
#define  U32_TSValue            0x0038  // TS Value to transmit

//=====================
// Event Clock Control
//
#define  U32_uSecDiv            0x004C  // Event Clock Freq Rounded to Nearest 1 MHz

#define  U32_ClockControl       0x0050

#define  ClockControl_plllock   0x80000000
#define  ClockControl_Sel_MASK  0x07000000
#define  ClockControl_Sel_SHIFT 24
#define  ClockControl_pllbw     0x70000000
#define  ClockControl_pllbw_SHIFT 28
#define  ClockControl_Div_MASK  0x003f0000
#define  ClockControl_Div_SHIFT 16
#define  ClockControl_EXTRF     0x01000000 // External/Internal reference clock select
#define  ClockControl_cglock    0x00000200

#define  PLLBandwidth_MAX       4

#define  U8_ClockSource         0x0050  // Event Clock Source(Internal or RF Input)
#define  U8_RfDiv               0x0051  // RF Input Divider
#define  U16_ClockStatus        0x0052  // Event Clock Status

//=====================
// Event Analyzer Registers
//
#define  U32_EvAnControl        0x0060  // Event Analyser Control/Status Register
#define  U16_EvAnEvent          0x0066  // Event Code & Data Buffer Byte
#define  U32_EvAnTimeHigh       0x0068  // High-Order 32 Bits of Time Stamp Counter
#define  U32_EvAnTimeLow        0x006C  // Low-Order 32 Bits of Time Stamp Counter

//=====================
// Sequence RAM Control Registers
//
#define  U32_SeqControl_base    0x0070  // Sequencer Control Register Array Base
#define  U32_SeqControl(n)      (U32_SeqControl_base + (4*n))

#define  SeqControl_TrigSrc_MASK 0x000000ff
#define  SeqControl_TrigSrc_SHIFT 0

//=====================
// Fractional Synthesizer Control Word
//
#define  U32_FracSynthWord      0x0080  // RF Reference Clock Pattern (Micrel SY87739L)

//=====================
// RF Recovery
//
#define  U32_RxInitPS           0x0088  // Initial Value For RF Recovery DCM Phase

// SPI device access (eg. FPGA configuration eeprom)
#define U32_SPIDData    0x0A0
#define U32_SPIDCtrl    0x0A4

//=====================
// Trigger Event Control Registers
//
#define  U32_TrigEventCtrl_base 0x0100  // Trigger Event Control Register Array Base
#define  U32_TrigEventCtrl(n)   (U32_TrigEventCtrl_base + (4*(n)))

#define  TrigEventCtrl_Ena        0x00000100
#define  TrigEventCtrl_Code_MASK  0x000000ff
#define  TrigEventCtrl_Code_SHIFT 0

#define  U8_TrigEventCode(n)    (U32_TrigEventCtrl(n) + 3)

#define  EVG_TRIG_EVT_ENA       0x00000100

//=====================
// Multiplexed Counter Control Register Arrays
//
#define  U32_MuxControl_base    0x0180  // Mux Counter Control Register Base Offset
#define  U32_MuxPrescaler_base  0x0184  // Mux Counter Prescaler Register Base Offset

#define  U32_MuxControl(n)      (U32_MuxControl_base + (8*(n)))

#define  MuxControl_Pol          0x40000000
#define  MuxControl_Sts          0x80000000
#define  MuxControl_TrigMap_MASK 0x000000ff
#define  MuxControl_TrigMap_SHIFT 0

#define  U32_MuxPrescaler(n)    (U32_MuxPrescaler_base + (8*(n)))

//=====================
// Front Panel Output Mapping Register Array
//
#define  U16_FrontOutMap_base      0x0400  // Front Output Port Mapping Register Offset
#define  U16_FrontOutMap(n)        (U16_FrontOutMap_base + (2*(n)))

//=====================
// Backplane Output Mapping Register Array
//
#define  U16_BackOutMap_base      0x0420  // Backplane Output Port Mapping Register Offset
#define  U16_BackOutMap(n)        (U16_BackOutMap_base + (2*(n)))

//=====================
// Universal Output Mapping Register Array
//
#define  U16_UnivOutMap_base    0x0440  // Universal Output Mapping Register
#define  U16_UnivOutMap(n)      (U16_UnivOutMap_base + (2*(n)))

//=====================
// Transition Board Output Mapping Register Array
//
#define  U16_RearOutMap_base    0x0480  // Transition Board Output Mapping Register
#define  U16_RearOutMap(n)      (U16_RearOutMap_base + (2*(n)))

//=====================
// Front Panel Input Mapping Registers
//
#define  U32_FrontInMap_base       0x0500  // Front Input Port Mapping Register
#define  U32_FrontInMap(n)         (U32_FrontInMap_base + (4*(n)))

//=====================
// Front Panel Universal Input Mapping Registers
//
#define  U32_UnivInMap_base     0x0540  // Front Univ Input Port Mapping Register
#define  U32_UnivInMap(n)       (U32_UnivInMap_base + (4*(n)))

//=====================
// Backplane Input Mapping Registers
//
#define  U32_BackInMap_base     0x0580 // Backplane Input Port Mapping Register
#define  U32_BackInMap(n)       (U32_BackInMap_base + (4*(n)))

//=====================
// Rear Universal Input Mapping Registers
//
#define  U32_RearInMap_base       0x0600  // Rear Univ Input Port Mapping Register
#define  U32_RearInMap(n)         (U32_RearInMap_base + (4*(n)))

//=====================
// Data Buffer Area
//
#define  U8_DataBuffer_base     0x0800  // Data Buffer Array Base Offset
#define  U8_DataBuffer(n)       (U8_DataBuffer_base + n)

//=====================
// Sequence RAMs
//
#define  U32_SeqRamTS_base      0x8000  // Sequence Ram Timestamp Array Base Offset
#define  U32_SeqRamTS(n,m)      (U32_SeqRamTS_base + (0x4000*(n)) + (8*(m)))

#define  U32_SeqRamEvent_base    0x8004  // Sequence Ram Event Code Array Base Offset
#define  U32_SeqRamEvent(n,m)    (U32_SeqRamEvent_base + (0x4000*(n)) + (8*(m)))

// Number of entrys in each ram
#define  SeqRam_Length (0x4000/8)

//=====================
// Size of Event Generator Register Space
//
#define  EVG_REGMAP_SIZE        0x10000  // Register map size is 64K


/**************************************************************************************************/
/*    Sequence RAM Control Register (0x0070, 0x0074) Bit Assignments                              */
/**************************************************************************************************/

#define  EVG_SEQ_RAM_RUNNING    0x02000000  // Sequence RAM is Running (read only)
#define  EVG_SEQ_RAM_ENABLED    0x01000000  // Sequence RAM is Enabled (read only)

#define  EVG_SEQ_RAM_SW_TRIG    0x00200000  // Sequence RAM Software Trigger Bit
#define  EVG_SEQ_RAM_RESET      0x00040000  // Sequence RAM Reset
#define  EVG_SEQ_RAM_DISABLE    0x00020000  // Sequence RAM Disable
#define  EVG_SEQ_RAM_ARM        0x00010000  // Sequence RAM Enable/Arm

#define  EVG_SEQ_RAM_REPEAT_MASK 0x00180000 // Sequence RAM Repeat Mode Mask
#define  EVG_SEQ_RAM_NORMAL     0x00000000  // Normal Mode: Repeat every trigger
#define  EVG_SEQ_RAM_SINGLE     0x00100000  // Single-Shot Mode: Disable on completion
#define  EVG_SEQ_RAM_RECYCLE    0x00080000  // Continuous Mode: Repeat on completion

//Mask registers
#define  EVG_SEQ_RAM_SWMASK         0x0000F000  // Sequence RAM Software mask
#define  EVG_SEQ_RAM_SWMASK_shift   12
#define  EVG_SEQ_RAM_SWENABLE       0x00000F00  // Sequence RAM Software enable
#define  EVG_SEQ_RAM_SWENABLE_shift 8

/**************************************************************************************************/
/* Control Register flags                                                                         */
/**************************************************************************************************/

#define  EVG_MASTER_ENA         0x80000000
#define  EVG_DIS_EVT_REC        0x40000000
#define  EVG_REV_PWD_DOWN       0x20000000
#define  EVG_MXC_RESET          0x01000000
#define  EVG_BCGEN              0x00800000
#define  EVG_DCMST              0x00400000

/**************************************************************************************************/
/* Input                                                                                          */
/**************************************************************************************************/

#define  EVG_EXT_INP_IRQ_ENA    0x01000000
#define  EVG_INP_FP_ENA         0x0F000000
#define  EVG_INP_FP_ENA_shift   24
#define  EVG_INP_FP_MASK        0xF0000000
#define  EVG_INP_FP_MASK_shift  28
#define  EVG_INP_MXCR_ENA       0x00008000
#define  EVG_INP_MXCR_ENA_shift 15

#ifndef  EVG_CONSTANTS
#define  EVG_CONSTANTS

#define evgNumMxc 8
#define evgNumEvtTrig 8
#define evgNumDbusBit 8
#define evgNumFrontOut 6
#define evgNumUnivOut 4
#define evgNumBackOut 8
#define evgNumRearOut 16
#define evgNumSeqRam 2
#define evgAllowedTsGitter 0.5f
#define evgEndOfSeqBuf 5

#endif

#endif /* EVGREGMAP_H */
