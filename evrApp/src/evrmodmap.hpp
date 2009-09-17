
#ifndef EVRMODMAP_HPP_INC
#define EVRMODMAP_HPP_INC

/**\defgroup rmmreg Registers for the EVR Moduler Register Map
 *
 * Registers as described in MRF document 'cPCI-EVR-2x0_3.doc'
 *   Event Receiver
 *     cPCI-EVR-220, cPCI-EVR-230, and PMC-EVR-230
 *     (preliminary for VME-EVR-230/VME-EVR-230RF)
 *   Technical Reference
 *   Firmware Version 0003
 * Author: Jukka Pietarinen
 * Date  : 3 October 2008
 */
/*@{*/

/* 32 Bit registers */

#define RMM_STATUS  0x000
#  define RMM_STATUS_DBUS_MASK 0xFF000000
#  define RMM_STATUS_LEGVIO    0x00010000
#  define RMM_STATUS_FIFO_STOP 0x00000020

#define RMM_CONTROL 0x004
/* Master enable */
#  define RMM_CONTROL_ENA 0x80000000
/* Forward events */
#  define RMM_CONTROL_FWD 0x40000000
/* Transmit loopback */
#  define RMM_CONTROL_TLB 0x20000000
/* Receive loopback */
#  define RMM_CONTROL_RLB 0x10000000
/* DBUS #4 is timestamp counter clock */
#  define RMM_CONTROL_TSC 0x00004000
/* Reset timestamp */
#  define RMM_CONTROL_RTS 0x00002000
/* Latch timestamp */
#  define RMM_CONTROL_LTS 0x00000400
/* Event map enable */
#  define RMM_CONTROL_MEA 0x00000200
/* Select map ram */
#  define RMM_CONTROL_MSL 0x00000100
/* Latch timestamp */
#  define RMM_CONTROL_LTS 0x00000400
/* Event Log reset */
#  define RMM_CONTROL_ERS 0x00000080
/* Event Log enable */
#  define RMM_CONTROL_EEN 0x00000040
/* Event Log disable */
#  define RMM_CONTROL_EDI 0x00000020
/* Event Log event enable */
#  define RMM_CONTROL_ESE 0x00000010
/* Reset FIFO */
#  define RMM_CONTROL_RFF 0x00000004

#define RMM_IRQ_FLAG 0x008
/* Data Buffer IRQ */
#  define RMM_IRQ_BUF 0x00000020
/* Hardware mapped event signal */
#  define RMM_IRQ_HWF 0x00000010
/* Event IRQ */
#  define RMM_IRQ_EIF 0x00000008
/* Heartbeat */
#  define RMM_IRQ_HBF 0x00000004
/* FIFO full */
#  define RMM_IRQ_FFF 0x00000002
/* Receiver error */
#  define RMM_IRQ_RXE 0x00000001
#define RMM_IRQ_MASK 0x00C
/* Bit mask is the same as IRQ Flag
 * with one extra
 */
#  define RMM_IRQ_ENA 0x80000000

/* Mapping to control Hardware mapped IRQ source */
#define RMM_PULSE_IRQ 0x010
/* Use mapping IDs (RMM_MAP_*) */

#define RMM_RXBUF_CSR 0x020
#define RMM_TXBUF_CSR 0x024

#define RMM_VERSION 0x02C
#  define RMM_VERSION_TYPE  0xF0000000
#  define RMM_VERSION_FORM  0x0F000000
#  define RMM_VERSION_VERID 0x000000FF

/* Timestamp counter prescaler */
#define RMM_ECLK_PRE 0x040
/* Event clock microseconds divider */
#define RMM_ECLK_DIV 0x04C
/* Clock control */
#define RMM_ECLK_CTRL 0x050
#  define RM_ECLK_CTRL_LOCK 0x00000200

#define RMM_SEC_SHIFT 0x05C
#define RMM_SEC_COUNT 0x060
#define RMM_EVT_COUNT 0x064
#define RMM_SEC_LATCH 0x068
#define RMM_EVT_LATCH 0x06C

#define RMM_SEC_FIFO  0x070
#define RMM_EVT_FIFO  0x074
#define RMM_CODE_FIFO 0x078

#define RMM_LOG_STATUS 0x07C
#  define RMM_LOG_STATUS_OVERFLOW 0x80000000
#  define RMM_LOG_STATUS_PTR_MASK 0x000000FF

#define RMM_FRAC_DIV  0x080
#define RMM_INIT_PHASE 0x088

#define RMM_GPIO_DIR 0x090
#define RMM_GPIO_IN  0x094
#define RMM_GPIO_OUT 0x098

#define RMM_SCALER_0 0x100
/* 0 <= N <= 2 */
#define RMM_SCALER(N) ( RMM_SCALER_0 + 4*(N) )

