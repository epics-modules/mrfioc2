/*************************************************************************\
* Copyright (c) 2016 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdio.h>

#include <mrfCommonIO.h>

#include "mrmevgseq.h"
#include "evgMrm.h"

#include "evgRegMap.h"

#if defined(__rtems__)
#  define DEBUG(LVL, ARGS) do{if(SeqManagerDebug>=(LVL)) {printk ARGS ;}}while(0)
#elif defined(vxWorks)
#  define DEBUG(LVL, ARGS) do{}while(0)
#else
#  define DEBUG(LVL, ARGS) do{if(SeqManagerDebug>=(LVL)) {printf ARGS ;}}while(0)
#endif

EvgSeqManager::EvgSeqManager(evgMrm *owner, volatile epicsUInt8 *base)
    :SeqManager(owner->name()+":SEQMGR", TypeEVG)
    ,owner(owner)
    ,base(base)
{
    addHW(0, base + U32_SeqControl(0) , base + U32_SeqRamTS(0,0));
    addHW(1, base + U32_SeqControl(1) , base + U32_SeqRamTS(1,0));
}

EvgSeqManager::~EvgSeqManager() {}

double EvgSeqManager::getClkFreq() const
{
    return owner->getEvtClk()->getFrequency()*1e6;
}

//! Called from ISR
void EvgSeqManager::mapTriggerSrc(unsigned i, unsigned src)
{
    assert(i<=1);
    /* Input mapping special codes 0x02xxxxxx
     *
     * 0x20000000 - Disable
     * 0x020100xx - Front panel in
     * 0x020200xx - UV in
     * 0x020300xx - TB in
     */
    if((src&0xff000000)!=0x02000000) {
        DEBUG(0, ("EvgSeqManager::mapTriggerSrc unsupported %x\n", src));
        return;
    }

    InputType itype = (InputType)((src>>16)&0xff);
    unsigned idx = src&0xff;

    for(evgMrm::inputs_iterator it = owner->beginInputs(), end = owner->endInputs();
        it!=end; ++it)
    {
        bool match = it->first.second==itype && it->first.first==idx;
        evgInput *inp = it->second;
        epicsUInt32 map = inp->getSeqTrigMap();
        // set or clear sequencer trigger bit
        if(match)
            map |= 1u<<i;
        else
            map &= ~(1u<<i);
        inp->setSeqTrigMap(map);
    }
}

//! Called from ISR
epicsUInt32 EvgSeqManager::testStartOfSeq()
{
    // SoS for sequencer 0 is bit 8
    return (NAT_READ32(base, IrqFlag)>>8)&0x3;
}
