/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: if_wereg.h,v 1.13 1994/01/06 00:15:14 karels Exp $
 */

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Tim L. Tucker.
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
 *	California, Berkeley and its contributors.
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
 *	@(#)if_wereg.h	7.1 (Berkeley) 5/9/91
 */

/*
 * Definitions for if_we.c support of three card types:
 * - Western Digital/SMC 8003/8013 (Elite) Ethernet/Starlan adapter 
 *   (using WD83C690 Ether chip, WD83C584 bus interface chip)
 * - SMC Ultra Ethernet adapter 
 *   (using SMC 83C790 Ethernet chip, incorporating bus interface)
 * - 3Com 3C503 Etherlink II Ethernet adapter
 *
 * All are based on related chips compatible with the DS8390
 * and use a shared-memory interface.
 */

/* Common constants */
#define WE_TXBUF_SIZE	1536	/* TX buffer size */


/*----------------------------------------------------------------------
 * Western Digital/SMC 8003/8013 (Elite) Ethernet/Starlan adapter 
 */
#define WD_VMEM_OFFSET	0x0	/* memory starts from 0 */

#define WD_NPORT	32
 
/*
 * Memory Select Register (MSR)
 */
#define WD_MSR		0x0

#define WDMSR_ADDR	0x3f	/* 6 memory decode bits (13-18) */
#define WDMSR_MENB	0x40	/* memory enable */
#define WDMSR_RST	0x80	/* reset */

/*
 * Interface Configuration Register (ICR)
 */
#define WD_ICR		0x1

#define WDICR_BIT16	0x01	/* 16-bit interface */
#define WDICR_OAR	0x02	/* select register - 0 = BIO 1 = EAR */
#define WDICR_IR2	0x04	/* select the second set of IRQs */
#define WDICR_MSZ	0x08	/* memory size (0=8k, 1=32k) */
#define WDICR_RLA	0x10	/* recall lLAN address */
#define WDICR_RX7	0x20	/* recall all but i/o and LAN address */
#define WDICR_RIO	0x40	/* recall i/o address */
#define WDICR_STO	0x80	/* store to non-volatile memory */

/*
 * Interrupt Request Register (IRR)
 */
#define WD_IRR		0x4

#define WDIRR_0WS	0x01	/* Enable 0ws on 8-bit bus */
#define WDIRR_OUT1	0x02	/* WD83C584 output pin 1 */
#define WDIRR_OUT2	0x04	/* WD83C584 output pin 2 */
#define WDIRR_OUT3	0x08	/* WD83C584 output pin 3 */
#define WDIRR_FLASH	0x10	/* Flash memory is in the ROM socket */
#define WDIRR_IR	0x60	/* Interrupt req. vector (see WDICR_IR2) */
#define  WDIRR_IRQ2_10	 0x00
#define  WDIRR_IRQ3_11	 0x20
#define  WDIRR_IRQ5_15	 0x40
#define  WDIRR_IRQ7_4	 0x60
#define WDIRR_IEN	0x80	/* Interrupt enable */

/*
 * LA Address Register (LAAR)
 * Only used in 16-bit mode on 16-bit boards.
 * As a19 is always = 1, we can use a constant.
 */
#define	WD_LAAR		0x05

#define	WDLAAR_ADDRHI	0x1f		/* Bits 19-23 of RAM address */
#define	WD_ADDR19	1
#define	WDLAAR_0WS16	0x20		/* 0 wait states in 16-bit mode */
#define	WDLAAR_L16EN	0x40		/* Enable 16-bit operation */
#define	WDLAAR_M16EN	0x80		/* Enable 16-bit memory access */


/*
 * WD Ethernet address (ROM)
 */
#define WD_ROMADDR(i)	(0x8 + (i))

#define WD_CHECKSUM	0xFF		/* ROM checksum */

/*
 * WD Card ID (ROM)
 */
#define WD_CARDID	0xe

/*
 * The WD_* types are one-byte hardware type codes in the WD/SMC cards.
 * We augment this with a manufacturer type in higher bits for pseudo-types
 * such as the 3COM 3C503.
 */
#define	WD_MFR(type)	((type) >> 8)

#define	WD_WD		0		/* mfr. type for WD, known 0 here */
#define	WD_STARLAN	0x02		/* WD8003S StarLAN */
#define	WD_ETHER	0x03		/* WD8003E */
#define	WD_ETHER2	0x05		/* WD8013EBT */
#define	WD_ETHER3	0x27		/* WD8013EB */
#define WD_ETHER4	0x2c		/* WD8013EBP */
#define WD_ETHER5	0x29		/* WD8013EPC */
#define CNET_ETHER	0x07		/* CNET */

#define WD_3Com		0x1		/* mfr. type for 3Com Etherlink II */
#define WD_3C503	(WD_3Com << 8)	/* 3Com Etherlink II */

#define WD_SMC		0x2
#define SMC_ULTRA	(WD_SMC << 8)	/* SMC Ultra 790 based */

/*
 * WD NIC starts from  base + WD_NIC_OFFSET
 */
#define WD_NIC_OFFSET	16		/* i/o base offset to NIC	*/

#define WD_TXBUF_SIZE	6		/* Size of TX buffer in pages */

/*
 * Valid IRQs that we can set - 2/9, 3, 4, 5, 7, 10, 11, 15
 */
#define WD_IRQS		(IRQ2|IRQ3|IRQ4|IRQ5|IRQ7|IRQ10|IRQ11|IRQ15)
#define WD_IRQVALID(i)	(WD_IRQS & (1 << (i)))

/*
 * Valid i/o port base address
 * this board has really lax restrictions...
 */
#define WD_BASEVALID(b)	(((b) & 0x1f) == 0)

/*
 * Constants for tx buffer page numbers.
 */
#define WE_TXBUF0 WD_VMEM_OFFSET
#define WE_TXBUF1 (WD_VMEM_OFFSET+(WE_TXBUF_SIZE / DS_PGSIZE))

/*
 * Max and min shared memory sizes.
 */
#define WD_MAX_MEMSIZE 65536
#define WD_MIN_MEMSIZE 8192


/*-----------------------------------------------------------------------
 * Definitions specific to the SMC Ultra card.
 * (Many of the WD definitions also apply, if the correct registers
 * are mapped; registers 8 through F are controlled by SMCHWR_SWH
 * to be either SMC/790 registers or LAN Address registers).
 */

/*
 * Control Register (CR), like WD_MSR control section
 */
#define	SMC_CR		0x0

#define SMCCR_RST	0x80		/* reset */
#define SMCCR_MENB	0x40		/* memory enable */

/*
 * EEROM Register (EER)
 */
#define SMC_EER		0x01
#define	SMCEER_RC	0x40		/* recall EEPROM */

/*
 * Hardware Support Register (HWR)
 */
#define SMC_HWR		0x4

#define SMCHWR_SWH	0x80	/* Switch reg set: 0- addr, 1- SMC conf */
#define SMCHWR_HOST16	0x10		/* 16-bit host interface */
#define	SMCHWR_NUKE	0x08		/* restart, like power-on reset */

/*
 * BIOS Page Register (BPR), ROM addr and misc.
 */
#define	SMC_BPR		0x5

#define SMCBPR_M16EN	0x80		/* memory 16-bit enable */

/*
 * Interrupt Control Register (ICR)
 */
#define SMC_ICR		0x6

#define	SMCICR_EIL	0x01		/* interrupt enable */
#define SMCICR_SINT	0x08		/* force software interrupt */

/*
 * Revision Register (REV)
 */
#define	SMC_REV		0x7

#define	SMCREV_CHIP	0xf0		/* chip type */
#define SMCREV_CHIP790	0x20		/* chip is 83C790 */
#define	SMCREV_REV	0x0f		/* chip revision */

/*
 * RAM Address Register (RAR)
 */
#define SMC_RAR		0x0b

#define SMCRAR_HRAM	0x80		/* LA20-23 are 1's */
#define	SMCRAR_RAM8K	0x00
#define	SMCRAR_RAM16K	0x10
#define	SMCRAR_RAM32K	0x20
#define	SMCRAR_RAM64K	0x30
#define SMCRAR_RA17	0x40		/* address bit 17 */
#define SMCRAR_LOWADDR	0x0f		/* address bits 13-16 */

/*
 * General Control Register (GCR)
 */
#define	SMC_GCR		0x0d

#define	SMCGCR_ZWSEN	0x20		/* zero wait-state enable */
#define	SMCGCR_GPOUT	0x02		/* if BNC is enabled */
#define	SMCGCR_LIT	0x01		/* on for UTP */

#define	GPR_BNC		SMCGCR_GPOUT
#define	GPR_AUI		0
#define	GPR_FLIP	(GPR_BNC^GPR_AUI)
#define	GPR_MASK	SMCGCR_GPOUT

/*-----------------------------------------------------------------------
 * 3Com 3C503 Etherlink II Ethernet adapter
 */
#define EC_MEM_SIZE	8192

#define EC_VMEM_OFFSET	0x20		/* memory starts from 0x2000 */

#define EC_NPORT	16

/*
 * Description of header of each packet in receive area of shared memory.
 */
#define EN_RBUF_STAT	0x0	/* Received frame status. */
#define EN_RBUF_NXT_PG	0x1	/* Page after this frame */
#define EN_RBUF_SIZE_LO	0x2	/* Length of this frame */
#define EN_RBUF_SIZE_HI	0x3	/* Length of this frame */
#define EN_RBUF_NHDR	0x4	/* Length of above header area */

/*
 * E33 Control registers. (base + 40x)
 */
#define E33G_STARTPG	0x400
#define E33G_STOPPG	0x401
#define E33G_NBURST	0x402
#define E33G_IOBASE	0x403
#define E33G_ROMBASE	0x404
#define E33G_GACFR	0x405
#define E33G_CNTRL	0x406
#define E33G_STATUS	0x407
#define E33G_IDCFR	0x408
#define E33G_DMAAH	0x409
#define E33G_DMAAL	0x40a
#define E33G_VP2	0x40b
#define E33G_VP1	0x40c
#define E33G_VP0	0x40d
#define E33G_FIFOH	0x40e
#define E33G_FIFOL	0x40f

/*
 * Bits in E33G_GACFR register.
 */
#define EGACFR_NORM	0x49
#define EGACFR_IRQOFF	0xc9

/*
 * Bits in E33G_CNTRL register.
 */
#define ECNTRL_RESET    0x01	/* Software reset of ASIC and 8390 */
#define ECNTRL_THIN     0x02	/* Enable thinnet interface */
#define ECNTRL_SAPROM   0x04	/* Map Address PROM */
#define ECNTRL_DBLBFR   0x20	/* FIFO Configuration bit */
#define ECNTRL_OUTPUT   0x40	/* PC->3c503 direction if set */
#define ECNTRL_START    0x80	/* Start DMA Logic */

/*
 * Bits in E33G_STATUS register.
 */
#define	ESTAT_DPRDY	0x80	/* Data port of FIFO ready */
#define ESTAT_UFLW	0x40	/* Tried to read FIFO when it was empty. */
#define ESTAT_OFLW	0x20	/* Tried to write FIFO when it was full */
#define ESTAT_DTC	0x10	/* Terminal count from PC bus DMA Logic */
#define ESTAT_DIP	0x8	/* DMA in progress */

/*
 * Bits in E33G_IDCFR
 */
#define EIDCFR_DRQ1	0x01	/* DRQ1 */
#define EIDCFR_DRQ2	0x02	/* DRQ2 */
#define EIDCFR_DRQ3	0x04	/* DRQ3 */
#define EIDCFR_IRQ2	0x10	/* IRQ2 */
#define EIDCFR_IRQ3	0x20	/* IRQ3 */
#define EIDCFR_IRQ4	0x40	/* IRQ4 */
#define EIDCFR_IRQ5	0x80	/* IRQ5 */

/*
 * Bits in E33G_ROMBASE
 */
#define EROM_MASK	0xf0
#define EROM_C8000	0x10
#define EROM_CC000	0x20
#define EROM_D8000	0x40
#define EROM_DC000	0x80

/*
 * Valid IRQs -- 2, 3, 4, 5, 9
 */
#define EC_IRQS		0x23c
#define EC_IRQVALID(i)	(EC_IRQS & (1 << (i)))

/*
 * Valid i/o port base addresses
 */
#define EC_BASEVALID(b)	((b) == 0x300 || (b) == 0x310 || (b) == 0x330 || \
			 (b) == 0x350 || (b) == 0x250 || (b) == 0x280 || \
			 (b) == 0x2a0 || (b) == 0x2e0)

