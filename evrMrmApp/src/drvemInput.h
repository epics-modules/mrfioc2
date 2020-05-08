/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef EVRMRMINPUT_H_INC
#define EVRMRMINPUT_H_INC

#include <cstdlib>
#include "evr/input.h"

/**
 * Controls only the single output mapping register
 * shared by all (except CML) outputs on MRM EVRs.
 *
 * This class is reused by other subunits which
 * have identical mapping registers.
 */
class MRMInput : public Input
{
public:
    MRMInput(const std::string& n, volatile unsigned char *, size_t);
    virtual ~MRMInput(){};

    /* no locking needed */
    virtual void lock() const OVERRIDE FINAL {};
    virtual void unlock() const OVERRIDE FINAL {};

    virtual void dbusSet(epicsUInt16) OVERRIDE FINAL;
    virtual epicsUInt16 dbus() const OVERRIDE FINAL;

    virtual void levelHighSet(bool) OVERRIDE FINAL;
    virtual bool levelHigh() const OVERRIDE FINAL;

    virtual void edgeRiseSet(bool) OVERRIDE FINAL;
    virtual bool edgeRise() const OVERRIDE FINAL;

    virtual void extModeSet(TrigMode) OVERRIDE FINAL;
    virtual TrigMode extMode() const OVERRIDE FINAL;

    virtual void extEvtSet(epicsUInt32) OVERRIDE FINAL;
    virtual epicsUInt32 extEvt() const OVERRIDE FINAL;

    virtual void backModeSet(TrigMode) OVERRIDE FINAL;
    virtual TrigMode backMode() const OVERRIDE FINAL;

    virtual void backEvtSet(epicsUInt32) OVERRIDE FINAL;
    virtual epicsUInt32 backEvt() const OVERRIDE FINAL;

private:
    volatile unsigned char * const base;
    const size_t idx;
};


#endif // EVRMRMINPUT_H_INC
