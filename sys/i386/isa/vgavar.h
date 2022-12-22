/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: vgavar.h,v 1.7 1993/12/19 04:46:00 karels Exp $
 */

/*
 * Standard VGA IO Port Addresses
 */
#define		VGA_WCOLMODE	0x3C8		/* color mode reg (write) */
#define		VGA_RCOLMODE	0x3C7		/* color mode reg (read) */
#define		VGA_COLMASK	0x3C6		/* color mask register */
#define		VGA_CLUT	0x3C9		/* color lookup table reg */ 

#define		VGA_NPORT	16		/* 0x3C0 - 0x3CF */

/*
 * VGA register/memory parameters
 */
#define		VGA_NCLUTENT	768		/* num of CLUT entries */

#define		VGA_MEMADDR	0xA0000		/* VGA display memory */		
#define		VGA_GRAPHICS	1		/* card in grf mode */
#define		VGA_TEXT	0		/* card in text mode */

struct vga_softc {
	struct	device sc_dev;  		/* base device */
	struct	isadev sc_id;   		/* ISA device */
	int	vga_flags;			/* software flags   */
	int	vga_type;			/* type of display  */
	caddr_t	vga_mem_addr;			/* vga memory addr  */
	int	vga_mem_size;			/* vga memory size  */
	short	vga_io_addr;			/* i/o port address */
	u_char	mode;				/* text (0) or graphics (1) */
	u_char	*vga_screen;			/* save area for fonts */ 
	u_char	clut[VGA_NCLUTENT];		/* color lookup table */
};

#define VGA_DEAD	 0x00	
#define VGA_OPEN         0x02

#define VGAUNIT(d)      ((d) & 0x7)
