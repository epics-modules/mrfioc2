/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#include "drvemCML.h"

#include <stdexcept>
#include <algorithm>

#include <mrfCommonIO.h>
#include <mrfBitOps.h>
#include "evrRegMap.h"
#include "drvem.h"

MRMCML::MRMCML(unsigned char i,EVRMRM& o, outkind k, evrForm f)
  :mult(f==evrFormCPCIFULL ? 40 : 20)
  ,base(o.base)
  ,N(i)
  ,owner(o)
  ,shadowEnable(0)
  ,kind(k)
{
    epicsUInt32 val=READ32(base, OutputCMLEna(N));

    val&=~OutputCMLEna_type_mask;

    switch(kind) {
    case typeCML: break;
    case typeTG203: val|=OutputCMLEna_type_203; break;
    case typeTG300: val|=OutputCMLEna_type_300; break;
    default:
        throw std::invalid_argument("Invalid CML kind");
    }

    shadowEnable=val;
}

MRMCML::~MRMCML()
{
}

cmlMode
MRMCML::mode() const
{
    switch(shadowEnable&OutputCMLEna_mode_mask) {
    case OutputCMLEna_mode_orig:
        return cmlModeOrig;
    case OutputCMLEna_mode_freq:
        return cmlModeFreq;
    case OutputCMLEna_mode_patt:
        return cmlModePattern;
    default:
        return cmlModeInvalid;
    }
}

void
MRMCML::setMode(cmlMode m)
{
    epicsUInt32 mask=0;
    switch(m) {
    case cmlModeOrig:    mask |= OutputCMLEna_mode_orig; break;
    case cmlModeFreq:    mask |= OutputCMLEna_mode_freq; break;
    case cmlModePattern: mask |= OutputCMLEna_mode_patt; break;
    default:
        throw std::out_of_range("Invalid CML Mode");
    }
    shadowEnable &= ~OutputCMLEna_mode_mask;
    shadowEnable |= mask;
    WRITE32(base, OutputCMLEna(N), shadowEnable);
}

bool
MRMCML::enabled() const
{
    return shadowEnable&OutputCMLEna_ena;
}

void
MRMCML::enable(bool s)
{
    if(s)
        shadowEnable |= OutputCMLEna_ena;
    else
        shadowEnable &= ~OutputCMLEna_ena;
    WRITE32(base, OutputCMLEna(N), shadowEnable);
}

bool
MRMCML::inReset() const
{
    return shadowEnable & OutputCMLEna_rst;
}

void
MRMCML::reset(bool s)
{
    if(s)
        shadowEnable |= OutputCMLEna_rst;
    else
        shadowEnable &= ~OutputCMLEna_rst;
    WRITE32(base, OutputCMLEna(N), shadowEnable);
}

bool
MRMCML::powered() const
{
    return !(shadowEnable & OutputCMLEna_pow);
}

void
MRMCML::power(bool s)
{
    if(s)
        shadowEnable |= OutputCMLEna_pow;
    else
        shadowEnable &= ~OutputCMLEna_pow;
    WRITE32(base, OutputCMLEna(N), shadowEnable);
}

double
MRMCML::fineDelay() const
{
    return 0.0;
}

void
MRMCML::setFineDelay(double)
{

}

void
MRMCML::setPolarityInvert(bool s)
{
    if(s)
        shadowEnable |= OutputCMLEna_ftrg;
    else
        shadowEnable &= ~OutputCMLEna_ftrg;
    WRITE32(base, OutputCMLEna(N), shadowEnable);
}

bool
MRMCML::polarityInvert() const
{
    return shadowEnable & OutputCMLEna_ftrg;
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
    double period=1.0/(mult*owner.clock());

    return countHigh()*period;
}

double
MRMCML::timeLow () const
{
    double period=1.0/(mult*owner.clock());

    return countLow()*period;
}

void
MRMCML::setTimeHigh(double v)
{
    double period=1.0/(mult*owner.clock());

    setCountHigh((epicsUInt32)(v/period));
}

void
MRMCML::setTimeLow (double v)
{
    double period=1.0/(mult*owner.clock());

    setCountLow((epicsUInt32)(v/period));
}

  // For Pattern mode

bool
MRMCML::recyclePat() const
{
    return shadowEnable & OutputCMLEna_cycl;
}

void
MRMCML::setRecyclePat(bool s)
{
    if(s)
        shadowEnable |= OutputCMLEna_cycl;
    else
        shadowEnable &= ~OutputCMLEna_cycl;
    WRITE32(base, OutputCMLEna(N), shadowEnable);
}

epicsUInt32
MRMCML::lenPattern(pattern p) const
{
    epicsUInt32 val;

    if(p==patternWaveform) {
        val=READ32(base, OutputCMLPatLength(N));
        // Always <=2048
        return mult*val;
    }

    return 1;
}

epicsUInt32
MRMCML::lenPatternMax(pattern p) const
{
    if(p==patternWaveform)
        return mult*OutputCMLPatLengthMax;
    else
        return 1;
}

