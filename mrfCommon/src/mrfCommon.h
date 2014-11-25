/***************************************************************************************************
|* mrfCommon.h -- Micro-Research Finland (MRF) Event System Series Common Defintions
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Eric Bjorklund
|* Date:     19 October 2009
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 19 Oct 2008  E.Bjorklund     Adapted from the original software for the APS Register Map
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*   This header file contains various constants and defintions used by the Micro Research Finland
|*   event system. The definitions in this file are used by both driver and device support modules,
|*   as well as user code that calls the device support interface.
|*
|*------------------------------------------------------------------------------
|* HARDWARE SUPPORTED:
|*   Series 2xx Event Generator and Event Receiver Cards
|*   APS Register Mask
|*   Modular Register Mask
|*
|*------------------------------------------------------------------------------
|* OPERATING SYSTEMS SUPPORTED:
|*   vxWorks
|*   RTEMS
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

#ifndef MRF_COMMON_H
#define MRF_COMMON_H

/**************************************************************************************************/
/*  Include Header Files from the Common Utilities                                                */
/**************************************************************************************************/

#include  <epicsVersion.h>      /* EPICS Version definition                                       */
#include  <epicsTypes.h>        /* EPICS Architecture-independent type definitions                */
#include  <epicsTime.h>         /* EPICS Time definitions                                         */
#include  <epicsMath.h>         /* EPICS Common math functions & definitions                      */
#include  <epicsInterrupt.h>

#include  <alarm.h>             /* EPICS Alarm status and severity definitions                    */
#include  <dbAccess.h>          /* EPICS Database Access definitions                              */
#include  <dbCommon.h>          /* EPICS Common record field definitions                          */
#include  <devSup.h>            /* EPICS Device support messages and definitions                  */
#include  <menuYesNo.h>         /* EPICS Yes/No record-support menu                               */

#include  <limits.h>            /* Standard C numeric limits                                      */

/**************************************************************************************************/
/*  MRF Event System Constants                                                                    */
/**************************************************************************************************/

#define MRF_NUM_EVENTS              256        /* Number of possible events                       */
#define MRF_EVENT_FIFO_SIZE         512        /* Size of EVR/EVG event FIFO                      */
#define MRF_MAX_DATA_BUFFER        2048        /* Maximum size of the distributed data buffer     */
#define MRF_FRAC_SYNTH_REF         24.0        /* Fractional Synth reference frequency (MHz).     */
#define MRF_DEF_CLOCK_SPEED       125.0        /* Default event clock speed is 125 MHz.           */
#define MRF_SN_BYTES                  6        /* Number of bytes in serial number                */
#define MRF_SN_STRING_SIZE           18        /* Size of serial number string (including NULL)   */
#define MRF_DESCRIPTION_SIZE         80        /* Size of description text string (inclucing NULL)*/

/* Event system codes with special meanings.
 */

/* The idle event.  Not usable */
#define MRF_EVENT_NULL             0x00
/* Sending 0x70 or 0x71 will cause the value in the seconds shift register to
 * be shifted up by 1.  The low bit will be set to 0 or 1 as appropriate.
 */
#define MRF_EVENT_TS_SHIFT_0       0x70
#define MRF_EVENT_TS_SHIFT_1       0x71
/* Reset the heartbeat timeout counter */
#define MRF_EVENT_HEARTBEAT        0x7A
/* Reset prescaler dividers.  Synchronizes the phase of all frequency outputs */
#define MRF_EVENT_RST_PRESCALERS   0x7B
/* Increment the fractional part of the timestamp when TS source is Mapped Code */
#define MRF_EVENT_TS_COUNTER_INC   0x7C
/* Zeros the fractional part of TS, and copys the seconds shift register to the
 * primary seconds register.
 */
#define MRF_EVENT_TS_COUNTER_RST   0x7D
/* Special code for use in sequencer.  Used in other contexts is not recommended. */
#define MRF_EVENT_END_OF_SEQUENCE  0x7F


/**************************************************************************************************/
/*  MRF Supported Bus Types                                                                       */
/**************************************************************************************************/

#define MRF_BUS_COMPACT_PCI           0        /* 0 = Compact PCI (3U)                            */
#define MRF_BUS_PMC                   1        /* 1 = PMC                                         */
#define MRF_BUS_VME                   2        /* 2 = VME 64x                                     */


/**************************************************************************************************/
/*  MRF Board Types                                                                               */
/**************************************************************************************************/

#define MRF_CARD_TYPE_EVR             1        /* 1 = Event Receiver                              */
#define MRF_CARD_TYPE_EVG             2        /* 2 = Event Generator                             */


/**************************************************************************************************/
/*  MRF Board Series Codes                                                                        */
/**************************************************************************************************/

#define MRF_SERIES_200       0x000000C8        /* Series 200 Code (in Hex)                        */
#define MRF_SERIES_220       0x000000DC        /* Series 220 Code (in Hex)                        */
#define MRF_SERIES_230       0x000000E6        /* Series 230 Code (in Hex)                        */


