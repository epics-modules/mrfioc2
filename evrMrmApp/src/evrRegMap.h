/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef EVRREGMAP_H
#define EVRREGMAP_H

#include <mrfBitOps.h>
#include <shareLib.h> /* for INLINE (C only) */

#ifdef __cplusplus
#  ifndef INLINE
#    define INLINE static inline
#  endif
#endif

/*
 * Registers for Modular Register Map version of EVR
 *
 * For firmware version #4
 * as documented in EVR-MRM-004.doc
 * Jukka Pietarinen
 * 07 Apr 2011
 *
 * Important note about data width
 *
 * All registers can be accessed with 8, 16, or 32 width
 * however, to support transparent operation for both
 * VME and PCI bus it is necessary to use only 32 bit
 * access.
 *
 * Bus bridge chips will transparently change the byte order.
 * VME bridges do this for any data width.  The PLX and lattice bridges
 * do this assuming 32-bit data width.
 */

#define U32_Status      0x000
#  define Status_dbus_mask  0xff000000
#  define Status_dbus_shift 24
#  define Status_legvio     0x00010000
#  define Status_sfpmod     0x00000080
#  define Status_linksts    0x00000040
#  define Status_fifostop   0x00000020

#define U32_Control     0x004
#  define Control_enable  0x80000000

#  define Control_evtfwd  0x40000000

/* 0 - normal, 1 loop back in logic */
#  define Control_txloop  0x20000000
/* 0 - normal, 1 loop back in SFP */
#  define Control_rxloop  0x10000000

#  define Control_outena  0x08000000 /* cPCI-EVRTG-300 only */

#  define Control_sreset  0x04000000 /* soft FPGA reset */

#  define Control_endian  0x02000000 /* 0 - MSB, 1 - LSB, 300 PCI devices only */

#  define Control_GTXio   0x01000000 /* GTX use external inhibit */

#  define Control_DCEna   0x00400000

/*                        Timestamp clock on DBUS #4 */
#  define Control_tsdbus  0x00004000
#  define Control_tsrst   0x00002000
#  define Control_tsltch  0x00000400

#  define Control_mapena  0x00000200
#  define Control_mapsel  0x00000100

#  define Control_logrst  0x00000080
#  define Control_logena  0x00000040
#  define Control_logdis  0x00000020
/*  Stop Event Enable */
#  define Control_logsea  0x00000010
#  define Control_fiforst 0x00000008

#define U32_IRQFlag     0x008
#  define IRQ_EoS       0x1000
#  define IRQ_SoS       0x0100
#  define IRQ_LinkChg   0x40
#  define IRQ_BufFull   0x20
#  define IRQ_HWMapped  0x10
#  define IRQ_Event     0x08
#  define IRQ_Heartbeat 0x04
#  define IRQ_FIFOFull  0x02
#  define IRQ_RXErr     0x01

#define U32_IRQEnable   0x00c
/* Same bits as IRQFlag plus */
#  define IRQ_Enable    0x80000000
#  define IRQ_PCIee     0x40000000

#define U32_IRQPulseMap 0x010

//=====================
// Software Event Control Registers
//
#define  U32_SwEvent            0x0018

#define  SwEvent_Ena            0x00000100
#define  SwEvent_Pend           0x00000200
#define  SwEvent_Code_MASK      0x000000ff
#define  SwEvent_Code_SHIFT     0

// With Linux this bit should used by the kernel driver exclusively
#define U32_PCI_MIE             0x001C
#define EVG_MIE_ENABLE          0x40000000

#define U32_DataBufCtrl 0x020
/* Write 1 to start, read for run status */
#  define DataBufCtrl_rx     0x8000
/* Write 1 to stop, read for complete status */
#  define DataBufCtrl_stop   0x4000
#  define DataBufCtrl_sumerr 0x2000
#  define DataBufCtrl_mode   0x1000
#  define DataBufCtrl_len_mask 0x0fff

#define U32_DataTxCtrl  0x024
#  define DataTxCtrl_done 0x100000
#  define DataTxCtrl_run  0x080000
#  define DataTxCtrl_trig 0x040000
#  define DataTxCtrl_ena  0x020000
#  define DataTxCtrl_mode 0x010000
#  define DataTxCtrl_len_mask 0x0007fc

#define U32_FWVersion   0x02c
#  define FWVersion_type_mask 0xf0000000
#  define FWVersion_type_shift 28
#  define FWVersion_form_mask 0x0f000000
#  define FWVersion_form_shift 24
#  define FWVersion_ver_mask  0x0000ffff
#  define FWVersion_ver_shift  0

#define U32_CounterPS   0x040 /* Timestamp event counter prescaler */

#define U32_USecDiv     0x04C

#define U32_ClkCtrl     0x050
#  define ClkCtrl_plllock 0x80000000
#  define ClkCtrl_clkmd_MASK 0x06000000
#  define ClkCtrl_clkmd_SHIFT 25
#  define ClkCtrl_cglock 0x00000200

