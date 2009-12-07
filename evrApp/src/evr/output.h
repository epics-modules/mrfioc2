
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
  virtual epicsUInt32 source() const=0;
  virtual void setSource(epicsUInt32)=0;
  /*@}*/

  virtual const char*sourceName(epicsUInt32) const=0;
};

#endif // OUTPUT_HPP_INC
