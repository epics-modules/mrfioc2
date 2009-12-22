
#ifndef MRMEVRPRESCALER_H_INC
#define MRMEVRPRESCALER_H_INC

#include <evr/prescaler.h>

class MRMPreScaler : public PreScaler
{
  volatile unsigned char* base;

public:
  MRMPreScaler(EVR& o,volatile unsigned char* b):
    PreScaler(o),base(b) {};
  virtual ~MRMPreScaler(){};

  virtual epicsUInt32 prescaler() const;
  virtual void setPrescaler(epicsUInt32);
};

#endif // MRMEVRPRESCALER_H_INC
