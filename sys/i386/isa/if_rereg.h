/*
 * Copyright (c)1993 Foretune Co., Ltd. All rights reserved.
 *
 *	BSDI $Id: if_rereg.h,v 1.1 1994/01/29 05:24:12 karels Exp $
 */

/*
 * Allied Telesis RE2000/AT-1700 series Ethernet adapter driver
 */

#define ETH_ADDR_LEN	6
#define RE_NPORT	32

/* Fujitsu MB86965A */

#define DLCR0	0x00	/* Transmit status */
#define DLCR1	0x01	/* Receive status */
#define DLCR2	0x02	/* Transmit interrupt mask */
#define DLCR3	0x03	/* Receive interrupt mask */
#define DLCR4	0x04	/* Transmit mode */
#define DLCR5	0x05	/* Receive mode */
#define DLCR6	0x06	/* Control 1 */
#define DLCR7	0x07	/* Control 2 */

/* register bank 0 */

#define DLCR8	0x08	/* MAC address 1 */
#define DLCR9	0x09	/* MAC address 2 */
#define DLCR10	0x0a	/* MAC address 3 */
#define DLCR11	0x0b	/* MAC address 4 */
#define DLCR12	0x0c	/* MAC address 5 */
#define DLCR13	0x0d	/* MAC address 6 */

#define DLCR14	0x0e	/* TDR low */
#define DLCR15	0x0f	/* TDR high */

/* register bank 1 */

#define MAR8	0x08	/* Multicast address 1 */
#define MAR9	0x09	/* Multicast address 2 */
#define MAR10	0x0a	/* Multicast address 3 */
#define MAR11	0x0b	/* Multicast address 4 */
#define MAR12	0x0c	/* Multicast address 5 */
#define MAR13	0x0d	/* Multicast address 6 */
#define MAR14	0x0e	/* Multicast address 7 */
#define MAR15	0x0f	/* Multicast address 8 */

/* register bank 2 */

#define BMPR8	0x08	/* Buffer memory port */
#define BMPR10	0x0a	/* Transmit packet count */
#define BMPR11	0x0b	/* 16 times collision control */
#define BMPR12	0x0c	/* DMA mask */
#define BMPR13	0x0d	/* DMA mode / Transceiver mode */
#define BMPR14	0x0e	/* Receive control / Transceiver interrupt mask */
#define BMPR15	0x0f	/* Transceiver status */

/* EEPROM control */

#define BMPR16	0x10	/* EEPROM control */
#define BMPR17	0x11	/* EEPROM data port */

/* misc */

#define BMPR18	0x12	/* I/O base control */
#define	BMPR19	0x13	/* board configration data */

/* -------- */

#define	RE_TX_STAT	DLCR0
#define	RE_RX_STAT	DLCR1
#define	RE_TX_MASK	DLCR2
#define	RE_RX_MASK	DLCR3
#define	RE_TX_MODE	DLCR4
#define	RE_COLL_CNT	DLCR4
#define	RE_RX_MODE	DLCR5
#define	RE_CTL1		DLCR6
#define	RE_CTL2		DLCR7

#define	RE_LAR		DLCR8	/* MAC address */
#define RE_MAR		MAR8	/* Multicast address */

#define	RE_BUF_IO	BMPR8	/* Buffer memory port */
#define RE_TX_CNT	BMPR10

#define	RE_TR_MODE	BMPR13
#define	RE_TR_MASK	BMPR14
#define	RE_TR_STAT	BMPR15

#define ROM_CTL		BMPR16
#define ROM_SK		0x40	/* EEPROM shift clock */
#define ROM_CS		0x20	/* EEPROM chip select */
#define ROM_CS_SK	(ROM_SK | ROM_CS)
#define	ROM_OFF		0x00

#define ROM_DATA	BMPR17
#define ROM_RD		0x80	/* EEPROM data in */
#define ROM_WR		0x80	/* EEPROM data out */
#define ROM_WR1		ROM_WR
#define ROM_WR0		0x00

#define	ROM_DELAY()	DELAY(10)

#define RE_VID0		0x00	/* Allied Telesis Vendor ID 0 */
#define RE_VID1		0x00	/* Allied Telesis Vendor ID 1 */
#define RE_VID2		0xf4	/* Allied Telesis Vendor ID 2 */
#define RE_PID		0xc0	/* RE2000/AT-1700 product ID */

#define RE_CONFIG	BMPR19
