
#ifndef PULSER_HPP_INC
#define PULSER_HPP_INC

#include "evr/util.hpp"

#include <epicsTypes.h>

struct MapType {
  enum type {
    None,
    Trigger,
    Reset,
    Set
  };
};

/**@brief A programmable delay unit.
 *
 * A Pulser has two modes of operation: Triggered,
 * and gated.  In triggered mode an event starts a count
 * down (delay) to the start of the pulse.  A second
 * counter (width) then runs until the end of the pulse.
 * Gated mode has two event codes.  One is sets the output
 * high and the second resets the output low.
 */
class Pulser : public IOStatus
{
public:
  virtual ~Pulser()=0;

  /**\defgroup ena Enable/disable pulser output.
   */
  /*@{*/
  virtual bool enabled()=0;
  virtual void enable(bool)=0;
  /*@}*/

  /**\defgroup dly Set triggered mode delay length.
   *
   * Units of event clock period.
   */
  /*@{*/
  virtual delay(epicsUInt32)=0;
  virtual epicsUInt32 setDelay()=0;
  //virtual delayUnit(TimeUnits::type)=0;
  //virtual TimeUnits::type setDelayUnit()=0;
  /*@}*/

  /**\defgroup wth Set triggered mode width
   *
   * Units of event clock period.
   */
  /*@{*/
  virtual width(epicsUInt32)=0;
  virtual epicsUInt32 setWidth()=0;
  //virtual widthUnit(TimeUnits::type)=0;
  //virtual TimeUnits::type setWidthUnit()=0;
  /*@}*/

  /**\defgroup scaler Set triggered mode prescaler
   */
  /*@{*/
  virtual epicsUint32 prescaler()=0;
  virtual void setPrescaler(epicsUint32)=0;
  /*@}*/

  /**\defgroup pol Set output polarity
   *
   * Selects normal or inverted.
   */
  /*@{*/
  virtual bool polarityNorm()=0;
  virtual void setPolarityNorm(bool)=0;
  /*@}*/

  /**\defgroup man Manually set pulser output state.
   *
   * Override to set an output to the requested state.
   * Settings other than TSL::Float temporarily disable all event mappings
   */
  /*@{*/
  virtual TSL::type state()=0;
  virtual void setState(TSL::type)=0;
  /*@}*/

  /**\defgroup map Control which source(s) effect this pulser.
   *
   * Meaning of source id number is device specific.
   *
   * Note: this is one place where Device Support will have some depth.
   */
  /*@{*/
  //! What action is source 'src' mapped to?
  virtual MapType::type mappedSource(epicsUInt32 src)=0;
  //! Set mapping of source 'src'.
  virtual void sourceSetMap(epicsUInt32 src,MapType::type action)=0;
  //! Return a human readable string naming 'src'.
  virtual const char* sourceName(epicsUInt32 src)=0;

  //! Return a human readable string describing the mapping (does not affect hardware).
  virtual const char* mapDesc(epicsUInt32,MapType::type) const;//!< Default uses sourceName()
  /*@}*/
};

#endif // PULSER_HPP_INC
