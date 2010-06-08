
#ifndef CMLSHORT_HPP_INC
#define CMLSHORT_HPP_INC

#include "evr/util.h"

#include <epicsTypes.h>

enum cmlShort {
  cmlShortRise=0,
  cmlShortHigh,
  cmlShortFall,
  cmlShortLow
};

/**@brief Configuration for the short (20bit x 4) CML patterns
 */
class CML : public IOStatus
{
public:
  virtual ~CML()=0;

  virtual epicsUInt32 pattern(cmlShort) const=0;
  virtual void patternSet(cmlShort, epicsUInt32)=0;

  virtual bool enabled() const=0;
  virtual void enable(bool)=0;

  virtual bool inReset() const=0;
  virtual void reset(bool)=0;

  virtual bool powered() const=0;
  virtual void power(bool)=0;

  template<cmlShort I>
  epicsUInt32
  getPattern() const{return this->pattern(I);};

  template<cmlShort I>
  void
  setPattern(epicsUInt32 v){return this->patternSet(I,v);};
};

#endif // CMLSHORT_HPP_INC
