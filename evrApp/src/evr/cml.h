/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef CMLSHORT_HPP_INC
#define CMLSHORT_HPP_INC

#include "evr/util.h"

#include <epicsTypes.h>

enum cmlMode {
  cmlModeOrig,
  cmlModeFreq,
  cmlModePattern,
  cmlModeInvalid
};

class CML : public IOStatus
{
public:
  enum pattern {
    patternWaveform,
    patternRise,
    patternHigh,
    patternFall,
    patternLow
  };

  virtual ~CML()=0;

  virtual cmlMode mode() const=0;
  virtual void setMode(cmlMode)=0;

  virtual bool enabled() const=0;
  virtual void enable(bool)=0;

  virtual bool inReset() const=0;
  virtual void reset(bool)=0;

  virtual bool powered() const=0;
  virtual void power(bool)=0;

  //! Speed of CML clock as a multiple of the event clock
  virtual epicsUInt32 freqMultiple() const=0;

  //! delay by fraction of one event clock period.  Units of sec
  virtual double fineDelay() const=0;
  virtual void setFineDelay(double)=0;

  // For Frequency mode

  //! Trigger level
  virtual bool polarityInvert() const=0;
  virtual void setPolarityInvert(bool)=0;

  virtual epicsUInt32 countHigh() const=0;
  virtual epicsUInt32 countLow () const=0;
  virtual void setCountHigh(epicsUInt32)=0;
  virtual void setCountLow (epicsUInt32)=0;
  virtual double timeHigh() const=0;
  virtual double timeLow () const=0;
  virtual void setTimeHigh(double)=0;
  virtual void setTimeLow (double)=0;

  // waveform mode

  virtual bool recyclePat() const=0;
  virtual void setRecyclePat(bool)=0;

  // waveform and 4x pattern modes

  virtual epicsUInt32 lenPattern(pattern) const=0;
  virtual epicsUInt32 lenPatternMax(pattern) const=0;
  virtual epicsUInt32 getPattern(pattern, unsigned char*, epicsUInt32) const=0;
  virtual void setPattern(pattern, const unsigned char*, epicsUInt32)=0;

  // Helpers

  template<pattern P>
  epicsUInt32 lenPattern() const{return lenPattern(P);}
  template<pattern P>
  epicsUInt32 lenPatternMax() const{return lenPatternMax(P);}

  template<pattern P>
  epicsUInt32
  getPattern(unsigned char* b, epicsUInt32 l) const{return this->getPattern(P,b,l);};

  template<pattern P>
  void
  setPattern(const unsigned char* b, epicsUInt32 l){this->setPattern(P,b,l);};

  void setModRaw(epicsUInt16 r){setMode((cmlMode)r);};
  epicsUInt16 modeRaw() const{return (epicsUInt16)mode();};
};

#endif // CMLSHORT_HPP_INC
