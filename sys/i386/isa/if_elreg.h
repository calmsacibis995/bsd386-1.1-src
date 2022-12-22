/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI: $Id: if_elreg.h,v 1.3 1992/08/05 02:48:44 karels Exp $
 */

/*
 * EtherLink 16 (3C507) registers
 */

#define	IFF_BNC	IFF_LINK0	/* BNC/AUI switch */

#define EL_NPORT	16	/* number of i/o ports */

#define	EL_ID	0x100		/* The fixed ID register */

/*
 * The I/O subsystem
 */
#define el_nmd(base, n)	((base)+(n))	/* Network Management Data */
#define el_ctrl(base)	((base)+0x6)	/* Control Register */
#define el_iclr(base)	((base)+0xa)	/* Intrrupt Clear */
#define el_cattn(base)	((base)+0xb)	/* Channel Attention */
#define el_romcnf(base)	((base)+0xd)	/* ROM Configuration Register */
#define el_ramcnf(base)	((base)+0xe)	/* RAM Configuration Register */
#define el_intcnf(base)	((base)+0xf)	/* Interrupt Configuration Register */

/* 3COM adapter signature */
#define EL_SIGN "*3COM*"

/* Control Register bits */
#define EL_VB1		0x0		/* Select NMD bank 1 */
#define EL_VB2		0x1		/* Select NMD bank 2 */
#define EL_VB3		0x2		/* Select NMD bank 3 */
#define EL_VB4		0x3		/* Select NMD bank 4 */
#define EL_IEN		0x4		/* Interrupt Enable */
#define EL_INT		0x8		/* Interrupt latch active */
#define EL_LAE		0x10		/* LA address decode Enable */
#define EL_LBK		0x20		/* Loopback Enable */
#define EL_CA		0x40		/* Channel Attention (obsolete) */
#define EL_NORST	0x80		/* Do not do reset */

/* ROM Configuration Register */
#define EL_NOROM	0x0		/* No boot ROM enabled */
#define EL_ROM8K	0x1		/* 8Kb ROM */
#define EL_ROM16K	0x2		/* 16Kb ROM */
#define EL_ROM32K	0x3		/* 32Kb ROM */
#define	EL_ROMBASE	0x3c		/* ROM base address bits 13-16 */
#define EL_BNC		0x80		/* Enable on-board transceiver */

/* RAM Configuration Register */
#define EL_RAMBASE	0x3f		/* RAM Base address/window size */
#define EL_SAD		0x40		/* SA addres decode Disable */
#define EL_0WS		0x80		/* Enable 0 Wait States */

/* Interrupt Configuration Register */
#define EL_IL		0xf		/* Interrupt Level */
#define EL_RSTCNF	0x10		/* Restart the board conf. logic */
#define EL_ED0		0x10		/* Serial data from EEPROM */
#define EL_ED1		0x20		/* Serial data to EEPROM */
#define EL_ESK		0x40		/* Shift clock to the EEPROM */
#define EL_ECS		0x80		/* Chip select to the EEPROM */

/* The 3C507 card supports IRQ 3, 5, 7, 9, 10, 11, 12, 15 */
#define EL_IRQS 	0x9ea8
#define EL_ILALLOWED(n) ((1 << (n)) & EL_IRQS)  /* Is IRQ n allowed? */

/* Allowed EL RAM base sizes/addresses */
struct el_rambase {
	int	elr_val;	/* Value to store to RAMBASE */
	int	elr_addr;	/* The base address of RAM */
	int 	elr_size;	/* The size of RAM */
};

#define EL_RAMTAB	{ \
	0x0,	0xc0000,	16*1024, \
	0x1,	0xc0000,	32*1024, \
	0x2,	0xc0000,	48*1024, \
	0x3,	0xc0000,	64*1024, \
	0x8,	0xc8000,	16*1024, \
	0x9,	0xc8000,	32*1024, \
	0xa,	0xc8000,	48*1024, \
	0xb,	0xc8000,	64*1024, \
	0x10,	0xd0000,	16*1024, \
	0x11,	0xd0000,	32*1024, \
	0x12,	0xd0000,	48*1024, \
	0x13,	0xd0000,	64*1024, \
	0x18,	0xd8000,	16*1024, \
	0x19,	0xd8000,	32*1024, \
	0x30,	0xf00000,	64*1024, \
	0x31,	0xf20000,	64*1024, \
	0x32,	0xf40000,	64*1024, \
	0x33,	0xf60000,	64*1024, \
	0x38,	0xf80000,	64*1024, \
	0,	0,		0 \
}

/*
 * Static memory allocation in 3COM507's memory
 */
struct el_memory {
	struct iel_scb		Scb;	/* System Control Block */
	struct iel_iscp		Iscp;	/* Intermediate System Control block Pointer */
	struct iel_config	Config;	/* Config command template */
	struct iel_ia_setup 	Iasetup; /* Iasetup command */
	struct iel_mc_setup	Mcsetup; /* Mcsetup command */
};
