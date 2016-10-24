/*************************************************************************\
* Copyright (c) 2013 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef OUTPUT_HPP_INC
#define OUTPUT_HPP_INC

#include "evr/util.h"
#include "mrf/object.h"

#include <epicsTypes.h>

enum OutputType {
  OutputInt=0, //!< Internal
  OutputFP=1,  //!< Front Panel
  OutputFPUniv=2, //!< FP Universal
  OutputRB=3 //!< Rear Breakout
};

class epicsShareClass Output : public mrf::ObjectInst<Output>, public IOStatus
{
public:
  Output(const std::string& n) : mrf::ObjectInst<Output>(n) {}
  virtual ~Output()=0;

  /**\defgroup src Control which source(s) effect this output.
   *
   * Meaning of source id number is device specific.
   *
   * When enabled()==true then the user mapping provided to setSource()
   * is used.  When enabled()==false then a device specific special mapping
   * is used (eg. Force Low).
   *
   * Note: this is one place where Device Support will have some depth.
   */
  /*@{*/
  virtual epicsUInt32 source() const=0;
  virtual void setSource(epicsUInt32)=0;

  virtual bool enabled() const=0;
  virtual void enable(bool)=0;
  /*@}*/

  virtual const char*sourceName(epicsUInt32) const=0;
};

#endif // OUTPUT_HPP_INC
