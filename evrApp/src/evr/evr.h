
#ifndef EVR_HPP_INC
#define EVR_HPP_INC

#include "evr/util.h"

#include <epicsTypes.h>

class Pulser;
class Output;
class PreScaler;

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

  virtual ~EVR()=0;

  //! Hardware model
  virtual epicsUInt32 model() const=0;

  //! Firmware Version
  virtual epicsUInt32 version() const=0;

  /**\defgroup ena Enable/disable pulser output.
   */
  /*@{*/
  virtual bool enabled() const=0;
  virtual void enable(bool)=0;
  /*@}*/

  //! Pulser id number is device specific
  virtual Pulser* pulser(epicsUInt32)=0;
  virtual const Pulser* pulser(epicsUInt32) const=0;

  //! Output id number is device specific
  virtual Output* output(epicsUInt32)=0;
  virtual const Output* output(epicsUInt32) const=0;

  //! Prescaler id number is device specific
  virtual PreScaler* prescaler(epicsUInt32)=0;
  virtual const PreScaler* prescaler(epicsUInt32) const=0;

  /** Hook to handle general event mapping table manipulation.
   *  Allows 'special' events only (ie heartbeat, log, led, etc)
   *  Normal mappings (pulsers, outputs) must be made through the
   *  appropriate class (Pulser, Output).
   *
   * Note: this is one place where Device Support will have some depth.
   */
  virtual bool specialMapped(epicsUInt32 code, epicsUInt32 func) const=0;
  virtual void specialSetMap(epicsUInt32 code, epicsUInt32 func,bool set)=0;

  /** Return a human readable string describing 'src'.
   * 'src' can the mapping code for any event code, distributed bus bit,
   * prescaler, pulser, or input.
   */
  virtual const char* idName(epicsUInt32 src) const=0;

  /**\defgroup pll Module reference clock
   *
   *  Conversion between frequency and clock word
   *  Should happen outside.
   */
  /*@{*/
  virtual epicsUInt32 pllCtrl() const=0;
  virtual void pllSetCtrl(epicsUInt32)=0;
  //! Test for PLL error condition
  virtual bool pllLocked() const=0;
  virtual IOSCANPVT pllChanged()=0;
  /*@}*/

  virtual epicsUInt32 eventClockDiv() const=0;
  virtual void setEventClockDiv(epicsUInt32)=0;

  virtual IOSCANPVT recvError()=0;
  virtual epicsUInt32 recvErrorCount() const=0;

  virtual epicsUInt32 tsDiv() const=0;
  virtual void setTsDiv(epicsUInt32)=0;

  virtual void tsLatch()=0;
  virtual void tsLatchReset()=0;
  virtual epicsUInt32 tsLatchSec() const=0;
  virtual epicsUInt32 tsLatchCount() const=0;

  virtual epicsUInt16 dbus() const=0;

  virtual void enableHeartbeat(bool)=0;
  virtual IOSCANPVT heartbeatOccured()=0;

}; // class EVR

#endif // EVR_HPP_INC
