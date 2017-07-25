/*************************************************************************\
* Copyright (c) 2016 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef MRMEVGSEQ_H
#define MRMEVGSEQ_H

#include "mrmSeq.h"

class evgMrm;

class EvgSeqManager : public SeqManager
{
public:
    EvgSeqManager(evgMrm *owner, volatile epicsUInt8 *base);
    virtual ~EvgSeqManager();

    virtual double getClkFreq() const;

    virtual void mapTriggerSrc(unsigned i, unsigned src);

    virtual epicsUInt32 testStartOfSeq();

private:
    evgMrm * const owner;
    volatile epicsUInt8 *base;
};

#endif // MRMEVGSEQ_H
