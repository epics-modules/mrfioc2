
#include "evrnull.h"

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
  return false;
}

void
EVRNull::specialSetMap(epicsUInt32 code, epicsUInt32 func)
{
}
