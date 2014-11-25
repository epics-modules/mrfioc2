/*************************************************************************\
* Copyright (c) 2010 Brookhaven Science Associates, as Operator of
*     Brookhaven National Laboratory.
* mrfioc2 is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * Author: Michael Davidsaver <mdavidsaver@bnl.gov>
 */

#ifndef MRFCSR_H
#define MRFCSR_H


#define MRF_VME_IEEE_OUI     0x000EB2   /* VME Organizationally Unique Identifier (OUI) for MRF */

#define MRF_VME_EVG_BID      0x45470000 /* VME Event Generator */
#define MRF_VME_EVR_BID      0x45520000 /* VME Event Receiver */
#define MRF_VME_EVR_RF_BID   0x45524600 /* VME Event Receiver with RF Recovery */

#define MRF_SERIES_200       0x000000C8 /* Series 200 Code */
#define MRF_SERIES_220       0x000000DC /* Series 220 Code */
#define MRF_SERIES_230       0x000000E6 /* Series 230 Code */

/**************************************************************************************************/
/*  Board ID Field Masks                                                                          */
/**************************************************************************************************/

#define MRF_BID_TYPE_MASK    0xFFFFFF00 /* Mask for Board ID Field                                */
#define MRF_BID_SERIES_MASK  0x000000FF /* Mask for Board Series Field                            */

/**************************************************************************************************/
/*  Series 200 Board ID Codes                                                                     */
/**************************************************************************************************/

#define MRF_VME_EVG200_BID   (MRF_VME_EVG_BID    | MRF_SERIES_200) /* VME Event Generator 200     */
#define MRF_VME_EVR200_BID   (MRF_VME_EVR_BID    | MRF_SERIES_200) /* VME Event Receiver 200      */
#define MRF_VME_EVR200RF_BID (MRF_VME_EVR_RF_BID | MRF_SERIES_200) /* VME EVR 200 w/ RF Recovery  */

#define MRF_VME_GTX200_BID   (MRF_VME_GTX_BID    | MRF_SERIES_200) /* VME Electron Gun Trigger    */
#define MRF_VME_4CT200_BID   (MRF_VME_4CT_BID    | MRF_SERIES_200) /* VME Four Channel Trigger    */


/**************************************************************************************************/
/*  Series 230 Board ID Codes                                                                     */
/**************************************************************************************************/

#define MRF_VME_EVG230_BID   (MRF_VME_EVG_BID    | MRF_SERIES_230) /* VME Event Generator 230     */
#define MRF_VME_EVR230_BID   (MRF_VME_EVR_BID    | MRF_SERIES_230) /* VME Event Receiver 230      */
#define MRF_VME_EVR230RF_BID (MRF_VME_EVR_RF_BID | MRF_SERIES_230) /* VME EVR 230 w/ RF Recovery  */

/**************************************************************************************************/
/*  CR/CSR User-CSR Space Offsets (MRF Specific)                                                  */
/*                                                                                                */
/* Register address is slot base address + read(base + CR_BEG_UCSR) + UCSR_*                      */
/**************************************************************************************************/

#define  UCSR_IRQ_LEVEL                0x0000   /* Interrupt request level                        */
#define  UCSR_IRQ_VECTOR               0x0004   /* Interrupt request vector                       */
#define  UCSR_SERIAL_NUMBER            0x0010   /* Board serial number (MAC Address)              */

#define  UCSR_SN_BYTES                 6

#endif /* MRFCSR_H */
