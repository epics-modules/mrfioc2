/*************************************************************************\
* Copyright (c) 2013 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef EVRMRMOUTPUT_H_INC
#define EVRMRMOUTPUT_H_INC

#include "evr/output.h"
#include "evr/evr.h"

class EVRMRM;

/**
 * Controls only the single output mapping register
 * shared by all (except CML) outputs on MRM EVRs.
 *
 * This class is reused by other subunits which
 * have identical mapping registers.
 */
class MRMOutput : public Output
{
public:
  MRMOutput(const std::string& n, EVRMRM* owner, OutputType t, unsigned int idx);
  virtual ~MRMOutput();

  virtual void lock() const;
  virtual void unlock() const;

  virtual epicsUInt32 source() const;
  virtual void setSource(epicsUInt32);

  virtual bool enabled() const;
  virtual void enable(bool);

  virtual const char*sourceName(epicsUInt32) const;

private:
  EVRMRM * const owner;
  const OutputType type;
  const unsigned int N;
  bool isEnabled;
  epicsUInt32 shadowSource;

  virtual epicsUInt32 sourceInternal() const;
  virtual void setSourceInternal(epicsUInt32);
};


#endif // EVRMRMOUTPUT_H_INC
