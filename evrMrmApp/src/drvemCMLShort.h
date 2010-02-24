
#ifndef EVRMRMCMLSHORT_HPP_INC
#define EVRMRMCMLSHORT_HPP_INC

#include "evr/cml_short.h"

class MRMCMLShort : public CMLShort
{
public:
  MRMCMLShort(unsigned char, volatile unsigned char *);
  virtual ~MRMCMLShort();

  virtual epicsUInt32 pattern(cmlShort) const;
  virtual void patternSet(cmlShort, epicsUInt32);

  virtual bool enabled() const;
  virtual void enable(bool);

  virtual bool inReset() const;
  virtual void reset(bool);

  virtual bool powered() const;
  virtual void power(bool);

private:
  volatile unsigned char *base;
  unsigned char N;
};

#endif // EVRMRMCMLSHORT_HPP_INC
