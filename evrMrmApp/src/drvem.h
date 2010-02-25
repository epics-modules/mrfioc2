
#ifndef EVRMRML_H_INC
#define EVRMRML_H_INC

#include "evr/evr.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <utility>

#include <dbScan.h>
#include <epicsTime.h>
#include <callback.h>

#include "drvemInput.h"
#include "drvemOutput.h"
#include "drvemPrescaler.h"
#include "drvemPulser.h"
#include "drvemCMLShort.h"

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

  virtual MRMPulser* pulser(epicsUInt32);
  virtual const MRMPulser* pulser(epicsUInt32) const;

  virtual MRMOutput* output(OutputType,epicsUInt32 o);
  virtual const MRMOutput* output(OutputType,epicsUInt32 o) const;

  virtual MRMInput* input(epicsUInt32 idx);
  virtual const MRMInput* input(epicsUInt32) const;

  virtual MRMPreScaler* prescaler(epicsUInt32);
  virtual const MRMPreScaler* prescaler(epicsUInt32) const;

  virtual MRMCMLShort* cmlshort(epicsUInt32 idx);
  virtual const MRMCMLShort* cmlshort(epicsUInt32) const;

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

  virtual void setSourceTS(TSSource);
  virtual TSSource SourceTS() const;
  virtual double clockTS() const;
  virtual void clockTSSet(double);
  virtual bool getTimeStamp(epicsTimeStamp *ts,epicsUInt32 event);
  virtual bool getTicks(epicsUInt32 *tks);

  virtual epicsUInt16 dbus() const;

  virtual void enableHeartbeat(bool);
  virtual IOSCANPVT heartbeatOccured();

  static void isr(void*);

  const int id;
  volatile unsigned char * const base;
private:
  double stampClock;

  epicsUInt32 count_recv_error;
  epicsUInt32 count_hardware_irq;
  epicsUInt32 count_heartbeat;

  IOSCANPVT IRQmappedEvent; // Hardware mapped IRQ
  IOSCANPVT IRQbufferReady; // Event log ready
  IOSCANPVT IRQheadbeat;    // Heartbeat rx
  IOSCANPVT IRQrxError;     // Rx link state change
  IOSCANPVT IRQfifofull;    // Fifo overflow

  typedef std::vector<MRMInput*> inputs_t;
  inputs_t inputs;

  typedef std::map<std::pair<OutputType,epicsUInt32>,MRMOutput*> outputs_t;
  outputs_t outputs;

  typedef std::vector<MRMPreScaler*> prescalers_t;
  prescalers_t prescalers;

  typedef std::vector<MRMPulser*> pulsers_t;
  pulsers_t pulsers;

  typedef std::vector<MRMCMLShort*> shortcmls_t;
  shortcmls_t shortcmls;

  // Called when FIFO not-full IRQ is received
  CALLBACK drain_fifo_cb;
  static void drain_fifo(CALLBACK*);

  // Called when the Event Log is stopped
  CALLBACK drain_log_cb;
  static void drain_log(CALLBACK*);

  // Periodic callback to detect when link state goes from down to up
  CALLBACK poll_link_cb;
  static void poll_link(CALLBACK*);
}; // class EVRMRM

#endif // EVRMRML_H_INC
