/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
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
    MRMInput(volatile unsigned char *, size_t);
    virtual ~MRMInput(){};

    virtual void dbusSet(epicsUInt16);
    virtual epicsUInt16 dbus() const;

    virtual void levelHighSet(bool);
    virtual bool levelHigh() const;

    virtual void edgeRiseSet(bool);
    virtual bool edgeRise() const;

    virtual void extModeSet(TrigMode);
    virtual TrigMode extMode() const;

    virtual void extEvtSet(epicsUInt32);
    virtual epicsUInt32 extEvt() const;

    virtual void backModeSet(TrigMode);
    virtual TrigMode backMode() const;

    virtual void backEvtSet(epicsUInt32);
    virtual epicsUInt32 backEvt() const;

private:
    volatile unsigned char *base;
    size_t idx;
};


#endif // EVRMRMINPUT_H_INC
