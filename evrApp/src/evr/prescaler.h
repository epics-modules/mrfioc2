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
#include "evr/evrAPI.h"

class EVR;

class EVR_API PreScaler : public mrf::ObjectInst<PreScaler>
{
    OBJECT_DECL(PreScaler);
public:
  PreScaler(const std::string& n, EVR& o);
  virtual ~PreScaler()=0;

  virtual epicsUInt32 prescaler() const=0;
  virtual void setPrescaler(epicsUInt32)=0;

  EVR& owner;
};

#endif // PRESCALER_HPP_INC
