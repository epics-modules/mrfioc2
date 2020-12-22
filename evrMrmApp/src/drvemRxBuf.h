/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef DRVEMRXBUF_H
#define DRVEMRXBUF_H

#include <callback.h>

#include "bufrxmgr.h"

class EVRMRM_API mrmBufRx : public bufRxManager
{
public:
    mrmBufRx(const std::string&, volatile void *base,unsigned int qdepth, unsigned int bsize=0);
    virtual ~mrmBufRx();

    /* no locking needed */
    virtual void lock() const OVERRIDE FINAL {};
    virtual void unlock() const OVERRIDE FINAL {};

    virtual bool dataRxEnabled() const OVERRIDE FINAL;
    virtual void dataRxEnable(bool) OVERRIDE FINAL;

    static void drainbuf(callbackPvt*);

protected:
    volatile unsigned char * const base;
};

#endif // DRVEMRXBUF_H
