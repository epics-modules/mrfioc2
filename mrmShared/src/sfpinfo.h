#ifndef SFPINFO_H
#define SFPINFO_H

#define SFPMEM_SIZE 512

/*
 * SFP module EEPROM and diagnostics register offsets
 * from start of EEPROM.
 *
 * This is a sub-set of the registers documented.
 *
 * For firmware version #5
 * as documented in EVR-MRM-004.doc
 * Jukka Pietarinen
 * 07 Apr 2011
 *
 */

#define SFP_typeid 0
#define SFP_typeid_MASK 0xff00ff00

#define SFP_linkrate 12

/* 16 byte ascii string identifiers */
#define SFP_vendor_name 20
#define SFP_part_num 40
#define SFP_serial 68

/* 4 byte string */
#define SFP_part_rev 56
#define SFP_man_date 84 /* YYMM, eg. 1004 == Apr 2010 */

/* two byte 2s complement signed */
#define SFP_temp 352
#define SFP_tx_pwr 358
#define SFP_rx_pwr 360

#endif // SFPINFO_H
