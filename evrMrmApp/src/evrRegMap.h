
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
 * as documented in EVR-MRM-001.doc
 * Jukka Pietarinen
 * 01 Apr 2010
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

/* Loopback 0 - normal, 1 - connects local tx to local rx */
#  define Control_txloop  0x20000000
#  define Control_rxloop  0x10000000

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

/* Listed as 32-bit, but uses the same format as the 16-bit mappings */
/*#define U32_IRQPulseMap 0x010*/
#define U16_IRQPulseMap 0x012

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
#  define FWVersion_ver_mask  0x000000ff
#  define FWVersion_ver_shift  0

enum evrForm {
  evrFormCPCI=0,
  evrFormPMC=1,
  evrFormVME64=2
};

#define U32_CounterPS   0x040

#define U32_USecDiv     0x04C

#define U32_ClkCtrl     0x050
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

#define U32_ScalerN     0x100
#  define ScalerMax 3
/* 0 <= N <= 2 */
#define U32_Scaler(N)   (U32_ScalerN + (4*(N)))

#define U32_PulserNCtrl 0x200
#define U32_PulserNScal 0x204
#define U32_PulserNDely 0x208
#define U32_PulserNWdth 0x20c
#  define PulserMax 10

/* 0 <= N <= 9 */
#define U32_PulserCtrl(N) (U32_PulserNCtrl + (16*(N)))
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

/* Front panel outputs */
#define U16_OutputMapFPN 0x400
#  define OutputMapFPMax 8

/* 0 <= N <= 7 */
#define U16_OutputMapFP(N) (U16_OutputMapFPN + (2*(N)))

/* Front panel universal outputs */
#define U16_OutputMapFPUnivN 0x440
#  define OutputMapFPUnivMax 10

/* 0 <= N <= 9 */
#define U16_OutputMapFPUniv(N) (U16_OutputMapFPUnivN + (2*(N)))

/* Transition board outputs */
#define U16_OutputMapRBN 0x480
#  define OutputMapRBMax 32

/* 0 <= N <= 31 */
#define U16_OutputMapRB(N) (U16_OutputMapRBN + (2*(N)))

/* Front panel inputs */
#define U8_InputMapFPCfgN  0x500
#  define InputMapFPCfg_lvl  0x20
#  define InputMapFPCfg_blvl 0x10
#  define InputMapFPCfg_elvl 0x08
#  define InputMapFPCfg_edge 0x04
#  define InputMapFPCfg_bedg 0x02
#  define InputMapFPCfg_eedg 0x01
#define U8_InputMapFPDBusN 0x501
#define U8_InputMapFPDBEvt 0x502
#define U8_InputMapFPDEEvt 0x503
#  define InputMapFPMax 2

/* 0 <= N <= 1 */
#define U8_InputMapFPCfg(N)  (U8_InputMapFPCfgN  + (4*(N)))
#define U8_InputMapFPDBus(N) (U8_InputMapFPDBusN + (4*(N)))
#define U8_InputMapFPBEvt(N) (U8_InputMapFPDBEvt + (4*(N)))
#define U8_InputMapFPEEvt(N) (U8_InputMapFPDEEvt + (4*(N)))

/* Current mode logic (CML) outputs */
#define U32_OutputCMLNLow  0x600
#define U32_OutputCMLNRise 0x604
#define U32_OutputCMLNFall 0x608
#define U32_OutputCMLNHigh 0x60c
#define U32_OutputCMLNEna  0x610
#  define OutputCMLEna_cycl 0x80
#  define OutputCMLEna_ftrg 0x40
#  define OutputCMLEna_mode_mask 0x30
#  define OutputCMLEna_mode_orig 0x00
#  define OutputCMLEna_mode_freq 0x10
#  define OutputCMLEna_mode_patt 0x20
#  define OutputCMLEna_rst 0x04
#  define OutputCMLEna_pow 0x02
#  define OutputCMLEna_ena 0x01
#define U16_OutputCMLNCountHigh 0x0614
#define U16_OutputCMLNCountLow  0x0616
#define U32_OutputCMLNPatLength 0x0618
#  define OutputCMLPatLengthMax 2047

#define U32_OutputCMLNPat_base 0x20000
#define U32_OutputCMLPat(i,N) (U32_OutputCMLNPat_base + 0x4000*(i) + 4*(N))
#  define OutputCMLPatNBit 20
#  define OutputCMLPatMask ((1<<OutputCMLPatNBit)-1)

#  define OutputCMLMax 3

/* 0 <= N <= 2 */
#define U32_OutputCMLLow(N)  (U32_OutputCMLNLow +(0x20*(N)))
#define U32_OutputCMLRise(N) (U32_OutputCMLNRise +(0x20*(N)))
#define U32_OutputCMLFall(N) (U32_OutputCMLNFall +(0x20*(N)))
#define U32_OutputCMLHigh(N) (U32_OutputCMLNHigh +(0x20*(N)))
#define U32_OutputCMLEna(N)  (U32_OutputCMLNEna +(0x20*(N)))
#define U16_OutputCMLCountHigh(N) (U16_OutputCMLNCountHigh +(0x20*(N)))
#define U16_OutputCMLCountLow(N)  (U16_OutputCMLNCountLow  +(0x20*(N)))
#define U32_OutputCMLPatLength(N) (U32_OutputCMLNPatLength +(0x20*(N)))

#define U8_DataRx_base     0x0800
#define U8_DataTx_base     0x1800
#define U8_EventLog_base   0x2000

/* 0 <= N <= 0x7ff */
#define U8_DataRx(N)      (U8_DataRx_base + (N))
#define U8_DataTx(N)      (U8_DataTx_base + (N))

/* 0 <= N <= 0xfff */
#define U8_EventLog(N)    (U8_EventLog_base

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

#define U32__MappingRam(M,E,N) (U32_MappingRam_base + (0x2000*(M)) + (0x10*(E)) + (N))
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

#define U8_SFPDIAG_base 0x8300
#define U8_SFPDIAG(N) (U8_SFPDIAG_base + (N))

#endif /* EVRREGMAP_H */
