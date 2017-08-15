/*************************************************************************\
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef DELAY_H
#define DELAY_H

#include "mrf/object.h"

#include <epicsGuard.h>
#include <epicsTypes.h>

class epicsShareClass DelayModuleEvr : public mrf::ObjectInst<DelayModuleEvr>
{
public:
    explicit DelayModuleEvr(const std::string& n) : mrf::ObjectInst<DelayModuleEvr>(n) {}
	virtual ~DelayModuleEvr() = 0;

	virtual void setDelay0(double val)=0;
	virtual double getDelay0() const = 0;
	virtual void setDelay1(double val) = 0;
	virtual double getDelay1() const = 0;
	virtual void setState(bool enabled) = 0;
	virtual bool enabled() const = 0;
};

#endif // DELAY_H
