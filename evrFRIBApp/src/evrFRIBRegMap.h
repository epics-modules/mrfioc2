/*
 * This software is Copyright by the Board of Trustees of Michigan
 * State University (c) Copyright 2017.
 *
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef EVRFRIBREGMAP_H
#define EVRFRIBREGMAP_H

#include <mrfBitOps.h>
#include <shareLib.h> /* for INLINE (C only) */

#define NUMPULSER 2

#define REGLEN  (4*0x900)

// RO
#define U32_FWInfo (4*0)
#  define FWInfo_Version_mask (0x000000ff)
#  define FWInfo_Version_shift 0
#  define FWInfo_Flavor_mask (0x0000ff00)
#  define FWInfo_Flavor_shift 8
#  define FWInfo_Flavor_EVR (0xe1)
#  define FWInfo_Flavor_EVG (0xb1)

// RO
#define U32_Status (4*1)
#  define Status_Alive  (0x1)
#  define Status_Sim    (0x2)
#  define Status_CycleCnt_mask (0xffff0000)
#  define Status_CycleCnt_shift 16

// RW
#define U32_Config (4*2)
#  define Config_TestPatt (0x1)
#  define Config_FPSSim   (0x2)
#  define Config_EVGSim   (0x4)
#  define Config_BeamOn   (0x8)

// RW
#define U32_OutSelect (4*3)
#  define OutSelect_Divider_mask (0xffff)
#  define OutSelect_Divider_shift 0
#  define OutSelect_Enable (0x80000000)

// RW
#define U32_Command   (4*4)
#  define Command_ResetEVR     (0x1)
#  define Command_ResetFPSComm (0x2)
#  define Command_ForceNPermit (0x4)
#  define Command_NOKClear     (0x8)
#  define Command_NOKForce     (0x10)

// RO
#define U32_FPSComm   (4*5)
#  define FPSComm_HBLost_mask (0xffff0000)
#  define FPSComm_HBLost_shift 16
#  define FPSComm_HBPresent_mask (0x0000ffff)
#  define FPSComm_HBPresent_shift 0

// RO (read latches TimeFrac)
#define U32_TimeSec  (4*6)
// RO
#define U32_TimeFrac (4*7)

// RO
#define U32_FPSSource  (4*8)

// RO
#define U32_FPSStatus  (4*9)
#  define FPSStatus_NPERMIT   (0x1)
#  define FPSStatus_NOK_AUTO  (0x2)
#  define FPSStatus_NOK_LATCH (0x4)
#  define FPSStatus_NOK       (0x8)

// RO
#define U32_NOKTimeSec  (4*0xa)
// RO
#define U32_NOKTimeFrac (4*0xb)
// RO
#define U32_BeamDuty    (4*0xc)

// RO
#define U32_FIFOCode (4*0x20)
// RO
#define U32_FIFOSec  (4*0x21)
// RO
#define U32_FIFOFrac (4*0x22)
// RO
#define U32_FIFOSts  (4*0x23)
// RW
#define U32_FIFOEna  (4*0x24)

// RW
#define U32_EvtConfig(N) (4*(0x100+(N)))
#  define EvtConfig_FIFOUnMap (1u<<24u)
#  define EvtConfig_Pulse(N) (1u<<(16u+(N)))
#  define EvtConfig_Set(N)   (1u<<(8u+(N)))
#  define EvtConfig_Clear(N) (1u<<(0u+(N)))

// RW
#define U32_PulserDelay(N) (4*(0x200+2*(N)))
// RW
#define U32_PulserWidth(N) (4*(0x201+2*(N)))

// RW
#define U32_FlashLockout (4*0x801)
// RW
#define U32_FlashCmdAddr (4*0x802)
// WO
#define U32_FlashData    (4*0x803)
// RO
#define U32_FlashDataRB  (4*0x804)
// WO
#define U32_FlashCmdWord (4*0x805)
// WO
#define U32_FlashCmdExec (4*0x806)

#endif // EVRFRIBREGMAP_H
