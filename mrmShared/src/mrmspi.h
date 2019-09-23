/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2015 Paul Scherrer Institute (PSI), Villigen, Switzerland
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef MRMSPI_H
#define MRMSPI_H

#include <mrf/spi.h>
#include <shareLib.h>

// SPI bus access for 0x200 firmware series EVR and EVG cores.
class epicsShareClass MRMSPI : public mrf::SPIInterface
{
    volatile unsigned char * const base;
public:
    MRMSPI(volatile unsigned char *base);
    virtual ~MRMSPI();

    virtual void select(unsigned id);
    virtual epicsUInt8 cycle(epicsUInt8 in);
};

#endif // MRMSPI_H
