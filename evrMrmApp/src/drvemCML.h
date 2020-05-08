/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef EVRMRMCMLSHORT_HPP_INC
#define EVRMRMCMLSHORT_HPP_INC

#include "evr/cml.h"

#include "configurationInfo.h"

class EVRMRM;

class MRMCML : public CML
{
public:
    enum outkind { typeCML, typeTG300, typeTG203 };

    MRMCML(const std::string&, unsigned char, EVRMRM&, outkind, formFactor);
    virtual ~MRMCML();

    virtual void lock() const OVERRIDE FINAL;
    virtual void unlock() const OVERRIDE FINAL;

    virtual cmlMode mode() const OVERRIDE FINAL;
    virtual void setMode(cmlMode) OVERRIDE FINAL;

    virtual bool enabled() const OVERRIDE FINAL;
    virtual void enable(bool) OVERRIDE FINAL;

    virtual bool inReset() const OVERRIDE FINAL;
    virtual void reset(bool) OVERRIDE FINAL;

    virtual bool powered() const OVERRIDE FINAL;
    virtual void power(bool) OVERRIDE FINAL;

    virtual epicsUInt32 freqMultiple() const OVERRIDE FINAL {return mult;}

    virtual double fineDelay() const OVERRIDE FINAL;
    virtual void setFineDelay(double) OVERRIDE FINAL;

    // For Frequency mode

    //! Trigger level
    virtual bool polarityInvert() const OVERRIDE FINAL;
    virtual void setPolarityInvert(bool) OVERRIDE FINAL;

    virtual epicsUInt32 countHigh() const OVERRIDE FINAL;
    virtual epicsUInt32 countLow () const OVERRIDE FINAL;
    virtual epicsUInt32 countInit () const OVERRIDE FINAL;
    virtual void setCountHigh(epicsUInt32) OVERRIDE FINAL;
    virtual void setCountLow (epicsUInt32) OVERRIDE FINAL;
    virtual void setCountInit (epicsUInt32) OVERRIDE FINAL;
    virtual double timeHigh() const OVERRIDE FINAL;
    virtual double timeLow () const OVERRIDE FINAL;
    virtual double timeInit () const OVERRIDE FINAL;
    virtual void setTimeHigh(double) OVERRIDE FINAL;
    virtual void setTimeLow (double) OVERRIDE FINAL;
    virtual void setTimeInit (double) OVERRIDE FINAL;

    // For Pattern mode

    virtual bool recyclePat() const OVERRIDE FINAL;
    virtual void setRecyclePat(bool) OVERRIDE FINAL;

    virtual epicsUInt32 lenPattern(pattern) const OVERRIDE FINAL;
    virtual epicsUInt32 lenPatternMax(pattern) const OVERRIDE FINAL;
    virtual epicsUInt32 getPattern(pattern, unsigned char*, epicsUInt32) const OVERRIDE FINAL;
    virtual void setPattern(pattern, const unsigned char*, epicsUInt32) OVERRIDE FINAL;

private:

    epicsUInt32 mult, wordlen;
    volatile unsigned char *base;
    unsigned char N;
    EVRMRM& owner;

    epicsUInt32 shadowEnable;

    epicsUInt32 *shadowPattern[5]; // 5 is wavefrom + 4x pattern
    epicsUInt32  shadowWaveformlength;

    void syncPattern(pattern);

    outkind kind;
};

#endif // EVRMRMCMLSHORT_HPP_INC
