
#ifndef EVRMRML_H_INC
#define EVRMRML_H_INC

#include "evr/evr.h"

#include <string>
#include <vector>
#include <set>
#include <list>
#include <map>
#include <utility>

#include <dbScan.h>
#include <epicsTime.h>
#include <callback.h>
#include <epicsMutex.h>

#include "drvemInput.h"
#include "drvemOutput.h"
#include "drvemPrescaler.h"
#include "drvemPulser.h"
#include "drvemCML.h"
#include "drvemRxBuf.h"

#include "mrmDataBufTx.h"

struct eventCode {
  epicsUInt8 code; // constant

  // For efficiency events will only
  // be mapped into the FIFO when this
  // counter is non-zero.
  size_t interested;

  epicsUInt32 last_sec;
  epicsUInt32 last_evt;

  IOSCANPVT occured;

  typedef std::list<CALLBACK*> notifiees_t;
  notifiees_t notifiees;
};

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

  virtual MRMCML* cml(epicsUInt32 idx);
  virtual const MRMCML* cml(epicsUInt32) const;

  virtual bool specialMapped(epicsUInt32 code, epicsUInt32 func) const;
  virtual void specialSetMap(epicsUInt32 code, epicsUInt32 func,bool);

  virtual double clock() const
        {SCOPED_LOCK(shadowLock);return eventClock;}
  virtual void clockSet(double);

  virtual bool pllLocked() const;

  virtual bool linkStatus() const;
  virtual IOSCANPVT linkChanged(){return IRQrxError;}
  virtual epicsUInt32 recvErrorCount() const{return count_recv_error;}

  virtual epicsUInt32 uSecDiv() const;

  virtual epicsUInt32 tsDiv() const
        {SCOPED_LOCK(shadowLock);return shadowCounterPS;}

  virtual void setSourceTS(TSSource);
  virtual TSSource SourceTS() const
        {SCOPED_LOCK(shadowLock);return shadowSourceTS;}
  virtual double clockTS() const;
  virtual void clockTSSet(double);
  virtual bool interestedInEvent(epicsUInt32 event,bool set);

  virtual bool TimeStampValid() const
        {SCOPED_LOCK(shadowLock);return timestampValid;}
  virtual IOSCANPVT TimeStampValidEvent(){return timestampValidChange;}

  virtual bool getTimeStamp(epicsTimeStamp *ts,epicsUInt32 event);
  virtual bool getTicks(epicsUInt32 *tks);
  virtual IOSCANPVT eventOccurred(epicsUInt32 event);
  virtual void eventNotityAdd(epicsUInt32, CALLBACK*);
  virtual void eventNotityDel(epicsUInt32, CALLBACK*);

  virtual epicsUInt16 dbus() const;

  virtual epicsUInt32 heartbeatTIMOCount() const{return count_heartbeat;}
  virtual IOSCANPVT heartbeatTIMOOccured(){return IRQheartbeat;}

  virtual epicsUInt32 FIFOFullCount() const{return count_FIFO_overflow;}

  static void isr(void*);

  const int id;
  volatile unsigned char * const base;
  mrmDataBufTx buftx;
  mrmBufRx bufrx;
private:

  // Set by ISR
  volatile epicsUInt32 count_recv_error;
  volatile epicsUInt32 count_hardware_irq;
  volatile epicsUInt32 count_heartbeat;

  // Guarded by shadowLock
  epicsUInt32 count_FIFO_overflow;

  // scanIoRequest() from ISR or callback
  IOSCANPVT IRQmappedEvent; // Hardware mapped IRQ
  IOSCANPVT IRQbufferReady; // Event log ready
  IOSCANPVT IRQheartbeat;   // Heartbeat timeout
  IOSCANPVT IRQrxError;     // Rx link state change
  IOSCANPVT IRQfifofull;    // Fifo overflow

  // Software events
  IOSCANPVT timestampValidChange;

  // Set by ctor, not changed after

  typedef std::vector<MRMInput*> inputs_t;
  inputs_t inputs;

  typedef std::map<std::pair<OutputType,epicsUInt32>,MRMOutput*> outputs_t;
  outputs_t outputs;

  typedef std::vector<MRMPreScaler*> prescalers_t;
  prescalers_t prescalers;

  typedef std::vector<MRMPulser*> pulsers_t;
  pulsers_t pulsers;

  typedef std::vector<MRMCML*> shortcmls_t;
  shortcmls_t shortcmls;

  // Called when FIFO not-full IRQ is received
  CALLBACK drain_fifo_cb;
  static void drain_fifo(CALLBACK*);

  // Take lock when accessing interest counter or TS members
  epicsMutex events_lock; // really should be a rwlock
  eventCode events[256];

  // Buffer received
  CALLBACK data_rx_cb;

  // Called when the Event Log is stopped
  CALLBACK drain_log_cb;
  static void drain_log(CALLBACK*);

  // Periodic callback to detect when link state goes from down to up
  CALLBACK poll_link_cb;
  static void poll_link(CALLBACK*);

  /** Guards access to all data members not accessed by ISR
   */
  mutable epicsMutex shadowLock;

  // Set by clockTSSet() with IRQ disabled
  double stampClock;
  TSSource shadowSourceTS;
  epicsUInt32 shadowCounterPS;
  double eventClock; //!< Stored in Hz

  bool timestampValid;
  epicsUInt32 lastInvalidTimestamp;
  epicsUInt32 lastValidTimestamp;
  CALLBACK seconds_tick_cb;
  static void seconds_tick(CALLBACK*);

  // bit map of which event #'s are mapped
  // used as a safty check to avoid overloaded mappings
  epicsUInt32 _mapped[256];

  void _map(epicsUInt8 evt, epicsUInt8 func)   { _mapped[evt] |=    1<<(func);  }
  void _unmap(epicsUInt8 evt, epicsUInt8 func) { _mapped[evt] &= ~( 1<<(func) );}
  bool _ismap(epicsUInt8 evt, epicsUInt8 func) const { return _mapped[evt] & 1<<(func); }
}; // class EVRMRM

#endif // EVRMRML_H_INC
