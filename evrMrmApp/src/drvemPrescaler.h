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
#include <evrMrmAPI.h>

class EVRMRM_API MRMPreScaler : public mrf::ObjectInst<MRMPreScaler,PreScaler>
{
    typedef mrf::ObjectInst<MRMPreScaler,PreScaler> base_t;
    OBJECT_DECL(MRMPreScaler);
    volatile unsigned char* base;

public:
    MRMPreScaler(const std::string& n, EVR& o,volatile unsigned char* b);
    virtual ~MRMPreScaler();

    /* no locking needed */
    virtual void lock() const OVERRIDE FINAL{};
    virtual void unlock() const OVERRIDE FINAL{};

    virtual epicsUInt32 prescaler() const OVERRIDE FINAL;
    virtual void setPrescaler(epicsUInt32) OVERRIDE FINAL;

    epicsUInt32 prescalerPhasOffs() const;
    void setPrescalerPhasOffs(epicsUInt32);
};

#endif // MRMEVRPRESCALER_H_INC
