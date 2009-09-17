
#ifndef EVR230PMC_H_INC
#define EVR230PMC_H_INC

#include "devLibPCI.h"

#include "evr.hpp"

class EVR230PMC : public EVR
{
public:
  EVR230PMC(const epicsPCIDevice*);
  virtual ~EVR230PMC();

  virtual bool enabled();
  virtual void enable(bool);

  virtual epicsUint32 eventClock();
  virtual epicsUint32 eventClockDiv();
  virtual void setEventClock(epicsUint32,epicsUint32);

  virtual IOSCANPVT recvError(){return scanRecvError;};
  virtual size_t recvErrorCount(){return countRecvError;};

  virtual void tsLatch();
  virtual void tsLatchReset();
  virtual epicsUInt32 tsLatchSec();
  virtual epicsUInt32 tsLatchCount();

protected:
  IOSCANPVT scanRecvError;
  size_t countRecvError;

  const epicsPCIDevice* dev;
};

#endif // EVR230PMC_H_INC
