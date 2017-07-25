/*************************************************************************\
* Copyright (c) 2016 Michael Davidsaver
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef MRMEVRSEQ_H
#define MRMEVRSEQ_H

#include "mrmSeq.h"

class EVRMRM;

class EvrSeqManager : public SeqManager
{
public:
    EvrSeqManager(EVRMRM *owner);
    virtual ~EvrSeqManager();

    virtual double getClkFreq() const;

    virtual void mapTriggerSrc(unsigned i, unsigned src);

    virtual epicsUInt32 testStartOfSeq();

private:
    EVRMRM * const owner;
};

#endif // MRMEVRSEQ_H
