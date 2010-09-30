
#include "drvemCML.h"

#include <stdexcept>

#include <mrfCommonIO.h>
#include <mrfBitOps.h>
#include "evrRegMap.h"
#include "drvem.h"

MRMCML::MRMCML(unsigned char i,EVRMRM& o)
  :base(o.base)
  ,N(i)
  ,owner(o)
{
}

MRMCML::~MRMCML()
{
}

cmlMode
MRMCML::mode() const
{
  epicsUInt32 val=READ32(base, OutputCMLEna(N)) & OutputCMLEna_mode_mask;
  if (val==OutputCMLEna_mode_orig)
    return cmlModeOrig;
  else if (val==OutputCMLEna_mode_freq)
    return cmlModeFreq;
  else if (val==OutputCMLEna_mode_patt)
    return cmlModePattern;
  else
    return cmlModeInvalid;
}

void
MRMCML::setMode(cmlMode m)
{
  epicsUInt32 val=READ32(base, OutputCMLEna(N)) & ~OutputCMLEna_mode_mask;
  switch(m) {
  case cmlModeOrig:    val |= OutputCMLEna_mode_orig; break;
  case cmlModeFreq:    val |= OutputCMLEna_mode_freq; break;
  case cmlModePattern: val |= OutputCMLEna_mode_patt; break;
  default:
    throw std::out_of_range("Invalid CML Mode");
  }
  WRITE32(base, OutputCMLEna(N), val);
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

void
MRMCML::setPolarityInvert(bool s)
{
    if(s)
        BITSET(NAT,32,base, OutputCMLEna(N), OutputCMLEna_ftrg);
    else
        BITCLR(NAT,32,base, OutputCMLEna(N), OutputCMLEna_ftrg);
}

bool
MRMCML::polarityInvert() const
{
    return READ32(base, OutputCMLEna(N)) & OutputCMLEna_ftrg;
}

epicsUInt32
MRMCML::countHigh() const
{
  return READ16(base, OutputCMLCountHigh(N));
}

epicsUInt32
MRMCML::countLow () const
{
  return READ16(base, OutputCMLCountLow(N));
}

void
MRMCML::setCountHigh(epicsUInt32 v)
{
  if(v<=20 || v>=65535)
    throw std::out_of_range("Invalid CML freq. count");
  WRITE16(base, OutputCMLCountHigh(N), v);
}

void
MRMCML::setCountLow (epicsUInt32 v)
{
  if(v<=20 || v>=65535)
    throw std::out_of_range("Invalid CML freq. count");
  WRITE16(base, OutputCMLCountLow(N), v);
}

double
MRMCML::timeHigh() const
{
  double period=1.0/(OutputCMLPatNBit*owner.clock());

  return countHigh()*period;
}

double
MRMCML::timeLow () const
{
  double period=1.0/(OutputCMLPatNBit*owner.clock());

  return countLow()*period;
}

void
MRMCML::setTimeHigh(double v)
{
  double period=1.0/(OutputCMLPatNBit*owner.clock());

  setCountHigh(v/period);
}

void
MRMCML::setTimeLow (double v)
{
  double period=1.0/(OutputCMLPatNBit*owner.clock());

  setCountLow(v/period);
}

  // For Pattern mode

bool
MRMCML::recyclePat() const
{
    return READ32(base, OutputCMLEna(N)) & OutputCMLEna_cycl;
}

void
MRMCML::setRecyclePat(bool s)
{
    if(s)
        BITSET(NAT,32,base, OutputCMLEna(N), OutputCMLEna_cycl);
    else
        BITCLR(NAT,32,base, OutputCMLEna(N), OutputCMLEna_cycl);
}

epicsUInt32
MRMCML::lenPattern() const
{
  epicsUInt32 val=READ32(base, OutputCMLPatLength(N));
  // Always <=2048
  return OutputCMLPatNBit*val;
}

epicsUInt32
MRMCML::lenPatternMax() const
{
  return OutputCMLPatNBit*OutputCMLPatLengthMax;
}

void
MRMCML::getPattern(unsigned char *buf, epicsUInt32 *blen) const
{
  epicsUInt32 plen=lenPattern();

  // number of bytes of 'buf' to write
  epicsUInt32 outlen = plen < *blen ? plen : *blen;

  epicsUInt32 val=0;
  for(epicsUInt32 i=0; i<outlen; i++) {
    if(i%OutputCMLPatNBit==0){
      val=READ32(base, OutputCMLPat(N, i/OutputCMLPatNBit));
    }

    buf[i]=val>>(OutputCMLPatNBit-1-i%OutputCMLPatNBit);
    buf[i]&=0x1;
  }

  *blen=outlen;
}

void
MRMCML::setPattern(const unsigned char *buf, epicsUInt32 blen)
{
  // If we are given a length that is not a multiple of 20
  // then truncate.
  if(blen%OutputCMLPatNBit)
    blen-=blen%OutputCMLPatNBit;

  if(blen>lenPatternMax())
    throw std::out_of_range("Pattern is too long");

  bool reenable=enabled();
  if (reenable && mode()==cmlModePattern)
    enable(false);

  epicsUInt32 val=0;
  for(epicsUInt32 i=0; i<blen; i++) {
    val|=(!!buf[i])<<(OutputCMLPatNBit-1-i%OutputCMLPatNBit);

    if(i%OutputCMLPatNBit==(OutputCMLPatNBit-1)) {
      WRITE32(base, OutputCMLPat(N, i/OutputCMLPatNBit), val);
      val=0;
    }
  }

  WRITE32(base, OutputCMLPatLength(N), blen/OutputCMLPatNBit);

  if (reenable && mode()==cmlModePattern)
    enable(true);
}
