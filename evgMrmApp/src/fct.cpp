/*************************************************************************\
* Copyright (c) 2018 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <sstream>

#include "fct.h"
#include "evgMrm.h"
#include "sfp.h"

#define U32_Status 0
#define U32_Control 4
#define U32_UpDCValue 0x10
#define U32_FIFODCValue 0x14
#define U32_IntDCValue 0x18
#define U32_TOPID 0x2c
#define U32_PortNDCValue(N) (0x40 +(N)*4)

FCT::FCT(evgMrm *evg, const std::string& id, volatile epicsUInt8* const base)
    :mrf::ObjectInst<FCT>(id)
    ,evg(evg)
    ,base(base)
    ,sfp(8)
{
    for(size_t i=0; i<sfp.size(); i++) {
        std::ostringstream name;
        name<<id<<":SFP"<<(i+1); // manual numbers SFP from 1
        sfp[i] = new SFP(name.str(), base + 0x1000 + 0x200*i);
    }
}

FCT::~FCT() {}

epicsUInt16 FCT::statusRaw() const
{
    epicsUInt32 cur = READ32(base, Status);
    cur &= 0xff;
    WRITE32(base, Control, cur); // clear VIO latches
    return ~cur; // invert to get 1==Ok
}

double FCT::dcUpstream() const
{
    double period=1e9/evg->getFrequency(); // in nanoseconds
    return double(READ32(base, UpDCValue))/65536.0*period;
}

double FCT::dcFIFO() const
{
    double period=1e9/evg->getFrequency(); // in nanoseconds
    return double(READ32(base, FIFODCValue))/65536.0*period;
}

double FCT::dcInternal() const
{
    double period=1e9/evg->getFrequency(); // in nanoseconds
    return double(READ32(base, IntDCValue))/65536.0*period;
}

epicsUInt32 FCT::topoId() const
{
    return READ32(base, TOPID);
}

double FCT::dcPortN(unsigned port) const
{
    return READ32(base, PortNDCValue(port));
}

OBJECT_BEGIN(FCT)
    OBJECT_PROP1("Status", &FCT::statusRaw);
    OBJECT_PROP1("DCUpstream", &FCT::dcUpstream);
    OBJECT_PROP1("DCFIFO", &FCT::dcFIFO);
    OBJECT_PROP1("DCInternal", &FCT::dcInternal);
    OBJECT_PROP1("TopoID", &FCT::topoId);
    OBJECT_PROP1("DCPort1", &FCT::dcPort<0>);
    OBJECT_PROP1("DCPort2", &FCT::dcPort<1>);
    OBJECT_PROP1("DCPort3", &FCT::dcPort<2>);
    OBJECT_PROP1("DCPort4", &FCT::dcPort<3>);
    OBJECT_PROP1("DCPort5", &FCT::dcPort<4>);
    OBJECT_PROP1("DCPort6", &FCT::dcPort<5>);
    OBJECT_PROP1("DCPort7", &FCT::dcPort<6>);
    OBJECT_PROP1("DCPort8", &FCT::dcPort<7>);
OBJECT_END(FCT)
