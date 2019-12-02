/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef MRMEVRPRESCALER_H_INC
#define MRMEVRPRESCALER_H_INC

#include <evr/prescaler.h>

class epicsShareClass MRMPreScaler : public PreScaler
{
    volatile unsigned char* base;

public:
    MRMPreScaler(const std::string& n, EVR& o,volatile unsigned char* b):
            PreScaler(n,o),base(b) {};
    virtual ~MRMPreScaler(){};

    /* no locking needed */
    virtual void lock() const{};
    virtual void unlock() const{};

    virtual epicsUInt32 prescaler() const;
    virtual void setPrescaler(epicsUInt32);

    virtual epicsUInt32 prescalerPhasOffs() const;
    virtual void setPrescalerPhasOffs(epicsUInt32);
};

#endif // MRMEVRPRESCALER_H_INC