/**************************************************************************************************/
/*  Site-Specific Defaults                                                                        */
/*  (these parameters take their values from the MRF_CONFIG_SITE* files)                          */
/**************************************************************************************************/

/*=====================
 * Default Event Clock Frequency (in MegaHertz)
 */
#ifdef EVENT_CLOCK_FREQ
    #define EVENT_CLOCK_DEFAULT    EVENT_CLOCK_FREQ     /* Use site-selected event clock speed    */
#else
    #define EVENT_CLOCK_DEFAULT    0.00                 /* Defaults to cntrl word value or 125.0  */
#endif

/**************************************************************************************************/
/*  Function Prototype Definitions                                                                */
/**************************************************************************************************/

#ifdef __cplusplus

template<class Mutex>
class scopedLock
{
    Mutex& m;
    bool locked;
public:
    scopedLock(Mutex& mutex, bool lock=true) : m(mutex), locked(lock)
    {
        if (lock) m.lock();
    }
    ~scopedLock()
    {
        unlock();
    }
    inline void lock(){if (!locked) m.lock();locked=true;}
    inline void unlock(){if (locked) m.unlock();locked=false;}
};
#define SCOPED_LOCK2(m, name) scopedLock<epicsMutex> name(m)
#define SCOPED_LOCK(m) SCOPED_LOCK2(m, m##_guard)

class interruptLock
{
    int key;
public:
    interruptLock()
        :key(epicsInterruptLock())
    {}
    ~interruptLock()
    { epicsInterruptUnlock(key); }
};

#endif /* __cplusplus */

/**************************************************************************************************/
/*  Definitions for Compatibiliby with Older Versions of EPICS                                    */
/**************************************************************************************************/


/*---------------------
 * Macros for version comparison
 */
#ifndef VERSION_INT
#  define VERSION_INT(V,R,M,P) ( ((V)<<24) | ((R)<<16) | ((M)<<8) | (P))
#  define EPICS_VERSION_INT  VERSION_INT(EPICS_VERSION, EPICS_REVISION, EPICS_MODIFICATION, EPICS_PATCH_LEVEL)
#endif

/*---------------------
 * Older versions (< 3.14.9) of recGblRecordError took a non-const string
 */
#if EPICS_VERSION_INT < VERSION_INT(3,14,9,0)
#  define recGblRecordError(ERR, REC, STR) recGblRecordError(ERR, REC, (char*)(STR))
#endif

/*---------------------
 * Older versions (< 3.14.10) do not define POSIX_TIME
 */
#ifndef POSIX_TIME_AT_EPICS_EPOCH
#  define POSIX_TIME_AT_EPICS_EPOCH 631152000u
#endif

/*---------------------
 * Older versions (< 3.14.10) use DBE_LOG instead of DBE_ARCHIVE
 */
#ifndef DBE_ARCHIVE
#  define DBE_ARCHIVE DBE_LOG
#endif

/*---------------------
 * epicsMath.h defines "finite()" for vxWorks, but "isfinite()" is the standard.
 * finite() does not appear to be supported in epicsMath.h for all architectures.
 */
#ifndef isfinite
#  define isfinite finite
#endif

#ifdef __cplusplus
/* Round down and convert float to unsigned int
 * throws std::range_error for NaN and out of range inputs
 */
epicsUInt32 roundToUInt(double val, epicsUInt32 maxresult=0xffffffff);

char *allocSNPrintf(size_t N, const char *fmt, ...) EPICS_PRINTF_STYLE(2,3);
#endif

/**************************************************************************************************/
/*  Make Sure That EPICS 64-Bit Integer Types Are Defined                                         */
/**************************************************************************************************/

/*---------------------
 * If we are using an ISO C99 compliant compiler and the EPICS version is 3.14.9 or above,
 * then EPICS 64-bit integer types have already been defined by the epicsTypes.h file.
 * We can quit now.
 */
#if (__STDC_VERSION__ < 19990L) || (EPICS_VERSION_INT < VERSION_INT(3,14,9,0))

  /*---------------------
   * If are using an ISO C99 compliant compiler, then we can get to the int64_t and uint64_t
   * definitions through inttypes.h
   */
#  if __STDC_VERSION__ >= 1990L
#    include  <inttypes.h>
     typedef int64_t   epicsInt64;
     typedef uint64_t  epicsUInt64;

  /*---------------------
   * If we are not using an ISO C99 compliant compiler, define the 64 bit integer types
   * based on whether this is a 32-bit or 64-bit architecture.
   */
#  elif  LONG_MAX > 0x7fffffffL
     typedef long                epicsInt64;
     typedef unsigned long       epicsUInt64;
#  else
     typedef long long           epicsInt64;
     typedef unsigned long long  epicsUInt64;
#  endif

#endif /*EPICS 64-bit integer types need defining*/


#endif
