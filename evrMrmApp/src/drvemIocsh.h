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
#include <shareLib.h>

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
epicsShareExtern int evrmrmVerb;


void epicsShareFunc
mrmEvrSetupPCI(const char* id, const char* pcispec);
void epicsShareFunc
mrmEvrSetupVME(const char* id,int slot,int base,int level, int vector);

void epicsShareFunc
mrmEvrDumpMap(const char* id,int evt,int ram);
void epicsShareFunc
mrmEvrForward(const char* id, const char* events_iocsh);
void epicsShareFunc
mrmEvrLoopback(const char* id, int rxLoopback, int txLoopback);

void epicsShareFunc
mrmEvrInithooks(initHookState state);

long epicsShareFunc
mrmEvrReport(int level);

void epicsShareFunc
mrmEvrProbe(const char *id);

#ifdef __cplusplus
}
#endif

#endif /* EVRMRMIOCSH_H */
