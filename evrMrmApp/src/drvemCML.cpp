
#include "drvemCML.h"

#include <mrfCommonIO.h>
#include <mrfBitOps.h>
#include "evrRegMap.h"

MRMCML::MRMCML(unsigned char i,volatile unsigned char *b)
  :base(b)
  ,N(i)
{
}

MRMCML::~MRMCML()
{
}

epicsUInt32
MRMCML::pattern(cmlShort p) const
{
  switch(p){
  case cmlShortRise:
    return READ32(base, OutputCMLRise(N));
  case cmlShortHigh:
    return READ32(base, OutputCMLHigh(N));
  case cmlShortFall:
    return READ32(base, OutputCMLFall(N));
  case cmlShortLow:
    return READ32(base, OutputCMLLow(N));
  default:
    return 0;
  }
}

void
MRMCML::patternSet(cmlShort p, epicsUInt32 v)
{
  switch(p){
  case cmlShortRise:
    WRITE32(base, OutputCMLRise(N), v&0xfFfff); break;
  case cmlShortHigh:
    WRITE32(base, OutputCMLHigh(N), v&0xfFfff); break;
  case cmlShortFall:
    WRITE32(base, OutputCMLFall(N), v&0xfFfff); break;
  case cmlShortLow:
    WRITE32(base, OutputCMLLow(N), v&0xfFfff); break;
  default:
    return;
  }
}

bool
MRMCML::enabled() const
{
    return READ32(base, OutputCMLEna(N)) & OutputCMLEna_ena;
}

void
MRMCML::enable(bool s)
{
    if(s)
        BITSET(NAT,32,base, OutputCMLEna(N), OutputCMLEna_ena);
    else
        BITCLR(NAT,32,base, OutputCMLEna(N), OutputCMLEna_ena);
}

bool
MRMCML::inReset() const
{
    return READ32(base, OutputCMLEna(N)) & OutputCMLEna_rst;
}

void
MRMCML::reset(bool s)
{
    if(s)
        BITSET(NAT,32,base, OutputCMLEna(N), OutputCMLEna_rst);
    else
        BITCLR(NAT,32,base, OutputCMLEna(N), OutputCMLEna_rst);
}

bool
MRMCML::powered() const
{
    return !(READ32(base, OutputCMLEna(N)) & OutputCMLEna_pow);
}

void
MRMCML::power(bool s)
{
    if(!s)
        BITSET(NAT,32,base, OutputCMLEna(N), OutputCMLEna_pow);
    else
        BITCLR(NAT,32,base, OutputCMLEna(N), OutputCMLEna_pow);
}
