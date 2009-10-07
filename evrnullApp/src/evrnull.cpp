
#include "evrnull.h"

#include <errlog.h>

EVRNull::EVRNull(const char* name) :
   m_enabled(false)
  ,m_locked(false)
  ,m_name(name)
  ,pllNotify()
{
  scanIoInit(&pllNotify);
}

void
EVRNull::enable(bool v)
{
  m_enabled=v;
  m_locked=v;
  scanIoRequest(pllNotify);
}

bool
EVRNull::specialMapped(epicsUInt32 code, epicsUInt32 func) const
{
  return smap.find(smap_ent_t(code,func)) != smap.end();
}

void
EVRNull::specialSetMap(epicsUInt32 code, epicsUInt32 func, bool set)
{
  if(set){
    errlogPrintf("%lx : map code %u to function %u\n",(unsigned long)this,code,func);
    smap.insert(smap_ent_t(code,func));
  }else{
    errlogPrintf("%lx : unmap code %u from function %u\n",(unsigned long)this,code,func);
    smap.erase(smap_ent_t(code,func));
  }
}
