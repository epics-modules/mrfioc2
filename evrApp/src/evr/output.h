
#ifndef OUTPUT_HPP_INC
#define OUTPUT_HPP_INC

#include "evr/util.h"

#include <epicsTypes.h>

class Output : public IOStatus
{
public:
  virtual ~Output()=0;

  /**\defgroup src Control which source(s) effect this output.
   *
   * Meaning of source id number is device specific.
   *
   * Note: this is one place where Device Support will have some depth.
   */
  /*@{*/
  virtual bool source(epicsUInt32) const=0;
  virtual void setSource(epicsUInt32, bool)=0;
  /*@}*/

  /**\defgroup man Manually set output state.
   *
   * Override to set an output to the requested state.
   * Settings other than TSL::Float temporarily disable all event mappings
   */
  /*@{*/
  virtual TSL::type state() const=0;
  // Settings other than TSL::Float temporarily disable all sources
  virtual void setState(TSL::type)=0;
  /*@}*/
};

#endif // OUTPUT_HPP_INC
