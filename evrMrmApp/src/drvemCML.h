/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef EVRMRMCMLSHORT_HPP_INC
#define EVRMRMCMLSHORT_HPP_INC

#include "evr/cml.h"

class EVRMRM;

class MRMCML : public CML
{
public:
    MRMCML(unsigned char, EVRMRM&);
    virtual ~MRMCML();

    virtual cmlMode mode() const{return shadowMode;}
    virtual void setMode(cmlMode);

    virtual bool enabled() const{return shadowEnable;}
    virtual void enable(bool);

    virtual bool inReset() const;
    virtual void reset(bool);

    virtual bool powered() const;
    virtual void power(bool);

    // For original (Classic) mode

    virtual epicsUInt32 pattern(cmlShort) const;
    virtual void patternSet(cmlShort, epicsUInt32);

    // For Frequency mode

    //! Trigger level
    virtual bool polarityInvert() const;
    virtual void setPolarityInvert(bool);

    virtual epicsUInt32 countHigh() const;
    virtual epicsUInt32 countLow () const;
    virtual void setCountHigh(epicsUInt32);
    virtual void setCountLow (epicsUInt32);
    virtual double timeHigh() const;
    virtual double timeLow () const;
    virtual void setTimeHigh(double);
    virtual void setTimeLow (double);

    // For Pattern mode

    virtual bool recyclePat() const;
    virtual void setRecyclePat(bool);

    virtual epicsUInt32 lenPattern() const;
    virtual epicsUInt32 lenPatternMax() const;
    virtual void getPattern(unsigned char*, epicsUInt32*) const;
    virtual void setPattern(const unsigned char*, epicsUInt32);

private:
    volatile unsigned char *base;
    unsigned char N;
    EVRMRM& owner;

    bool shadowEnable;
    cmlMode shadowMode;
};

#endif // EVRMRMCMLSHORT_HPP_INC
