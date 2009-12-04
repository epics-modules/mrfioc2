
#ifndef OUTNULL_H_INC
#define OUTNULL_H_INC

#include "evr/output.h"

#include <set>

class EVRNull;

class OutputNull : public Output
{
  OutputNull():src(0){};

  friend class EVRNull;
public:
  virtual ~OutputNull(){};

  virtual epicsUInt32 source() const{return src;};
  virtual void setSource(epicsUInt32 u){src=u;};

  virtual const char*sourceName(epicsUInt32) const{return "Unknown";};

protected:
  epicsUInt32 src;
};

#endif /* OUTNULL_H_INC */
