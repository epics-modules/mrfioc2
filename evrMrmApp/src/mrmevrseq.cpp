/*************************************************************************\
* Copyright (c) 2016 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdio.h>

#include <mrfCommonIO.h>

#include "mrmevrseq.h"

#include "drvem.h"
#include "evrRegMap.h"

#if defined(__rtems__)
#  define DEBUG(LVL, ARGS) do{if(SeqManagerDebug>=(LVL)) {printk ARGS ;}}while(0)
#elif defined(vxWorks)
#  define DEBUG(LVL, ARGS) do{}while(0)
#else
#  define DEBUG(LVL, ARGS) do{if(SeqManagerDebug>=(LVL)) {printf ARGS ;}}while(0)
#endif

EvrSeqManager::EvrSeqManager(EVRMRM *owner)
    :SeqManager(owner->name()+":SEQMGR", TypeEVR)
    ,owner(owner)
{
    addHW(0, owner->base + U32_SeqControl(0) , owner->base + U32_SeqRamTS(0,0));
}

EvrSeqManager::~EvrSeqManager() {}

double EvrSeqManager::getClkFreq() const
{
    return owner->clock();
}

//! Called from ISR
void EvrSeqManager::mapTriggerSrc(unsigned i, unsigned src)
{
    assert(i==0);
    DEBUG(0, ("EvrSeqManager::mapTriggerSrc external trigger mappings unsupported %x\n", src));
}

//! Called from ISR
epicsUInt32 EvrSeqManager::testStartOfSeq()
{
    // SoS for sequencer 0 is bit 8
    return (NAT_READ32(owner->base, IRQFlag)>>8)&0x1;
}