#define U32_SRSec       0x05C

#define U32_TSSec       0x060
#define U32_TSEvt       0x064
#define U32_TSSecLatch  0x068
#define U32_TSEvtLatch  0x06c

#define U32_EvtFIFOSec  0x070
#define U32_EvtFIFOEvt  0x074
#define U32_EvtFIFOCode 0x078

#define U32_LogStatus   0x07C

#define U32_FracDiv     0x080

#define U32_RFInitPhas  0x088

#define U32_GPIODir     0x090
#define U32_GPIOIn      0x094
#define U32_GPIOOut     0x098

// SPI device access (eg. FPGA configuration eeprom)
#define U32_SPIDData    0x0A0
#define U32_SPIDCtrl    0x0A4

#define U32_DCTarget    0x0b0
#define U32_DCRxVal     0x0b4
#define U32_DCIntVal    0x0b8
#define U32_DCStatus    0x0bc
#define U32_TOPID       0x0c0

#define  U32_SeqControl_base    0x00e0
#define  U32_SeqControl(n)      (U32_SeqControl_base + (4*n))


#define U32_ScalerN     0x100
#  define ScalerMax 3
/* 0 <= N <= 2 */
#define U32_Scaler(N)   (U32_ScalerN + (4*(N)))
#  define ScalerPhasOffs_offset 0x20

#define U32_PulserNCtrl 0x200
#define U32_PulserNScal 0x204
#define U32_PulserNDely 0x208
#define U32_PulserNWdth 0x20c
#  define PulserMax 10

/* 0 <= N <= 15 */
#define U32_PulserCtrl(N) (U32_PulserNCtrl + (16*(N)))
#  define PulserCtrl_masks         0xf0000000
#  define PulserCtrl_masks_shift   28
#  define PulserCtrl_enables       0x00f00000
#  define PulserCtrl_enables_shift 20
#  define PulserCtrl_ena  0x01
#  define PulserCtrl_mtrg 0x02
#  define PulserCtrl_mset 0x04
#  define PulserCtrl_mrst 0x08
#  define PulserCtrl_pol  0x10
#  define PulserCtrl_srst 0x20
#  define PulserCtrl_sset 0x40
#  define PulserCtrl_rbv  0x80

#define U32_PulserScal(N) (U32_PulserNScal + (16*(N)))
#define U32_PulserDely(N) (U32_PulserNDely + (16*(N)))
#define U32_PulserWdth(N) (U32_PulserNWdth + (16*(N)))

/* 2x 16-bit registers are treated as one to take advantage
 * of VME/PCI invariance.  Unfortunatly this only works for
 * 32-bit operations...
 *
 * Even numbered outputs are the high word,
 * odd outputs are the low word
 */

#define Output_mask(N)  ( ((N)&1) ? 0x0000ffff : 0xffff0000 )
#define Output_shift(N) ( ((N)&1) ? 0 : 16)

/* Front panel outputs */
#define U32_OutputMapFPN 0x400
#  define OutputMapFPMax 8

/* 0 <= N <= 7 */
#define U32_OutputMapFP(N) (U32_OutputMapFPN + (2*( (N) & (~0x1) )))

/* Front panel universal outputs */
#define U32_OutputMapFPUnivN 0x440
#  define OutputMapFPUnivMax 10

/* 0 <= N <= 9 */
#define U32_OutputMapFPUniv(N) (U32_OutputMapFPUnivN + (2*( (N) & (~0x1) )))

/* Transition board outputs */
#define U32_OutputMapRBN 0x480
#  define OutputMapRBMax 32

/* 0 <= N <= 31 */
#define U32_OutputMapRB(N) (U32_OutputMapRBN + (2*( (N) & (~0x1) )))

/* Backplane line outputs */
#define U32_OutputMapBackplaneN 0x4C0
#  define OutputMapBackplaneMax 8

/* 0 <= N <= 7 */
#define U32_OutputMapBackplane(N) (U32_OutputMapBackplaneN + (2*( (N) & (~0x1) )))

/* Front panel inputs */
#define U32_InputMapFPN  0x500
#  define InputMapFP_lvl  0x20000000
#  define InputMapFP_blvl 0x10000000
#  define InputMapFP_elvl 0x08000000
#  define InputMapFP_edge 0x04000000
#  define InputMapFP_bedg 0x02000000
#  define InputMapFP_eedg 0x01000000
#  define InputMapFP_dbus_mask 0x00ff0000
#  define InputMapFP_dbus_shft 16
#  define InputMapFP_back_mask 0x0000ff00
#  define InputMapFP_back_shft 8
#  define InputMapFP_ext_mask  0x000000ff
#  define InputMapFP_ext_shft  0
#  define InputMapFPMax 2