#define RMM_PULSE_CTRL_0 0x200
/* 0 <= N <= 9 */
#define RMM_PULSE_CTRL(N) ( RMM_PULSE_CTRL_0 + 0x10*(N) )
#  define RMM_PULSE_CTRL_OUT 0x00000080 /* read-only */
/* Software set */
#  define RMM_PULSE_CTRL_SWS 0x00000040
/* Software reset */
#  define RMM_PULSE_CTRL_SWR 0x00000020
/* Output polarity 0 norm, 1 inv */
#  define RMM_PULSE_CTRL_POL 0x00000010
/* Allow event mapped reset */
#  define RMM_PULSE_CTRL_MRE 0x00000008
/* Allow event mapped set */
#  define RMM_PULSE_CTRL_MSE 0x00000004
/* Allow event mapped trigger */
#  define RMM_PULSE_CTRL_MTE 0x00000002
/* Generator enable */
#  define RMM_PULSE_CTRL_ENA 0x00000001


#define RMM_PULSE_SCALE_0 0x204
#define RMM_PULSE_SCALE(N) ( RMM_PULSE_SCALE_0 + 0x10*(N) )

#define RMM_PULSE_DELAY_0 0x208
#define RMM_PULSE_DELAY(N) ( RMM_PULSE_DELAY_0 + 0x10*(N) )

#define RMM_PULSE_WIDTH_0 0x20C
#define RMM_PULSE_WIDTH(N) ( RMM_PULSE_WIDTH_0 + 0x10*(N) )

/* 16 Bit registers */

#define RMM_FP_MAP_0 0x400
/* 0 <= N <= 7 */
#define RMM_FP_MAP(N) ( RMM_FP_MAP_0 + 2*(N) )

#define RMM_UNIV_MAP_0 0x440
/* 0 <= N <= 9 */
#define RMM_UNIV_MAP(N) ( RMM_UNIV_MAP_0 + 2*(N) )

#define RMM_TB_MAP_0 0x480
/* 0 <= N <= 31 */
#define RMM_TB_MAP(N) ( RMM_TB_MAP_0 + 2*(N) )

/* 32 Bit registers */

#define RMM_FPIN_MAP_0 0x500
/* 0 <= N <= 1 */
#define RMM_FPIN_MAP(N) ( RMM_FPIN_MAP_0 + 4*(N) )


#define RMM_CML_PAT_LOW_4 0x600
/* 4 <= N <= 6 */
#define RMM_CML_PAT_LOW(N) ( RMM_CML_PAT_LOW_4 + 0x20*((N)-4) )

#define RMM_CML_PAT_RISE_4 0x604
/* 4 <= N <= 6 */
#define RMM_CML_PAT_RISE(N) ( RMM_CML_PAT_RISE_4 + 0x20*((N)-4) )

#define RMM_CML_PAT_FALL_4 0x608
/* 4 <= N <= 6 */
#define RMM_CML_PAT_FALL(N) ( RMM_CML_PAT_FALL_4 + 0x20*((N)-4) )

#define RMM_CML_PAT_HIGH_4 0x60C
/* 4 <= N <= 6 */
#define RMM_CML_PAT_HIGH(N) ( RMM_CML_PAT_HIGH_4 + 0x20*((N)-4) )

#define RMM_CML_CTRL_4 0x610
/* 4 <= N <= 6 */
#define RMM_CML_CTRL(N) ( RMM_CML_CTRL_4 + 0x20*((N)-4) )

#define RMM_RXDATA_BUFFER 0x0800
#define RMM_RXDATA_BUFFER_END 0x0FFF
#define RMM_RXDATA_BUFFER_LEN (RMM_RXDATA_BUFFER_END - RMM_RXDATA_BUFFER + 1)

#define RMM_TXDATA_BUFFER 0x1800
#define RMM_TXDATA_BUFFER_END 0x1FFF
#define RMM_TXDATA_BUFFER_LEN (RMM_TXDATA_BUFFER_END - RMM_TXDATA_BUFFER + 1)

#define RMM_LOG_BUFFER 0x2000
#define RMM_LOG_BUFFER_END 0x2FFF
#define RMM_LOG_BUFFER_LEN (RMM_LOG_BUFFER_END - RMM_LOG_BUFFER + 1)

#define RMM_MAP_0 0x4000
#define RMM_MAP_0_END 0x5FFF
#define RMM_MAP_LEN (RMM_MAP_0_END - RMM_MAP_0 + 1)

/* 0 <= N <= 1 */
#define RMM_MAP(N) ( RMM_MAP_0 + RMM_MAP_LEN*(N) )
#define RMM_MAP_END(N) ( RMM_MAP_0_END + RMM_MAP_LEN*(N) )

/*@}*/

/**\defgroup mapids Signal Mapping IDs
 * Signal source bit numbers (0 indexed)
 */
/*@{*/

/* 0 <= N <= 9 */
#define RMM_MAP_PULSE(N) (N)

/* 0 <= N <= 7 */
#define RMM_MAP_DBUS(N) ( 32+(N) )

/* 0 <= N <= 2 */
#define RMM_MAP_SCALE(N) ( 40+(N) )

#define RMM_MAP_FORCE_HIGH 62
#define RMM_MAP_FORCE_LOW 63

/*@}*/
#endif // EVRMODMAP_HPP_INC
