
#ifndef EVR_HPP_INC
#define EVR_HPP_INC

#include "evr/pulser.h"
#include "evr/output.h"
#include "evr/prescaler.h"

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
  virtual epicsUInt32 model()=0;

  //! Firmware Version
  virtual epicsUInt32 version()=0;

  /**\defgroup ena Enable/disable pulser output.
   */
  /*@{*/
  virtual bool enabled()=0;
  virtual void enable(bool)=0;
  /*@}*/


  /**\defgroup pll Module reference clock
   *
   *  Conversion between frequency and clock word
   *  Should happen outside.
   */
  /*@{*/
  virtual epicsUInt32 pllClock()=0;
  virtual void pllSetClock(epicsUInt32)=0;
  //! Test for PLL error conditions
  virtual bool pllLocked()=0;
  virtual IOSCANPVT pllChanged()=0;
  /*@}*/

  //! Pulser id number is device specific
  virtual Pulser* pulser(epicsUInt32)=0;

  //! Output id number is device specific
  virtual Output* output(epicsUInt32)=0;

  //! Prescaler id number is device specific
  virtual PreScaler* prescaler(epicsUInt32)=0;

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
  virtual void setEventClock(epicsUInt32)=0;
  virtual void setEventClockDiv(epicsUInt32)=0;

  virtual IOSCANPVT recvError()=0;
  virtual epicsUInt32 recvErrorCount()=0;

  virtual void tsLatch()=0;
  virtual void tsLatchReset()=0;
  virtual epicsUInt32 tsLatchSec()=0;
  virtual epicsUInt32 tsLatchCount()=0;

  virtual epicsUInt16 dbus()=0;

  virtual void enableHeartbeat(bool)=0;
  virtual IOSCANPVT heartbeatOccured()=0;

}; // class EVR

#endif // EVR_HPP_INC
