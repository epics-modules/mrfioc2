
#ifndef OUTNULL_H_INC
#define OUTNULL_H_INC

#include "evr/output.h"

#include <set>

class EVRNull;

class OutputNull : public Output
{
  OutputNull(EVRNull&);

  friend class EVRNull;
public:
  virtual ~OutputNull(){};

  virtual bool source(epicsUInt32 u) const;
  virtual void setSource(epicsUInt32 u, bool s);

  virtual TSL::type state() const;
  virtual void setState(TSL::type c){cur=c;};

protected:
  EVRNull& card;

  TSL::type cur;

  typedef std::set<epicsUInt32> map_t;
  map_t smap;
};

#endif /* OUTNULL_H_INC */
