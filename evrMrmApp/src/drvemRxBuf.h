/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef DRVEMRXBUF_H
#define DRVEMRXBUF_H

#include <callback.h>

#include "bufrxmgr.h"

class epicsShareClass mrmBufRx : public bufRxManager
{
public:
    mrmBufRx(const std::string&, volatile void *base,unsigned int qdepth, unsigned int bsize=0);
    virtual ~mrmBufRx();

    /* no locking needed */
    virtual void lock() const{};
    virtual void unlock() const{};

    virtual bool dataRxEnabled() const;
    virtual void dataRxEnable(bool);

    static void drainbuf(CALLBACK*);

protected:
    volatile unsigned char * const base;
};

#endif // DRVEMRXBUF_H
