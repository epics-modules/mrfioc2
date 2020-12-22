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

#include <stdio.h>

#include <epicsThread.h>

#include <mrfCommonIO.h>
#include "mrmspi.h"
#include <epicsExport.h>


// Data and Ctrl registers are offsets from device specific base
#define U32_SPIDData    0x0
#define U32_SPIDCtrl    0x4
#define  SPIDCtrl_Overrun 0x80
#define  SPIDCtrl_RecvRdy 0x40
#define  SPIDCtrl_SendRdy 0x20
#define  SPIDCtrl_SendEpt 0x10
#define  SPIDCtrl_TxOver  0x08
#define  SPIDCtrl_RxOver  0x04
#define  SPIDCtrl_OE      0x02
#define  SPIDCtrl_SS      0x01

int mrmSPIDebug;

MRMSPI::MRMSPI(volatile unsigned char *base)
    :base(base)
{}

MRMSPI::~MRMSPI() {}


void MRMSPI::select(unsigned id)
{
    if(mrmSPIDebug)
        printf("SPI: select %u\n", id);

    if(id==0) {
        // deselect
        WRITE32(base, SPIDCtrl, SPIDCtrl_OE);
        // wait a bit to ensure the chip sees deselect
        epicsThreadSleep(0.001);
        // disable drivers
        WRITE32(base, SPIDCtrl, 0);
    } else {
        // drivers on w/ !SS
        WRITE32(base, SPIDCtrl, SPIDCtrl_OE);
        // wait a bit to ensure the chip sees deselect
        epicsThreadSleep(0.001);
        // select
        WRITE32(base, SPIDCtrl, SPIDCtrl_OE|SPIDCtrl_SS);
    }
}

epicsUInt8 MRMSPI::cycle(epicsUInt8 in)
{
    double timeout = this->timeout();

    if(mrmSPIDebug)
        printf("SPI %02x ", int(in));

    // wait for send ready to be set
    {
        mrf::TimeoutCalculator T(timeout);
        while(T.ok() && !(READ32(base, SPIDCtrl)&SPIDCtrl_SendRdy))
            epicsThreadSleep(T.inc());
        if(!T.ok())
            throw std::runtime_error("SPI cycle timeout2");

        if(mrmSPIDebug)
            printf("(%f) ", T.sofar());
    }

    WRITE32(base, SPIDData, in);

    if(mrmSPIDebug)
        printf("-> ");

    // wait for recv ready to be set
    {
        mrf::TimeoutCalculator T(timeout);
        while(T.ok() && !(READ32(base, SPIDCtrl)&SPIDCtrl_RecvRdy))
            epicsThreadSleep(T.inc());
        if(!T.ok())
            throw std::runtime_error("SPI cycle timeout2");

        if(mrmSPIDebug)
            printf("(%f) ", T.sofar());
    }

    epicsUInt8 ret = READ32(base, SPIDData)&0xff;

    if(mrmSPIDebug) {
        printf("%02x\n", int(ret));
    }
    return ret;
}

extern "C" {
epicsExportAddress(int, mrmSPIDebug);
}
