/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef PRESCALER_HPP_INC
#define PRESCALER_HPP_INC

#include <epicsTypes.h>

#include "mrf/object.h"

class EVR;

class epicsShareClass PreScaler : public mrf::ObjectInst<PreScaler>
{
public:
  PreScaler(const std::string& n, EVR& o):mrf::ObjectInst<PreScaler>(n),owner(o){};
  virtual ~PreScaler()=0;

  virtual epicsUInt32 prescaler() const=0;
  virtual void setPrescaler(epicsUInt32)=0;

  virtual epicsUInt32 prescalerPhasOffs() const { return 0; };
  virtual void setPrescalerPhasOffs(epicsUInt32) { };

  EVR& owner;
};

#endif // PRESCALER_HPP_INC
