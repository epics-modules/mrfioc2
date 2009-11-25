
#ifndef PLX9030_H
#define PLX9030_H

/*
 * A selection of registers for the PLX PCI9030
 *
 * This device is exposed as BAR #0 on PCI and PMC
 * versions of the EVR
 */

/* Address space #0 is exposed as BAR #2 */
#define U32_LAS0BRD  0x28
/* Set for big endian, clear for little endian (swapped) */
#  define LAS0BRD_ENDIAN 0x01000000

#endif /* PLX9030_H */
