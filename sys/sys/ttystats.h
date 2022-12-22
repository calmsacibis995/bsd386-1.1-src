/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: ttystats.h,v 1.2 1992/11/19 16:25:46 karels Exp $
 */

#ifndef _TTYSTATS_H_
#define	_TTYSTATS_H_
/* 
 * Global tty statistics;
 * returned by getkerninfo().
 */
struct ttytotals {
	long	tty_nin;
	long	tty_nout;
	long	tty_cancc;
	long	tty_rawcc;
};

/* 
 * (Temporary) Internal tty device description, also getkerninfo()
 * tty_names return structure.  Each of these structures describes
 * a tty device or multiplexor with one of more tty lines associated.
 * This should be redone ala disk devices, but pseudo-devices like pty
 * don't currently have device structures.
 */
struct ttydevice_tmp {
	char	tty_name[8];		/* driver name */
	int	tty_unit;		/* driver (board) unit number */
	int	tty_base;		/* starting line minor number */
	int	tty_count;		/* number of lines */
	struct	tty *tty_ttys;		/* tty structures per line */
	struct	ttydevice_tmp *tty_next;	/* linked list */
};

#ifdef KERNEL
extern	void tty_attach __P((struct ttydevice_tmp *));
#endif /* KERNEL */
#endif /* !_TTYSTATS_H_ */
