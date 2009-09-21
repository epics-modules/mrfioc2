
#ifndef PRESCALER_HPP_INC
#define PRESCALER_HPP_INC

#include <epicsTypes.h>

class PreScaler : public IOStatus
{
public:
  virtual ~PreScaler()=0;

  virtual epicsUint32 prescaler()=0;
  virtual void setPrescaler(epicsUint32)=0;
};

#endif // PRESCALER_HPP_INC
