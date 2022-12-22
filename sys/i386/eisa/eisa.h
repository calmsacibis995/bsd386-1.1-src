/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: eisa.h,v 1.4 1993/12/23 05:30:37 karels Exp $
 */

#define	EISA_ID_OFFSET		0xfffd9
#define	EISA_ID_LEN		4
#define	EISA_ID			"EISA"

#define	EISA_IOBASE(s)		((s) << 12 | 0xc00)
#define	EISA_NPORT		0x100
#define	EISA_SLOTMASK		0xf000

#define	EISA_NUM_PHYS_SLOT	16

#ifdef LOCORE
#define	EISA_PROD_ID_BASE	0xc80
#else
#define	EISA_PROD_ID_BASE(s)	((s) << 12 | 0xc80)
#endif

/*
 * EISA-specific port definitions.
 * Note that EISA cards typically reserve ports 0xc00-0xcff
 * in every slot's range too...
 */
#define	IO_EISABEGIN	0x400

#define	IO_ENMI		0x461	/* extended NMI status port */

#define	IO_ELCR1	0x4d0	/* edge/level control register 0-7 */
#define	IO_ELCR2	0x4d1	/* edge/level control register 8-15 */

#define	IO_EISAEND	0x4ff

#ifndef LOCORE
int eisa_match __P((struct cfdata *cf, struct isa_attach_args *ia));
void eisa_slotalloc __P((int slot));
#ifdef DEBUG
void eisa_print_devmap __P((void));
#endif /* DEBUG */
#endif /* !LOCORE */
