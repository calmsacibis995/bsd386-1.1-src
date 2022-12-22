/*	BSDI $Id: if_hpplusreg.h,v 1.1 1994/01/29 05:23:20 karels Exp $	*/

/*-
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and the University of California, San Diego
 *	and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Register descriptions for * HP EtherTwist PC LAN Adapter/16 Plus
 *      Supported Models:
 *              HP 27247B PC LAN Adapter/16 TP Plus [AUI/UTP]
 *              HP 27252A PC LAN Adapter/16 TL Plus [AUI/BNC]
 *
 * The board may be re-configured by running the 'hplanset' utility
 * supplied on the DOS disk with the board.
 *
 */

#define HPP_NPORTS	32	/* I think */

/*
 * Valid IRQs
 */
/* IRQs valid, one of 3-7, 9-12, or 15 */
#define HPP_IRQS (IRQ3|IRQ4|IRQ5|IRQ6|IRQ7|IRQ9|IRQ10|IRQ11|IRQ12|IRQ15)
#define HPP_IRQVALID(i)  (HPP_IRQS & (1<<(i)))

/*
 * Valid base addresses
 */

/* require base addr to be 0x{1,2,3}{even}{0} */
#define HPP_IOBASEVALID(b) (((b)&0xfc1f)==0x0000)

/*
 * PORT definitions as in the following picture (16 bit wide words here):
 * Note the 0 starts at the IO Base address
 *
 *               ----|----
 *       0       |   |   |       ID
 *               ----|----
 *       2       |   |   |       Paging
 *               ----|----
 *       4       |   |   |       Option
 *               ----|----
 *       6       |   |   |       ???	(not used)
 *               ----|----
 *       8       |   |   |       Page 0
 *               ----|----
 *       A       |   |   |       Page 2
 *               ----|----
 *       C       |   |   |       Page 4
 *               ----|----
 *       E       |   |   |       Page 6
 *               ----|----
 *       10      |   |   |       Start of 8390 registers
 *               ----|----
 */

/* port definitions [e.g. iobase + these offsets] */
/* the first four registers are visible in all contexts */

#define	HPP_ID		0x00	/* used only for board ID */
#define	HPP_PAGING	0x02	/* set context -- see below */
#define	HPP_OPTION	0x04	/* see below */
#define	HPP_UNKNOWN	0x06	/* unknown/not used */

/*
 * paging works as follows:
 *	each page is 256 bytes in length
 *	there are 128 total pages, giving us an overall on-board buffering
 *	capacity of 32KB.
 *	We wind up getting 1 xmit buffer in 6 pages
 *	and 20 receive buffers in 122 pages.
 *
 * All buffer access to the card must be performed with the 'performance page'
 * context established.  In that case, the board interprets any memory
 * access within the 2K range as a read or write from/to a *single* data
 * port.  Thus, this isn't really a conventional memory-mapped board.
 */

#define	HPP_PAGE0	0x08	/* write pointer in Perf context */
#define	HPP_PAGE1	0x09	/* xx */
#define	HPP_PAGE2	0x0a	/* read pointer in Perf context */
#define	HPP_PAGE3	0x0b	/* xx */
#define	HPP_PAGE4	0x0c	/* data I/O register for PIO */
#define	HPP_PAGE5	0x0d	/* xx */
#define	HPP_PAGE6	0x0e	/* not used? */

/*
 * this is where the ethernet chip control registers start
 */
#define	HPP_ENBASE	0x10	/* start of ns8390 EM registers */

/* paging definitions, write to HPP_PAGING to establish context */
#define	HPP_PAGE_PERF		0x00
#define	HPP_PAGE_MAC		0x01
#define	HPP_PAGE_HW		0x02
#define	HPP_PAGE_LAN		0x04
#define	HPP_PAGE_ID		0x06

/* option bits, write to HPP_OPTION in any context  */
#define	HPP_OPT_MMAP_DIS	0x1000	/* disable memory mapping */
#define	HPP_OPT_ZWAIT_ON	0x0080	/* enable 0-wait state operation */
#define	HPP_OPT_MEM_ON		0x0040	/* to check if memory enabled */
#define	HPP_OPT_IO_ON		0x0020	/* not used */
#define	HPP_OPT_BOOTROM_ON	0x0010	/* to disable bootrom mapping */
#define	HPP_OPT_FAKEINT		0x0008	/* not used */
#define	HPP_OPT_INEA		0x0004	/* enable interrupts */
#define	HPP_OPT_HWRST		0x0002	/* hardware reset */
#define	HPP_OPT_NICRST		0x0001	/* card reset */

/* shared memory management--specific to this driver */
#define	HPP_TX_START_PG		0x00
#define	HPP_RCV_START_PG	0x06
#define	HPP_RCV_STOP_PG		0x80

/* identifying the board and its configuration, compare at HPP_ID */
#define	HPP_IDBYTE0		0x50
#define	HPP_IDBYTE1		0x48
#define	HPP_IDBYTE3		0x53
#define	HPP_PAGEMASK		0xf0
#define	HPP_IDBYTE2		0x00

/* hpp_aui bit indicates if it's twisted pair or thinnet */
/* written to PAGE0 when in HPP_LAN_PAGE context */
#define	HPP_AUI_TL		0x40	/* ThinLan (cheapernet) */
#define	HPP_AUI_TP		0x01	/* Twisted Pair */

/* specific to the un*x driver */
#define	HPP_DEVNAME		"hpp"
#define	HPP_MEM_SIZE		2048

/* allow C8000, CC000, D0000, D4000, ..., EC000 */
/* (this actually lets some more scrape through, but oh well */
#define	HPP_MEMVALID(addr)		(((addr)&0x12fff) == 0x00)
#define	HPP_MEMALIGNED(addr)		(((addr)&0x1fff) == 0x00)
