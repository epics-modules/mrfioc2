
#ifndef PULSER_HPP_INC
#define PULSER_HPP_INC

#include "evr/util.h"

#include <epicsTypes.h>

#include <string>

struct MapType {
  enum type {
    None=0,
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
  virtual bool enabled() const=0;
  virtual void enable(bool)=0;
  /*@}*/

  /**\defgroup dly Set triggered mode delay length.
   *
   * Units of event clock period.
   */
  /*@{*/
  virtual void setDelayRaw(epicsUInt32)=0;
  virtual void setDelay(double)=0;
  virtual epicsUInt32 delayRaw() const=0;
  virtual double delay() const=0;
  /*@}*/

  /**\defgroup wth Set triggered mode width
   *
   * Units of event clock period.
   */
  /*@{*/
  virtual void setWidthRaw(epicsUInt32)=0;
  virtual void setWidth(double)=0;
  virtual epicsUInt32 widthRaw() const=0;
  virtual double width() const=0;
  /*@}*/

  /**\defgroup scaler Set triggered mode prescaler
   */
  /*@{*/
  virtual epicsUInt32 prescaler() const=0;
  virtual void setPrescaler(epicsUInt32)=0;
  /*@}*/

  /**\defgroup pol Set output polarity
   *
   * Selects normal or inverted.
   */
  /*@{*/
  virtual bool polarityInvert() const=0;
  virtual void setPolarityInvert(bool)=0;
  /*@}*/

  /**\defgroup map Control which source(s) effect this pulser.
   *
   * Meaning of source id number is device specific.
   *
   * Note: this is one place where Device Support will have some depth.
   */
  /*@{*/
  //! What action is source 'src' mapped to?
  virtual MapType::type mappedSource(epicsUInt32 src) const=0;
  //! Set mapping of source 'src'.
  virtual void sourceSetMap(epicsUInt32 src,MapType::type action)=0;

  //! Return a human readable string naming 'src'.
  virtual const char* sourceName(epicsUInt32 src) const=0;

  //! Return a human readable string describing the mapping (does not affect hardware).
  virtual std::string mapDesc(epicsUInt32,MapType::type) const;
  /*@}*/
};

#endif // PULSER_HPP_INC
