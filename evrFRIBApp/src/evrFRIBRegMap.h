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

#define U32_FWVersion (4*0)

#define U32_Status (4*1)
#  define Status_Alive  (0x1)
#  define Status_Sim    (0x2)
#  define Status_CycleCnt_mask (0xffff0000)
#  define Status_CycleCnt_shift 16

#define U32_Config (4*2)
#  define Config_TestPatt (0x1)
#  define Config_FPSSim   (0x2)
#  define Config_EVGSim   (0x4)
#  define Config_BeamOn   (0x8)

#define U32_OutSelect (4*3)
#  define OutSelect_Divider_mask (0xffff)
#  define OutSelect_Divider_shift 0
#  define OutSelect_Enable (0x80000000)

#define U32_Command   (4*4)
#  define Command_ResetEVR     (0x1)
#  define Command_ResetFPSComm (0x2)
#  define Command_ForceNPermit (0x4)
#  define Command_NOKClear     (0x8)
#  define Command_NOKForce     (0x10)

#define U32_FPSComm   (4*5)
#  define FPSComm_HBLost_mask (0xffff0000)
#  define FPSComm_HBLost_shift 16
#  define FPSComm_HBPresent_mask (0x0000ffff)
#  define FPSComm_HBPresent_shift 0

#define U32_TimeSec  (4*6)
#define U32_TimeFrac (4*7)

#define U32_FPSSource  (4*8)

#define U32_FPSStatus  (4*9)
#  define FPSStatus_NPERMIT   (0x1)
#  define FPSStatus_NOK_AUTO  (0x2)
#  define FPSStatus_NOK_LATCH (0x4)
#  define FPSStatus_NOK       (0x8)

#define U32_NOKTimeSec  (4*0xa)
#define U32_NOKTimeFrac (4*0xb)
#define U32_BeamDuty    (4*0xc)

#define U32_FIFOCode (4*0x20)
#define U32_FIFOSec  (4*0x21)
#define U32_FIFOFrac (4*0x22)
#define U32_FIFOSts  (4*0x23)
#define U32_FIFOEna  (4*0x24)

#define U32_EvtConfig(N) (4*(0x100+(N)))

#define U32_PulserDelay(N) (4*(0x200+2*(N)))
#define U32_PulserWidth(N) (4*(0x201+2*(N)))

#define U32_FlashLockout (4*0x801)
#define U32_FlashCmdAddr (4*0x802)
#define U32_FlashData    (4*0x803)
#define U32_FlashDataRB  (4*0x804)
#define U32_FlashCmdWord (4*0x805)
#define U32_FlashCmdExec (4*0x806)

#endif // EVRFRIBREGMAP_H
