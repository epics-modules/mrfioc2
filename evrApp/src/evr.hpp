
#ifndef EVR_HPP_INC
#define EVR_HPP_INC

#include <cstdlib>

#include <shareLib.h>
#include <epicsTypes.h>
#include <dbScan.h>

/** A general way to notify device support of status changes
 *
 * Use to impliment get_ioint_info()
 */
class IOStatus
{
public:
  virtual IOSCANPVT statusChange(){return NULL;};
};

struct TimeUnits {
  enum type {
    Tick,
    Micro,
    Milli,
    Sec
  };
};

//! Three state logic
struct TSL {
  enum type {
    Float, //!< Driver off
    Low,
    High
  };
};

struct MapType {
  enum type {
    None,
    Trigger,
    Reset,
    Set
  };
};

class Pulser : public IOStatus
{
public:
  virtual ~Pulser()=0;

  virtual bool enabled()=0;
  virtual void enable(bool)=0;

  virtual delay(epicsUInt32)=0;
  virtual epicsUInt32 setDelay()=0;
  virtual delayUnit(TimeUnits::type)=0;
  virtual TimeUnits::type setDelayUnit()=0;

  virtual width(epicsUInt32)=0;
  virtual epicsUInt32 setWidth()=0;
  virtual widthUnit(TimeUnits::type)=0;
  virtual TimeUnits::type setWidthUnit()=0;

  virtual epicsUint32 prescaler()=0;
  virtual void setPrescaler(epicsUint32)=0;

  virtual bool polarityNorm()=0;
  virtual void setPolarityNorm(bool)=0;

  // Manually set state
  virtual TSL::type state()=0;
  // Settings other than TSL::Float temporarily disable all event mappings
  virtual void setState(TSL::type)=0;

  /**
   * Control which source(s) effect this pulser.
   *
   * Meaning of source id number is device specific (0 = not mapped).
   *
   * Note: this is one place where Device Support will have some depth.
   */
  virtual MapType::type mappedSource(epicsUInt32)=0;
  virtual void sourceSetMap(epicsUInt32,MapType::type)=0;
  virtual const char* sourceName(epicsUInt32)=0;

  virtual const char* mapDesc(epicsUInt32,MapType::type);//!< Default uses sourceName()
};

class Output : public IOStatus
{
public:
  virtual ~Output()=0;

  /**
   * Control which source(s) effect this pulser.
   *
   * Meaning of source id number is device specific (0 = not mapped).
   *
   * Note: this is one place where Device Support will have some depth.
   */
  virtual bool source(epicsUInt32)=0;
  virtual void setSource(epicsUInt32, bool)=0;
  virtual const char* sourceName(epicsUInt32)=0;

  // Manually set state
  virtual TSL::type state()=0;
  // Settings other than TSL::Float temporarily disable all sources
  virtual void setState(TSL::type)=0;
};

class PreScaler : public IOStatus
{
public:
  virtual ~PreScaler()=0;

  virtual epicsUint32 prescaler()=0;
  virtual void setPrescaler(epicsUint32)=0;
};


/**@brief Base interface for EVRs.
 *
 * This is the interface which the generic EVR device support
 * will use to interact with an EVR.
 *
 * Device support can use one of the functions returning IOSCANPVT
 * to impliment get_ioint_info().
 */
class EVR
{
public:

  EVR();
  virtual ~EVR()=0;

  //! Hardware model
  virtual epicsUInt32 model()=0;

  //! Firmware Version
  virtual epicsUInt32 version()=0;

  virtual bool enabled()=0;
  virtual void enable(bool)=0;

  /** Conversion between frequency and clock word
   *  Should happen outside.
   */
  virtual epicsUInt32 pllClock()=0;
  virtual void pllSetClock(epicsUInt32)=0;
  virtual bool pllLocked()=0;
  virtual IOSCANPVT pllChanged()=0;

  //! Pulser id number is device specific
  virtual Pulser* pulser(epicsUInt32)=0;

  //! Output id number is device specific
  virtual Output* output(epicsUInt32)=0;

  //! Prescaler id number is device specific
  virtual Prescaler* prescaler(epicsUInt32)=0;

  /** Hook to handle general event mapping table manipulation.
   *  Allows 'special' events only (ie heartbeat, log, led, etc)
   *  Normal mappings (pulsers, outputs) must be made through the
   *  appropriate class (Pulser, Output).
   *
   * Note: this is one place where Device Support will have some depth.
   */
  virtual bool specialMapped(epicsUInt32 code, epicsUInt32 func)=0;
  virtual void specialSetMap(epicsUInt32 code, epicsUInt32 func)=0;

  virtual epicsUInt32 eventClock()=0;
  virtual epicsUInt32 eventClockDiv()=0;
  //! Set Event clock rate and microsecond divider
  virtual void setEventClock(epicsUInt32,epicsUInt32)=0;
  virtual void setEventClock(epicsUInt32,epicsUInt32)=0;

  virtual IOSCANPVT recvError()=0;
  virtual size_t recvErrorCount()=0;

  virtual void tsLatch()=0;
  virtual void tsLatchReset()=0;
  virtual epicsUInt32 tsLatchSec()=0;
  virtual epicsUInt32 tsLatchCount()=0;

  virtual epicsUInt16 dbus()=0;

  virtual void enableHeartbeat(bool)=0;
  virtual IOSCANPVT heartbeatOccured()=0;

}; // class EVR

#endif // EVR_HPP_INC