epicsUInt32
MRMCML::getPattern(pattern p, unsigned char *buf, epicsUInt32 blen) const
{
    epicsUInt32 plen=lenPattern(p);

    // number of bytes of 'buf' to write
    epicsUInt32 outlen = std::min(plen, blen);

    switch(mult) {
    case 20: return getPattern20(p,buf,outlen); break;
    case 40: return getPattern40(p,buf,outlen); break;
    default:
        throw std::runtime_error("unsupported word size");
    }
}

epicsUInt32
MRMCML::getPattern20(pattern p, unsigned char *buf, epicsUInt32 blen) const
{
    epicsUInt32 val=0;
    for(epicsUInt32 i=0; i<blen; i++) {
        if(i%mult==0){
            switch(p) {
            case patternWaveform:
                val=READ32(base, OutputCMLPat(N, i/mult)); break;
            case patternHigh:
                val=READ32(base, OutputCMLHigh(N)); break;
            case patternFall:
                val=READ32(base, OutputCMLFall(N)); break;
            case patternLow:
                val=READ32(base, OutputCMLLow(N)); break;
            case patternRise:
                val=READ32(base, OutputCMLRise(N)); break;
            default:
                val=0;
            }
        }

        buf[i]=val>>(mult-1-i%mult);
        buf[i]&=0x1;
    }
    return blen;
}

epicsUInt32
MRMCML::getPattern40(pattern p, unsigned char *buf, epicsUInt32 blen) const
{
    size_t offset;
    switch(p) {
    case patternWaveform:
    case patternLow:
        offset=0;
        break;
    case patternRise:
        offset=1;
        break;
    case patternHigh:
        offset=2;
        break;
    case patternFall:
        offset=3;
        break;
    default:
        return 0;
    }

    epicsUInt32 val=0;
    for(epicsUInt32 i=0; i<blen; i++) {
        size_t cmlword = (i/mult) + offset;
        size_t cmlbit  = (i%mult);
        size_t pciword = 2*cmlword + (cmlbit<8 ? 0 : 1);
        size_t pcibit  = cmlbit<8 ? 7-cmlbit : 31-(cmlbit-8);

        if(cmlbit==0 || cmlbit==8) {
            val=READ32(base, OutputCMLPat(N, pciword));
        }

        buf[i]=val>>pcibit;
        buf[i]&=0x1;

    }
    return blen;
}

void
MRMCML::setPattern(pattern p, const unsigned char *buf, epicsUInt32 blen)
{
    // If we are given a length that is not a multiple of word size
    // then truncate.
    if(blen%mult)
        blen-=blen%mult;

    if(blen>lenPatternMax(p))
        throw std::out_of_range("Pattern is too long");

    bool reenable=enabled();
    if (reenable && mode()!=cmlModeFreq)
        enable(false);

    switch(mult) {
    case 20: setPattern20(p,buf,blen); break;
    case 40: setPattern40(p,buf,blen); break;
    default:
        throw std::runtime_error("unsupported word size");
    }

    WRITE32(base, OutputCMLPatLength(N), blen/mult);

    if (reenable && mode()!=cmlModeFreq)
        enable(true);
}

void
MRMCML::setPattern20(pattern p, const unsigned char *buf, epicsUInt32 blen)
{
    epicsUInt32 val=0;
    for(epicsUInt32 i=0; i<blen; i++) {
        val|=(!!buf[i])<<(mult-1-i%mult);

        if(i%mult==(mult-1)) {
            switch(p) {
            case patternWaveform:
                WRITE32(base, OutputCMLPat(N, i/mult), val); break;
            case patternHigh:
                WRITE32(base, OutputCMLHigh(N), val); break;
            case patternFall:
                WRITE32(base, OutputCMLFall(N), val); break;
            case patternLow:
                WRITE32(base, OutputCMLLow(N), val); break;
            case patternRise:
                WRITE32(base, OutputCMLRise(N), val); break;
            }
            val=0;
        }
    }
}

void
MRMCML::setPattern40(pattern p, const unsigned char *buf, epicsUInt32 blen)
{
    size_t offset;
    switch(p) {
    case patternWaveform:
    case patternLow:
        offset=0;
        break;
    case patternRise:
        offset=1;
        break;
    case patternHigh:
        offset=2;
        break;
    case patternFall:
        offset=3;
        break;
    default:
        return;
    }

    epicsUInt32 val=0;
    for(epicsUInt32 i=0; i<blen; i++) {
        size_t cmlword = (i/mult) + offset;
        size_t cmlbit  = (i%mult);
        size_t pciword = 2*cmlword + (cmlbit<8 ? 0 : 1);
        size_t pcibit  = cmlbit<8 ? 7-cmlbit : 31-(cmlbit-8);
        printf("X %d=%d %d %d %d %d %08x\n", i, !!buf[i], cmlword, cmlbit, pciword, pcibit,val);

        val|=(!!buf[i])<<pcibit;

        if(cmlbit==7 || cmlbit==39) {
            WRITE32(base, OutputCMLPat(N, pciword), val);
            printf("%04x: %08lx\n", U32_OutputCMLPat(N, pciword), (unsigned long)val);
            val=0;
        }
    }

}
