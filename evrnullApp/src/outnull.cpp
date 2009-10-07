
#include "outnull.h"

#include <errlog.h>

OutputNull::OutputNull(EVRNull& o) :
    card(o),
    cur(TSL::Float),
    smap()
{
}

bool
OutputNull::source(epicsUInt32 u) const
{
  return smap.find(u) != smap.end();
}

void
OutputNull::setSource(epicsUInt32 u, bool s)
{
  if(s){
    errlogPrintf("%lx : map source %u\n",(unsigned long)this,u);
    smap.insert(u);
  }else{
    errlogPrintf("%lx : unmap source %u\n",(unsigned long)this,u);
    smap.erase(u);
  }
}

TSL::type
OutputNull::state() const
{
  switch(cur){
    case TSL::Float:
      errlogPrintf("%lx : Set float\n",(unsigned long)this); break;
    case TSL::Low:
      errlogPrintf("%lx : Force Low\n",(unsigned long)this); break;
    case TSL::High:
      errlogPrintf("%lx : Force High\n",(unsigned long)this); break;
    default:
      errlogPrintf("%lx : Invalid state\n",(unsigned long)this); break;
  }

  return cur;
}
