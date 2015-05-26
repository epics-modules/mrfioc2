/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <stdexcept>
#include <epicsInterrupt.h>

#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "evrRegMap.h"
#include "drvemInput.h"

MRMInput::MRMInput(const std::string& n, volatile unsigned char *b, size_t i)
  :Input(n)
  ,base(b)
  ,idx(i)
{
}

void
MRMInput::dbusSet(epicsUInt16 v)
{
    epicsUInt32 val;

    val = READ32(base, InputMapFP(idx) );
    val &= ~InputMapFP_dbus_mask;
    val |= v << InputMapFP_dbus_shft;
    WRITE32(base, InputMapFP(idx), val);
}

epicsUInt16
MRMInput::dbus() const
{
    epicsUInt32 val;
    val = READ32(base, InputMapFP(idx) );
    val &= InputMapFP_dbus_mask;
    val >>= InputMapFP_dbus_shft;
    return val;
}

void
MRMInput::levelHighSet(bool v)
{
    if(v)
        BITCLR(NAT,32, base, InputMapFP(idx), InputMapFP_lvl);
    else
        BITSET(NAT,32, base, InputMapFP(idx), InputMapFP_lvl);
}

bool
MRMInput::levelHigh() const
{
    return !(READ32(base,InputMapFP(idx)) & InputMapFP_lvl);
}

void
MRMInput::edgeRiseSet(bool v)
{
    if(v)
        BITCLR(NAT,32, base, InputMapFP(idx), InputMapFP_edge);
    else
        BITSET(NAT,32, base, InputMapFP(idx), InputMapFP_edge);
}

bool
MRMInput::edgeRise() const
{
    return !(READ32(base,InputMapFP(idx)) & InputMapFP_edge);
}

void
MRMInput::extModeSet(TrigMode m)
{
    switch(m){
    case TrigNone:
        // Disable both level and edge
        BITCLR(NAT,32, base, InputMapFP(idx), InputMapFP_eedg);
        BITCLR(NAT,32, base, InputMapFP(idx), InputMapFP_elvl);
        break;
    case TrigLevel:
        // disable edge, enable level
        BITCLR(NAT,32, base, InputMapFP(idx), InputMapFP_eedg);
        BITSET(NAT,32, base, InputMapFP(idx), InputMapFP_elvl);
        break;
    case TrigEdge:
        // disable level, enable edge
        BITSET(NAT,32, base, InputMapFP(idx), InputMapFP_eedg);
        BITCLR(NAT,32, base, InputMapFP(idx), InputMapFP_elvl);
        break;
    }
}

TrigMode
MRMInput::extMode() const
{
    epicsUInt32 v=READ32(base, InputMapFP(idx));

    bool e = (v&InputMapFP_eedg) != 0;
    bool l = (v&InputMapFP_elvl) != 0;

    if(!e && !l)
        return TrigNone;
    else if(e && !l)
        return TrigEdge;
    else if(!e && l)
        return TrigLevel;
    else
        throw std::runtime_error("External mode cannot be set to both Edge and Level at the same time.");
}

void
MRMInput::extEvtSet(epicsUInt32 e)
{
    if(e>255)
        throw std::out_of_range("Event code # out of range. Range: 0 - 255");

    int key=epicsInterruptLock();

    epicsUInt32 val;

    val = READ32(base, InputMapFP(idx) );
    val &= ~InputMapFP_ext_mask;
    val |= e << InputMapFP_ext_shft;
    WRITE32(base, InputMapFP(idx), val);

    epicsInterruptUnlock(key);
}

epicsUInt32
MRMInput::extEvt() const
{
    epicsUInt32 val;
    val = READ32(base, InputMapFP(idx) );
    val &= InputMapFP_ext_mask;
    val >>= InputMapFP_ext_shft;
    return val;
}

void
MRMInput::backModeSet(TrigMode m)
{
    switch(m){
    case TrigNone:
        // Disable both level and edge
        BITCLR(NAT,32, base, InputMapFP(idx), InputMapFP_bedg);
        BITCLR(NAT,32, base, InputMapFP(idx), InputMapFP_blvl);
        break;
    case TrigLevel:
        // disable edge, enable level
        BITCLR(NAT,32, base, InputMapFP(idx), InputMapFP_bedg);
        BITSET(NAT,32, base, InputMapFP(idx), InputMapFP_blvl);
        break;
    case TrigEdge:
        // disable level, enable edge
        BITSET(NAT,32, base, InputMapFP(idx), InputMapFP_bedg);
        BITCLR(NAT,32, base, InputMapFP(idx), InputMapFP_blvl);
        break;
    }
}

TrigMode
MRMInput::backMode() const
{
    epicsUInt32 v=READ32(base, InputMapFP(idx));

    bool e = (v&InputMapFP_bedg) != 0;
    bool l = (v&InputMapFP_blvl) != 0;

    if(!e && !l)
        return TrigNone;
    else if(e && !l)
        return TrigEdge;
    else if(!e && l)
        return TrigLevel;
    else
        throw std::runtime_error("Backwards mode cannot be set to both Edge and Level at the same time.");
}

void
MRMInput::backEvtSet(epicsUInt32 e)
{
    if(e>255)
        throw std::out_of_range("Event code # out of range. Range: 0 - 255");

    int key=epicsInterruptLock();

    epicsUInt32 val;

    val = READ32(base, InputMapFP(idx) );
    val &= ~InputMapFP_back_mask;
    val |= e << InputMapFP_back_shft;
    WRITE32(base, InputMapFP(idx), val);

    epicsInterruptUnlock(key);
}

epicsUInt32
MRMInput::backEvt() const
{
    epicsUInt32 val;
    val = READ32(base, InputMapFP(idx) );
    val &= InputMapFP_back_mask;
    val >>= InputMapFP_back_shft;
    return val;
}
