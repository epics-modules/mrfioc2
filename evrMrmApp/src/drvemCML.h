
#ifndef EVRMRMCMLSHORT_HPP_INC
#define EVRMRMCMLSHORT_HPP_INC

#include "evr/cml.h"

class MRMCML : public CML
{
public:
  MRMCML(unsigned char, volatile unsigned char *);
  virtual ~MRMCML();

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
