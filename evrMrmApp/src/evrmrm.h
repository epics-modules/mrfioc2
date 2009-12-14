
#ifndef EVRMRML_H_INC
#define EVRMRML_H_INC

#include "evr/evr.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <utility>

#include <dbScan.h>

#include "evrmrmoutput.h"
#include "evrmrmprescaler.h"

/**@brief Modular Register Map Event Receivers
 *
 * 
 */
class EVRMRM : public EVR
{
public:
  EVRMRM(int,volatile unsigned char*);

  virtual ~EVRMRM();

  virtual epicsUInt32 model() const;

  virtual epicsUInt32 version() const;

  virtual bool enabled() const;
  virtual void enable(bool v);

  virtual Pulser* pulser(epicsUInt32);
  virtual const Pulser* pulser(epicsUInt32) const;

  virtual MRMOutput* output(OutputType,epicsUInt32 o);
  virtual const MRMOutput* output(OutputType,epicsUInt32 o) const;

  virtual MRMPreScaler* prescaler(epicsUInt32);
  virtual const MRMPreScaler* prescaler(epicsUInt32) const;

  virtual bool specialMapped(epicsUInt32 code, epicsUInt32 func) const;
  virtual void specialSetMap(epicsUInt32 code, epicsUInt32 func,bool);
  virtual const char* idName(epicsUInt32 src) const;

  virtual double clock() const;
  virtual void clockSet(double);

  virtual bool pllLocked() const;

  virtual bool linkStatus() const;
  virtual IOSCANPVT linkChanged();
  virtual epicsUInt32 recvErrorCount() const;

  virtual epicsUInt32 uSecDiv() const;

  virtual epicsUInt32 tsDiv() const;
  virtual void setTsDiv(epicsUInt32);

  virtual void tsLatch();
  virtual void tsLatchReset();
  virtual epicsUInt32 tsLatchSec() const;
  virtual epicsUInt32 tsLatchCount() const;

  virtual epicsUInt16 dbus() const;

  virtual void enableHeartbeat(bool);
  virtual IOSCANPVT heartbeatOccured();

  static void isr(void*);
private:

  const int id;
  volatile unsigned char * const base;

  epicsUInt32 count_recv_error;
  epicsUInt32 count_hardware_irq;
  epicsUInt32 count_heartbeat;

  IOSCANPVT IRQmappedEvent;
  IOSCANPVT IRQbufferReady;
  IOSCANPVT IRQheadbeat;
  IOSCANPVT IRQrxError;

  typedef std::map<std::pair<OutputType,epicsUInt32>,MRMOutput*> outputs_t;
  outputs_t outputs;

  typedef std::vector<MRMPreScaler*> prescalers_t;
  prescalers_t prescalers;
}; // class EVRMRM

#endif // EVRMRML_H_INC
