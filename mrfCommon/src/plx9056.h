/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@gmail.com>
 */

#ifndef PLX9056_H
#define PLX9056_H

/*
 * A selection of registers for the PLX PCI9056
 *
 * This device is exposed as BAR #0
 */

#define U8_BIGEND9056 0x0C
#  define BIGEND9056_BIG (1<<2)

#define U32_INTCSR9056 0x68
#  define INTCSR9056_PCI_Enable (1<<8)
#  define INTCSR9056_LCL_Enable (1<<11)
#  define INTCSR9056_LCL_Active (1<<15)

#endif // PLX9056_H
