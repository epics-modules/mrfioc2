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

/**************************************************************************************************/
/*    Series 2xx Event Generator Modular Register Map                                             */
/*                                                                                                */
/*    Note: The "Uxx_" tag at the beginning of each of definitions below should not be included   */
/*          when the defined offset is passed to one of the I/O access macros. The macros will    */
/*          append the appropriate suffix to the offset name based on the number of bits to be    */
/*          read or written.    The purpose of this method is to produce a compiler error if you  */
/*          attempt to use a macro that does not match the size of the register.                  */
/**************************************************************************************************/

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

//=====================
// AC Trigger Control Registers
//
#define  U32_AcTrigControl      0x0010
#define  U8_AcTrigControl       0x0011  // AC Trigger Input Control Register
#define  U8_AcTrigDivider       0x0012  // AC Trigger Input Divider
#define  U8_AcTrigPhase         0x0013  // AC Trigger Input Phase Delay
#define  U8_AcTrigEvtMap        0x0017  // AC Trigger Input To Trigger Event Mapping

//=====================
// Software Event Control Registers
//
#define  U8_SwEventControl      0x001A  // Software Event Control Register
#define  U8_SwEventCode         0x001B  // Software Event Code Register

#define  SW_EVT_ENABLE          0x01
#define  SW_EVT_PEND            0x02    

//=====================
// Data Buffer and Distributed Data Bus Control
//
#define  U32_DataBufferControl  0x0020  // Data Buffer Control Register
#define  U32_DBusSrc            0x0024  // Distributed Data Bus Mapping Register

//=====================
// FPGA Firmware Version
//
#define  U32_FPGAVersion        0x002C  // FPGA Firmware Version

//=====================
// Event Clock Control
//
#define  U16_uSecDiv            0x004e  // Event Clock Freq Rounded to Nearest 1 MHz
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
#define  U32_Seq1Control        0x0070  // Sequencer 1 Control Register
#define  U32_Seq2Control        0x0074  // Sequencer 2 Control Register
#define  U32_SeqControl_base    0x0070  // Sequencer Control Register Array Base
#define  U32_SeqControl(n)      (U32_SeqControl_base + (4*n))

#define  U8_SeqTrigSrc_base     0x0073
#define  U8_SeqTrigSrc(n)       (U8_SeqTrigSrc_base + (4*n))


//=====================
// Fractional Synthesizer Control Word
//
#define  U32_FracSynthWord      0x0080  // RF Reference Clock Pattern (Micrel SY87739L)

//=====================
// RF Recovery
//
#define  U32_RxInitPS           0x0088  // Initial Value For RF Recovery DCM Phase

//=====================
// Trigger Event Control Registers
//
#define  U32_TrigEventCtrl_base 0x0100  // Trigger Event Control Register Array Base
#define  U32_TrigEventCtrl(n)   (U32_TrigEventCtrl_base + (4*(n)))

#define  U8_TrigEventCode(n)    (U32_TrigEventCtrl(n) + 3)

#define  EVG_TRIG_EVT_ENA       0x00000100

//=====================
// Multiplexed Counter Control Register Arrays
//
#define  U32_MuxControl_base    0x0180  // Mux Counter Control Register Base Offset
#define  U32_MuxPrescaler_base  0x0184  // Mux Counter Prescaler Register Base Offset

#define  U32_MuxControl(n)      (U32_MuxControl_base + (8*(n)))

#define  U32_MuxPrescaler(n)    (U32_MuxPrescaler_base + (8*(n)))
#define  U8_MuxTrigMap(n)       (U32_MuxControl(n) + 3)

//=====================
// Front Panel Output Mapping Register Array
//
#define  U16_FrontOutMap_base      0x0400  // Front Output Port Mapping Register Offset
#define  U16_FrontOutMap(n)        (U16_FrontOutMap_base + (2*(n)))

//=====================
// Front Panel Universal Output Mapping Register Array
//
#define  U16_UnivOutMap_base    0x0440  // Front Univ Output Mapping Register
#define  U16_UnivOutMap(n)      (U16_UnivOutMap_base + (2*(n)))

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

#define  U8_SeqRamEvent_base    0x8007  // Sequence Ram Event Code Array Base Offset
#define  U8_SeqRamEvent(n,m)    (U8_SeqRamEvent_base + (0x4000*(n)) + (8*(m)))

//=====================
// Size of Event Generator Register Space
//
#define  EVG_REGMAP_SIZE        0x10000  // Register map size is 64K


/**************************************************************************************************/
/*    Status Register (0x0000) Bit Assignments                                                    */
/**************************************************************************************************/

#define FPGAVersion_ZERO_MASK   0x00FFFF00
#define FPGAVersion_TYPE_MASK   0xF0000000
#define FPGAVersion_FORM_MASK   0x0f000000
#define FPGAVersion_FORM_SHIFT  24
#define FPGAVersion_TYPE_SHIFT  28
#define FPGAVersion_VER_MASK    0x000000FF



/**************************************************************************************************/
/*    AC Trigger Register Bit Assignmen                                                           */
/**************************************************************************************************/

#define  EVG_AC_TRIG_BYP        0x02
#define  EVG_AC_TRIG_SYNC       0x01

/**************************************************************************************************/
/*    Interrupt Flag Register (0x0008) and Interrupt Enable Register (0x000c) Bit Assignments     */
/**************************************************************************************************/

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

/**************************************************************************************************/
/*    Outgoing Event Link Clock Source Register (0x0050) Bit Assignments                          */
/**************************************************************************************************/

#define  EVG_CLK_SRC_EXTRF      0x01  // External/Internal reference clock select

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


/**************************************************************************************************/
/* Multiplexed Counter                                                                            */
/**************************************************************************************************/

#define  EVG_MUX_POLARITY       0x40000000 
#define  EVG_MUX_STATUS         0x80000000 

/**************************************************************************************************/
/* Control Register flags                                                                         */
/**************************************************************************************************/

#define  EVG_MASTER_ENA         0x80000000
#define  EVG_DIS_EVT_REC        0x40000000
#define  EVG_REV_PWD_DOWN       0x20000000
#define  EVG_MXC_RESET          0x01000000

/**************************************************************************************************/
/* Input                                                                                          */
/**************************************************************************************************/

#define  EVG_EXT_INP_IRQ_ENA    0x01000000

#ifndef  EVG_CONSTANTS
#define  EVG_CONSTANTS

const epicsUInt16 evgNumMxc = 8;
const epicsUInt16 evgNumEvtTrig = 8;
const epicsUInt16 evgNumDbusBit = 8;
const epicsUInt16 evgNumFrontOut = 6;
const epicsUInt16 evgNumUnivOut = 4;
const epicsUInt16 evgNumFrontInp = 2;
const epicsUInt16 evgNumUnivInp = 4;
const epicsUInt16 evgNumRearInp = 16;
const epicsUInt16 evgNumSeqRam = 2;
const epicsFloat32 evgAllowedTsGitter = 0.5f;
const epicsUInt16 evgEndOfSeqBuf = 5;

#endif

#endif /* EVGREGMAP_H */