/* 0 <= N <= 1 */
#define U32_InputMapFP(N)  (U32_InputMapFPN  + (4*(N)))

/* GTX delay */
#define U32_GTXDelayN 0x580
#define U32_GTXDelay(N) (U32_GTXDelayN + (4*(N)))

/* Current mode logic (CML) and GTX outputs */
#define U32_OutputCMLNLow  0x600
#define U32_OutputCMLNRise 0x604
#define U32_OutputCMLNFall 0x608
#define U32_OutputCMLNHigh 0x60c
#define U32_OutputCMLNEna  0x610
#  define OutputCMLEna_ftrig_mask 0xffff0000
#  define OutputCMLEna_ftrig_shft 16
#  define OutputCMLEna_type_mask 0x0c00
#  define OutputCMLEna_type_300 0x0800
#  define OutputCMLEna_type_203 0x0400
#  define OutputCMLEna_type_cml 0x0000
#  define OutputCMLEna_pha_mask 0x0300
#  define OutputCMLEna_pha_shift 8
#  define OutputCMLEna_cycl 0x80
#  define OutputCMLEna_ftrg 0x40
#  define OutputCMLEna_mode_mask 0x30
#  define OutputCMLEna_mode_orig 0x00
#  define OutputCMLEna_mode_freq 0x10
#  define OutputCMLEna_mode_patt 0x20
#  define OutputCMLEna_rst 0x04
#  define OutputCMLEna_pow 0x02
#  define OutputCMLEna_ena 0x01
#define U32_OutputCMLNCount 0x0614
#  define OutputCMLCount_mask      0xffff
#  define OutputCMLCount_high_shft 16
#  define OutputCMLCount_low_shft  0
#define U32_OutputCMLNPatLength 0x0618
#  define OutputCMLPatLengthMax 2047

#define U32_OutputCMLNPat_base 0x20000
#define U32_OutputCMLPat(i,N) (U32_OutputCMLNPat_base + 0x4000*(i) + 4*(N))

#  define OutputCMLMax 3
#  define OutputGTXMax 8

/* 0 <= N <= 2 */
#define U32_OutputCMLLow(N)  (U32_OutputCMLNLow +(0x20*(N)))
#define U32_OutputCMLRise(N) (U32_OutputCMLNRise +(0x20*(N)))
#define U32_OutputCMLFall(N) (U32_OutputCMLNFall +(0x20*(N)))
#define U32_OutputCMLHigh(N) (U32_OutputCMLNHigh +(0x20*(N)))
#define U32_OutputCMLEna(N)  (U32_OutputCMLNEna +(0x20*(N)))
/* The Count is offset by 1.  0 sends 1 word, 1 sends 2 words, ... */
#define U32_OutputCMLCount(N) (U32_OutputCMLNCount +(0x20*(N)))
#define U32_OutputCMLPatLength(N) (U32_OutputCMLNPatLength +(0x20*(N)))

#define U32_DataRx_base     0x0800
#define U32_DataTx_base     0x1800
#define U32_EventLog_base   0x2000

/* 0 <= N <= 0x7ff */
#define U32_DataRx(N)      (U32_DataRx_base + (N))
#define U32_DataTx(N)      (U32_DataTx_base + (N))

/* 0 <= N <= 0xfff */
#define U32_EventLog(N)    (U32_EventLog_base

/* 0 <= M <= 1   ram select
 * 0 <= E <= 255 event code number
 * 0 <= N <= 15  Byte
 *
 * Internal, Trigger, Set, or Reset - Block select
 */
#define U32_MappingRam_base 0x4000

#define MappingRamBlockInternal  0x0
#define MappingRamBlockTrigger   0x4
#define MappingRamBlockSet       0x8
#define MappingRamBlockReset     0xc

#define U32__MappingRam(M,E,N) (U32_MappingRam_base + (0x1000*(M)) + (0x10*(E)) + (N))
#define U32_MappingRam(M,E,N) U32__MappingRam(M,E, MappingRamBlock##N)

// MappingRam actions
#define ActionFIFOSave 127
#define ActionTSLatch  126
#define ActionLEDBlink 125
#define ActionEvtFwd   124
#define ActionLogStop  123
#define ActionLogSave  122
#define ActionHeartBeat 101
#define ActionPSRst    100

// Sequence Ram Timestamp Array Base Offset
#define  U32_SeqRamTS_base      0xc000
#define  U32_SeqRamTS(n,m)      (U32_SeqRamTS_base + (0x4000*(n)) + (8*(m)))

#define U32_SFPEEPROM_base 0x8200
#define U32_SFPEEPROM(N) (U32_SFPEEPROM_base + (N))
#define U32_SFPDIAG_base 0x8300
#define U32_SFPDIAG(N) (U32_SFPDIAG_base + (N))

#define EVR_REGMAP_SIZE 0x40000 // Total register map size = 256K

#endif /* EVRREGMAP_H */
