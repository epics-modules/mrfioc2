
#ifndef EVRMRMPULSER_H_INC
#define EVRMRMPULSER_H_INC

#include <evr/pulser.h>

class EVRMRM;

class MRMPulser : public Pulser
{
    const epicsUInt32 id;
    EVRMRM& owner;

public:
    MRMPulser(epicsUInt32,EVRMRM&);
    virtual ~MRMPulser(){};

    virtual bool enabled() const;
    virtual void enable(bool);

    virtual void setDelayRaw(epicsUInt32);
    virtual void setDelay(double);
    virtual epicsUInt32 delayRaw() const;
    virtual double delay() const;

    virtual void setWidthRaw(epicsUInt32);
    virtual void setWidth(double);
    virtual epicsUInt32 widthRaw() const;
    virtual double width() const;

    virtual epicsUInt32 prescaler() const;
    virtual void setPrescaler(epicsUInt32);

    virtual bool polarityInvert() const;
    virtual void setPolarityInvert(bool);

    virtual MapType::type mappedSource(epicsUInt32 src) const;
    virtual void sourceSetMap(epicsUInt32 src,MapType::type action);

    virtual const char* sourceName(epicsUInt32 src) const;

private:
    // bit map of which event #'s are mapped
    // used as a safty check to avoid overloaded mappings
    unsigned char mapped[256/8];

    void _map(epicsUInt8 evt)   {        mapped[evt/8] |=    1<<(evt%8);  }
    void _unmap(epicsUInt8 evt) {        mapped[evt/8] &= ~( 1<<(evt%8) );}
    bool _ismap(epicsUInt8 evt) const { return mapped[evt/8]  &    1<<(evt%8);  }
};

#endif // EVRMRMPULSER_H_INC
