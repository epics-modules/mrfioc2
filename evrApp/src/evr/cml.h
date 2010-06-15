
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

enum cmlShort {
  cmlShortRise=0,
  cmlShortHigh,
  cmlShortFall,
  cmlShortLow
};

class CML : public IOStatus
{
public:
  virtual ~CML()=0;

  virtual cmlMode mode() const=0;
  virtual void setMode(cmlMode)=0;

  virtual bool enabled() const=0;
  virtual void enable(bool)=0;

  virtual bool inReset() const=0;
  virtual void reset(bool)=0;

  virtual bool powered() const=0;
  virtual void power(bool)=0;

  // For original (20bit x 4) mode

  virtual epicsUInt32 pattern(cmlShort) const=0;
  virtual void patternSet(cmlShort, epicsUInt32)=0;

  // For Frequency mode

  //! Trigger level
  virtual bool polarityInvert() const=0;
  virtual void setPolarityInvert(bool)=0;

  virtual epicsUInt32 countHigh() const=0;
  virtual epicsUInt32 countLow () const=0;
  virtual void setCountHigh(epicsUInt32)=0;
  virtual void setCountLow (epicsUInt32)=0;

  // For Pattern mode (20bit x 2048) mode
  /* Patterns are given as an array of bytes.
   * each byte represents one bit in the result.
   * Lengths are given in bytes, but should be a
   * multiple of 20.
   */

  virtual epicsUInt32 lenPattern() const=0;
  virtual epicsUInt32 lenPatternMax() const=0;
  virtual void getPattern(unsigned char*, epicsUInt32*) const=0;
  virtual void setPattern(const unsigned char*, epicsUInt32)=0;

  // Helpers

  template<cmlShort I>
  epicsUInt32
  getPattern() const{return this->pattern(I);};

  template<cmlShort I>
  void
  setPattern(epicsUInt32 v){return this->patternSet(I,v);};

  void setModRaw(epicsUInt16 r){setMode((cmlMode)r);};
  epicsUInt16 modeRaw() const{return (epicsUInt16)mode();};
};

#endif // CMLSHORT_HPP_INC
