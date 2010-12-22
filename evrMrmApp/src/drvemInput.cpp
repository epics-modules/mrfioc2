/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include <drvemInput.h>

#include <mrfCommonIO.h>
#include <mrfBitOps.h>

#include "evrRegMap.h"

#include <stdexcept>

#define DBG evrmrmVerb

MRMInput::MRMInput(volatile unsigned char *b, size_t i)
  :base(b)
  ,idx(i)
{
}

void
MRMInput::dbusSet(epicsUInt16 v)
{
    epicsUInt8 mask=v;

    WRITE8(base, InputMapFPDBus(idx), mask);
}

epicsUInt16
MRMInput::dbus() const
{
    return READ8(base, InputMapFPDBus(idx));
}

void
MRMInput::levelHighSet(bool v)
{
    if(v)
        BITCLR(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_lvl);
    else
        BITSET(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_lvl);
}

bool
MRMInput::levelHigh() const
{
    return !(READ8(base,InputMapFPCfg(idx)) & InputMapFPCfg_lvl);
}

void
MRMInput::edgeRiseSet(bool v)
{
    if(v)
        BITCLR(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_edge);
    else
        BITSET(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_edge);
}

bool
MRMInput::edgeRise() const
{
    return !(READ8(base,InputMapFPCfg(idx)) & InputMapFPCfg_edge);
}

void
MRMInput::extModeSet(TrigMode m)
{
    switch(m){
	case TrigNone:
		// Disable both level and edge
        BITCLR(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_eedg);
        BITCLR(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_elvl);
		break;
	case TrigLevel:
		// disable edge, enable level
        BITCLR(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_eedg);
        BITSET(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_elvl);
		break;
	case TrigEdge:
		// disable level, enable edge
        BITSET(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_eedg);
        BITCLR(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_elvl);
		break;
    }
}

TrigMode
MRMInput::extMode() const
{
	epicsUInt8 v=READ8(base, InputMapFPCfg(idx));

	bool e=v&InputMapFPCfg_eedg;
	bool l=v&InputMapFPCfg_elvl;

	if(!e && !l)
		return TrigNone;
	else if(e && !l)
		return TrigEdge;
	else if(!e && l)
		return TrigLevel;
	else
		throw std::runtime_error("Input Ext has both Edge and Level at the same time??");
}

void
MRMInput::extEvtSet(epicsUInt32 e)
{
	if(e>255)
		std::out_of_range("Event code # out of range");

	WRITE8(base, InputMapFPEEvt(idx), (epicsUInt8)e);
}

epicsUInt32
MRMInput::extEvt() const
{
    return READ8(base, InputMapFPEEvt(idx));
}

void
MRMInput::backModeSet(TrigMode m)
{
    switch(m){
	case TrigNone:
		// Disable both level and edge
        BITCLR(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_bedg);
        BITCLR(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_blvl);
		break;
	case TrigLevel:
		// disable edge, enable level
        BITCLR(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_bedg);
        BITSET(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_blvl);
		break;
	case TrigEdge:
		// disable level, enable edge
        BITSET(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_bedg);
        BITCLR(NAT,8, base, InputMapFPCfg(idx), InputMapFPCfg_blvl);
		break;
    }
}

TrigMode
MRMInput::backMode() const
{
	epicsUInt8 v=READ8(base, InputMapFPCfg(idx));

	bool e=v&InputMapFPCfg_bedg;
	bool l=v&InputMapFPCfg_blvl;

	if(!e && !l)
		return TrigNone;
	else if(e && !l)
		return TrigEdge;
	else if(!e && l)
		return TrigLevel;
	else
		throw std::runtime_error("Input Back has both Edge and Level at the same time??");
}

void
MRMInput::backEvtSet(epicsUInt32 e)
{
	if(e>255)
		std::out_of_range("Event code # out of range");

	WRITE8(base, InputMapFPBEvt(idx), (epicsUInt8)e);
}

epicsUInt32
MRMInput::backEvt() const
{
    return READ8(base, InputMapFPBEvt(idx));
}
