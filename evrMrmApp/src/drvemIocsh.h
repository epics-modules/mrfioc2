/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef EVRMRMIOCSH_H
#define EVRMRMIOCSH_H

#include <initHooks.h>
#include <evrMrmAPI.h>

#ifdef __cplusplus
extern "C" {
#endif

/**@brief Extra noise control
 *
 * Controls level of extra diagnositic output from the
 * EVR MRM device support.  Error messages are always
 * printed.
 *
 * 0 - No output
 * 1 - Basic Startup
 * 2 - Extended startup some "write"
 * 3 - All "write" (output record processing)
 * 4 - Some "read" (input record processing) and IRQ
 */
EVRMRM_API extern int evrmrmVerb;


void EVRMRM_API
mrmEvrSetupPCI(const char* id, const char* pcispec);
void EVRMRM_API
mrmEvrSetupVME(const char* id,int slot,int base,int level, int vector);

void EVRMRM_API
mrmEvrDumpMap(const char* id,int evt,int ram);
void EVRMRM_API
mrmEvrForward(const char* id, const char* events_iocsh);
void EVRMRM_API
mrmEvrLoopback(const char* id, int rxLoopback, int txLoopback);

void EVRMRM_API
mrmEvrInithooks(initHookState state);

long EVRMRM_API
mrmEvrReport(int level);

void EVRMRM_API
mrmEvrProbe(const char *id);

#ifdef __cplusplus
}
#endif

#endif /* EVRMRMIOCSH_H */
