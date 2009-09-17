
#ifndef EVR_HPP_INC
#define EVR_HPP_INC

#include <sharelib.h>
#include <epicsTypes.h>
#include <dbScan.h>

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

  /**@brief Representation for Hardware Event codes.
   *
   * Normal: 0-255 (0 means disabled)
   * Special: >255
   */
  typedef epicsUInt16 event;

  struct Type {
    enum type {
      Invalid,
      EVR_230_PMC,
      EVR_230_CPCI,
      EVR_230_VME,
      EVR_230_VMERF
    };
  
    static
    const char *name(type);
  };

public:
00100 10110 1101 000 01110 101 101
  EVR();
  virtual ~EVR()=0;

  virtual bool enabled()=0;
  virtual void enable(bool)=0;

  virtual epicsUint32 eventClock()=0;
  virtual epicsUint32 eventClockDiv()=0;
  //! Set Event clock rate and microsecond divider
  virtual void setEventClock(epicsUint32,epicsUint32)=0;

  virtual IOSCANPVT recvError()=0;
  virtual size_t recvErrorCount()=0;

  virtual void tsLatch()=0;
  virtual void tsLatchReset()=0;
  virtual epicsUInt32 tsLatchSec()=0;
  virtual epicsUInt32 tsLatchCount()=0;

  virtual epicsUint16 dbus()=0;

public:
  /**\defgroup constattrs Model specific constant attributes
   *
   *  Things which don't change at IOC runtime.
   */
  /*@{*/

  //! Hardware model
  Type::type model;

  //! Firmware Version
  epicsUInt32 version;

  /**@brief Total number of output channels of all types (front and rear).
   * Mapping between output channel id number (0->nOutputs-1) is board
   * specific.
   */
  size_t outputCount;
  size_t prescalerCount;
  size_t pulserCount;

  bool fifoPresent;
  bool dataBufPresent;

  /*@}*/
}; // class EVR

#endif // EVR_HPP_INC
