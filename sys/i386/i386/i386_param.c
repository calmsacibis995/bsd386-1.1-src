/*-
 * Copyright (c) 1993, 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: i386_param.c,v 1.4 1994/01/30 07:55:46 karels Exp $
 */

/*
 * Configuration-dependent data for i386 kernels.
 */

#include "param.h"
#include "reboot.h"
#include "systm.h"

#include "device.h"
#include "ioctl.h"
#include "tty.h"

#include "cons.h"

/*
 * Root configuration
 */
configroot()
{

#if GENERIC
	if ((boothowto & RB_ASKNAME) == 0)
		setroot();
	setconf();
#else
	setroot();
#endif
}

/*
 * configuration for com driver
 */
#include "com.h"
#if NCOM > 0
#include "../isa/comreg.h"

#ifdef COMCONSOLE
int	comconsole = COMCONSOLE;
#else
int	comconsole = -1;
#endif
#ifndef	COMCNADDR
#define COMCNADDR CONADDR
#endif
int	com_cnaddr = COMCNADDR;		/* console base address */
#endif


/*
 * configuration for pccons console driver
 */
#include "pccons.h"
#if NPCCONS > 0

#include "i386/isa/isavar.h"
#include "i386/isa/pcconsvar.h"

#ifdef PCCONS_NOCOLOR
int	pccons_trycolor = 0;
#else
int	pccons_trycolor = 1;
#endif
int	cntlaltdel = 1;		/* recognize cntl-alt-del, ask reboot? */
#ifdef DEBUG
int	cntlaltdelcore = 1;	/* ask about core dump? */
#else
int	cntlaltdelcore = 0;	/* ask about core dump? */
#endif

#ifndef	KBD
#define	KBD	US
#endif

/*
 * Macro magic to #include KBDPREF ## KBD
 */
#undef i386
#ifdef __STDC__ 
#define KBDPREF	i386/isa/kbd/pcconstab.
#define	R(a,b)	< ## a ## b ## >
#define	S(a,b)	R(a,b)

#include S(KBDPREF,KBD)

#else /* __STDC__ */

#define KBDPREF	i386/isa/kbd/pcconstab
#define	D	KBDPREF.KBD
#define	I	<D>
#include I
#endif /* __STDC__ */
#define	i386
#endif

/* configuration for indirect console driver */
/* XXX - all this could be autoconfig()ed */
#if NPCCONS > 0
int pccnprobe(), pccninit(), pccngetc(), pccnputc();
#endif
#if NCOM > 0
int comcnprobe(), comcninit(), comcngetc(), comcnputc();
#endif

struct	consdev constab[] = {
#if NPCCONS > 0
	{ pccnprobe,	pccninit,	pccngetc,	pccnputc },
#endif
#if NCOM > 0
	{ comcnprobe,	comcninit,	comcngetc,	comcnputc },
#endif
	{ 0 },
};
/* end XXX */

#include "npx.h"
#if NNPX == 0
npxinit(p, control)
	struct proc *p;
	int control;
{
}
#endif
