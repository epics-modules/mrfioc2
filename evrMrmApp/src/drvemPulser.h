/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2022 Cosylab d.d.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef EVRMRMPULSER_H_INC
#define EVRMRMPULSER_H_INC

#include <epicsMutex.h>

#include <evr/pulser.h>

class EVRMRM;

class MRMPulser : public mrf::ObjectInst<MRMPulser, Pulser>
{
    typedef mrf::ObjectInst<MRMPulser, Pulser> base_t;
    const epicsUInt32 id;
    EVRMRM& owner;

public:
    MRMPulser(const std::string& n, epicsUInt32,EVRMRM&);
    virtual ~MRMPulser(){};

    virtual void lock() const OVERRIDE FINAL;
    virtual void unlock() const OVERRIDE FINAL;

    virtual bool enabled() const OVERRIDE FINAL;
    virtual void enable(bool) OVERRIDE FINAL;

    virtual void softSet() OVERRIDE FINAL;
    virtual void softReset() OVERRIDE FINAL;

    virtual void setDelayRaw(epicsUInt32) OVERRIDE FINAL;
    virtual void setDelay(double) OVERRIDE FINAL;
    virtual epicsUInt32 delayRaw() const OVERRIDE FINAL;
    virtual double delay() const OVERRIDE FINAL;

    virtual void setWidthRaw(epicsUInt32) OVERRIDE FINAL;
    virtual void setWidth(double) OVERRIDE FINAL;
    virtual epicsUInt32 widthRaw() const OVERRIDE FINAL;
    virtual double width() const OVERRIDE FINAL;

    virtual epicsUInt32 prescaler() const OVERRIDE FINAL;
    virtual void setPrescaler(epicsUInt32) OVERRIDE FINAL;

    virtual epicsUInt32 psTrig() const OVERRIDE FINAL;
    virtual void setPSTrig(epicsUInt32) OVERRIDE FINAL;

    virtual bool polarityInvert() const OVERRIDE FINAL;
    virtual void setPolarityInvert(bool) OVERRIDE FINAL;

    epicsUInt32 enables() const;
    void setEnables(epicsUInt32 inps);
    epicsUInt32 masks() const;
    void setMasks(epicsUInt32 inps);

    virtual MapType::type mappedSource(epicsUInt32 src) const OVERRIDE FINAL;
    virtual void sourceSetMap(epicsUInt32 src,MapType::type action) OVERRIDE FINAL;

private:
    // bit map of which event #'s are mapped
    // used as a safty check to avoid overloaded mappings
    unsigned char mapped[256/8];

    void _map(epicsUInt8 evt)   {        mapped[evt/8] |=    1<<(evt%8);  }
    void _unmap(epicsUInt8 evt) {        mapped[evt/8] &= ~( 1<<(evt%8) );}
    bool _ismap(epicsUInt8 evt) const { return (mapped[evt/8]  &    1<<(evt%8)) != 0;  }
};

#endif // EVRMRMPULSER_H_INC
