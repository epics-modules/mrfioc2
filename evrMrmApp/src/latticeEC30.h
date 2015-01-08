/*
 * Author: Jure Krasna <jure.krasna@cosylab.com>
 */

#ifndef LATTICEEC30_H_
#define LATTICEEC30_H_

/*
 * A list of registers for the PCIe EVR board, specifically the EC 30 chip
 *
 * This device is exposed as BAR #0 on PCI and PMC versions of the EVR
 */

#define U32_AC30CTRL                0x00004
# define AC30CTRL_LEMDE             0x19        /* Endian: clear = BE, set = LE */

#endif /* LATTICEEC30_H_ */
