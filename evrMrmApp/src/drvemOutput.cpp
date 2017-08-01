/*************************************************************************\
* Copyright (c) 2013 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */


#include "evrRegMap.h"
#include "drvem.h"

#include <mrfCommon.h>
#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include <stdexcept>
#include "drvemOutput.h"

MRMOutput::MRMOutput(const std::string& n, EVRMRM* o, OutputType t, unsigned int idx)
    :base_t(n)
    ,owner(o)
    ,type(t)
    ,N(idx)
    ,isEnabled(true)
{
    shadowSource = sourceInternal();
}

MRMOutput::~MRMOutput() {}

void MRMOutput::lock() const{owner->lock();}
void MRMOutput::unlock() const{owner->unlock();}

epicsUInt32
MRMOutput::source() const
{
    return shadowSource&0xff;
}

epicsUInt32
MRMOutput::source2() const
{
    return (shadowSource>>8)&0xff;
}

void
MRMOutput::setSource(epicsUInt32 v)
{
    if( ! ( (v<=63 && v>=61) ||
            (v<=42 && v>=32) ||
            (v<=15) )
    )
        throw std::out_of_range("Mapping code is out of range");

    shadowSource &= 0xff00;
    shadowSource |= v;

    if(isEnabled)
        setSourceInternal();
}

void
MRMOutput::setSource2(epicsUInt32 v)
{
    if( ! ( (v<=63 && v>=61) ||
            (v<=42 && v>=32) ||
            (v<=15) )
    )
        throw std::out_of_range("Mapping code is out of range");

    shadowSource &= 0x00ff;
    shadowSource |= v<<8;

    if(isEnabled)
        setSourceInternal();
}

bool
MRMOutput::enabled() const
{
    return isEnabled;
}

void
MRMOutput::enable(bool e)
{
    if(e==isEnabled)
        return;

    isEnabled = e;

    setSourceInternal();
}

epicsUInt32
MRMOutput::sourceInternal() const
{
    epicsUInt32 val=64; // an invalid value
    switch(type) {
    case OutputInt:
        return  READ32(owner->base, IRQPulseMap) & 0xffff;
    case OutputFP:
        val = READ32(owner->base, OutputMapFP(N)); break;
    case OutputFPUniv:
        val = READ32(owner->base, OutputMapFPUniv(N)); break;
    case OutputRB:
        val = READ32(owner->base, OutputMapRB(N)); break;
    case OutputBackplane:
        val = READ32(owner->base, OutputMapBackplane(N)); break;
    }
    val &= Output_mask(N);
    val >>= Output_shift(N);
    return val;
}

void
MRMOutput::setSourceInternal()
{
    epicsUInt32 regval = shadowSource;
    if(!isEnabled)
        regval = 0x3f3f; // Force Low  (TODO: when to tri-state?)

    epicsUInt32 val=63;
    switch(type) {
    case OutputInt:
        WRITE32(owner->base, IRQPulseMap, regval); return;
    case OutputFP:
        val = READ32(owner->base, OutputMapFP(N)); break;
    case OutputFPUniv:
        val = READ32(owner->base, OutputMapFPUniv(N)); break;
    case OutputRB:
        val = READ32(owner->base, OutputMapRB(N)); break;
    case OutputBackplane:
        val = READ32(owner->base, OutputMapBackplane(N)); break;
    }

    val &= ~Output_mask(N);
    val |= regval << Output_shift(N);

    switch(type) {
    case OutputInt:
        break; // will not get here
    case OutputFP:
        WRITE32(owner->base, OutputMapFP(N), val); break;
    case OutputFPUniv:
        WRITE32(owner->base, OutputMapFPUniv(N), val); break;
    case OutputRB:
        WRITE32(owner->base, OutputMapRB(N), val); break;
    case OutputBackplane:
        WRITE32(owner->base, OutputMapBackplane(N), val); break;
    }
}

const char*
MRMOutput::sourceName(epicsUInt32 id) const
{
    switch(id){
    case 63: return "Force Low";
    case 62: return "Force High";
    // 43 -> 61 Reserved
    case 42: return "Prescaler (Divider) 2";
    case 41: return "Prescaler (Divider) 1";
    case 40: return "Prescaler (Divider) 0";
    case 39: return "Distributed Bus Bit 7";
    case 38: return "Distributed Bus Bit 6";
    case 37: return "Distributed Bus Bit 5";
    case 36: return "Distributed Bus Bit 4";
    case 35: return "Distributed Bus Bit 3";
    case 34: return "Distributed Bus Bit 2";
    case 33: return "Distributed Bus Bit 1";
    case 32: return "Distributed Bus Bit 0";
    // 10 -> 31 Reserved
    case 9: return "Pulse generator 9";
    case 8: return "Pulse generator 8";
    case 7: return "Pulse generator 7";
    case 6: return "Pulse generator 6";
    case 5: return "Pulse generator 5";
    case 4: return "Pulse generator 4";
    case 3: return "Pulse generator 3";
    case 2: return "Pulse generator 2";
    case 1: return "Pulse generator 1";
    case 0: return "Pulse generator 0";
    default: return "Invalid output source";
    }
}

OBJECT_BEGIN2(MRMOutput, Output)
  OBJECT_PROP2("Map2", &MRMOutput::source2, &MRMOutput::setSource2);
OBJECT_END(MRMOutput)
