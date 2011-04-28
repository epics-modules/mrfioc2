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

#include "evrRegMap.h"

class EVRMRM;

class MRMCML : public CML
{
public:
    enum outkind { typeCML, typeTG300, typeTG203 };

    MRMCML(const std::string&, unsigned char, EVRMRM&, outkind, evrForm);
    virtual ~MRMCML();

    virtual cmlMode mode() const;
    virtual void setMode(cmlMode);

    virtual bool enabled() const;
    virtual void enable(bool);

    virtual bool inReset() const;
    virtual void reset(bool);

    virtual bool powered() const;
    virtual void power(bool);

    virtual epicsUInt32 freqMultiple() const{return mult;}

    virtual double fineDelay() const;
    virtual void setFineDelay(double);

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

    virtual epicsUInt32 lenPattern(pattern) const;
    virtual epicsUInt32 lenPatternMax(pattern) const;
    virtual epicsUInt32 getPattern(pattern, unsigned char*, epicsUInt32) const;
    virtual void setPattern(pattern, const unsigned char*, epicsUInt32);

private:
    epicsUInt32 getPattern20(pattern, unsigned char*, epicsUInt32) const;
    void setPattern20(pattern, const unsigned char*, epicsUInt32);
    epicsUInt32 getPattern40(pattern, unsigned char*, epicsUInt32) const;
    void setPattern40(pattern, const unsigned char*, epicsUInt32);

    epicsUInt32 mult;
    volatile unsigned char *base;
    unsigned char N;
    EVRMRM& owner;

    epicsUInt32 shadowEnable;

    outkind kind;
};

#endif // EVRMRMCMLSHORT_HPP_INC
