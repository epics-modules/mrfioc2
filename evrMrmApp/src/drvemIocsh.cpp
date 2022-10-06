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

#include <cstdio>
#include <cstring>

#include <stdexcept>
#include <sstream>
#include <map>

#include <epicsString.h>
#include <errlog.h>
#include <drvSup.h>
#include <iocsh.h>
#include <initHooks.h>
#include <epicsExit.h>

#include <devLibPCI.h>
#include <devcsr.h>
#include <epicsInterrupt.h>
#include <epicsThread.h>
#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "drvem.h"
#include "mrfcsr.h"
#include "mrmpci.h"

#include <epicsExport.h>

#include "drvemIocsh.h"

// for htons() et al.
#ifdef _WIN32
 #include <Winsock2.h>
#endif

#include "evrRegMap.h"
#include "plx9030.h"
#include "plx9056.h"
#include "latticeEC30.h"

#ifdef _WIN32
 #define strtok_r(strToken,strDelimit,lasts ) (*(lasts) = strtok((strToken),(strDelimit)))
#endif



static const iocshArg mrmEvrSetupPCIArg0 = { "name",iocshArgString};
static const iocshArg mrmEvrSetupPCIArg1 = { "PCI id or slot=#",iocshArgString};
static const iocshArg mrmEvrSetupPCIArg2 = { "[uTCA model: 'UNIV' or 'IFP']",iocshArgString};
static const iocshArg * const mrmEvrSetupPCIArgs[3] =
{&mrmEvrSetupPCIArg0,&mrmEvrSetupPCIArg1,&mrmEvrSetupPCIArg2};
static const iocshFuncDef mrmEvrSetupPCIFuncDef =
    {"mrmEvrSetupPCI",3,mrmEvrSetupPCIArgs};
static void mrmEvrSetupPCICallFunc(const iocshArgBuf *args)
{
    mrmEvrSetupPCI(args[0].sval,args[1].sval,args[2].sval);
}

static const iocshArg mrmEvrSetupVMEArg0 = { "name",iocshArgString};
static const iocshArg mrmEvrSetupVMEArg1 = { "Bus number",iocshArgInt};
static const iocshArg mrmEvrSetupVMEArg2 = { "A32 base address",iocshArgInt};
static const iocshArg mrmEvrSetupVMEArg3 = { "IRQ Level 1-7 (0 - disable)",iocshArgInt};
static const iocshArg mrmEvrSetupVMEArg4 = { "IRQ vector 0-255",iocshArgInt};
static const iocshArg * const mrmEvrSetupVMEArgs[5] =
{&mrmEvrSetupVMEArg0,&mrmEvrSetupVMEArg1,&mrmEvrSetupVMEArg2,&mrmEvrSetupVMEArg3,&mrmEvrSetupVMEArg4};
static const iocshFuncDef mrmEvrSetupVMEFuncDef =
    {"mrmEvrSetupVME",5,mrmEvrSetupVMEArgs};
static void mrmEvrSetupVMECallFunc(const iocshArgBuf *args)
{
    mrmEvrSetupVME(args[0].sval,args[1].ival,args[2].ival,args[3].ival,args[4].ival);
}


static const iocshArg mrmEvrDumpMapArg0 = { "name",iocshArgString};
static const iocshArg mrmEvrDumpMapArg1 = { "Event code",iocshArgInt};
static const iocshArg mrmEvrDumpMapArg2 = { "Mapping select 0 or 1",iocshArgInt};
static const iocshArg * const mrmEvrDumpMapArgs[3] =
{&mrmEvrDumpMapArg0,&mrmEvrDumpMapArg1,&mrmEvrDumpMapArg2};
static const iocshFuncDef mrmEvrDumpMapFuncDef =
    {"mrmEvrDumpMap",3,mrmEvrDumpMapArgs};
static void mrmEvrDumpMapCallFunc(const iocshArgBuf *args)
{
    mrmEvrDumpMap(args[0].sval,args[1].ival,args[2].ival);
}


static const iocshArg mrmEvrForwardArg0 = { "name",iocshArgString};
static const iocshArg mrmEvrForwardArg1 = { "Event spec string",iocshArgString};
static const iocshArg * const mrmEvrForwardArgs[2] =
{&mrmEvrForwardArg0,&mrmEvrForwardArg1};
static const iocshFuncDef mrmEvrForwardFuncDef =
    {"mrmEvrForward",2,mrmEvrForwardArgs};
static void mrmEvrForwardCallFunc(const iocshArgBuf *args)
{
    mrmEvrForward(args[0].sval,args[1].sval);
}

static const iocshArg mrmEvrLoopbackArg0 = { "name",iocshArgString};
static const iocshArg mrmEvrLoopbackArg1 = { "RX-loopback",iocshArgInt};
static const iocshArg mrmEvrLoopbackArg2 = { "TX-loopback",iocshArgInt};
static const iocshArg * const mrmEvrLoopbackArgs[3] =
    {&mrmEvrLoopbackArg0,&mrmEvrLoopbackArg1,&mrmEvrLoopbackArg2};
static const iocshFuncDef mrmEvrLoopbackFuncDef =
    {"mrmEvrLoopback",3,mrmEvrLoopbackArgs};

static void mrmEvrLoopbackCallFunc(const iocshArgBuf *args)
{
    mrmEvrLoopback(args[0].sval,args[1].ival,args[2].ival);
}

static
void mrmsetupreg()
{
    initHookRegister(&mrmEvrInithooks);
    iocshRegister(&mrmEvrSetupPCIFuncDef,mrmEvrSetupPCICallFunc);
    iocshRegister(&mrmEvrSetupVMEFuncDef,mrmEvrSetupVMECallFunc);
    iocshRegister(&mrmEvrDumpMapFuncDef,mrmEvrDumpMapCallFunc);
    iocshRegister(&mrmEvrForwardFuncDef,mrmEvrForwardCallFunc);
    iocshRegister(&mrmEvrLoopbackFuncDef,mrmEvrLoopbackCallFunc);
}


static
drvet drvEvrMrm = {
    2,
    (DRVSUPFUN)mrmEvrReport,
    NULL
};
extern "C"{
epicsExportRegistrar(mrmsetupreg);
epicsExportAddress (drvet, drvEvrMrm);
}
