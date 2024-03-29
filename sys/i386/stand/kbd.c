/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: kbd.c,v 1.6 1994/01/05 04:53:41 karels Exp $
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)kbd.c	7.4 (Berkeley) 5/4/91
 */

#define	L		0x0001	/* locking function */
#define	SHF		0x0002	/* keyboard shift */
#define	ALT		0x0004	/* alternate shift -- alternate chars */
#define	NUM		0x0008	/* numeric shift  cursors vs. numeric */
#define	CTL		0x0010	/* control shift  -- allows ctl function */
#define	CPS		0x0020	/* caps shift -- swaps case of letter */
#define	ASCII		0x0040	/* ascii code for this key */
#define	STP		0x0080	/* stop output */
#define	FUNC		0x0100	/* function key */
#define	SCROLL		0x0200	/* scroll lock key */

#include "saio.h"

u_char inb();

#ifndef KBD
#define KBD	US
#endif
/*
 * Macro magic to #include KBDPREF ## KBD
 */
#undef i386
#ifdef __STDC__ 
/* #define KBDPREF	i386/isa/kbd/pcconstab. */
#define KBDPREF	i386/stand/kbd/pcconstab.
#define	R(a,b)	< ## a ## b ## >
#define	S(a,b)	R(a,b)

#include S(KBDPREF,KBD)

#else /* __STDC__ */

/*#define KBDPREF	i386/isa/kbd/pcconstab */
#define KBDPREF	i386/stand/kbd/pcconstab
#define	D	KBDPREF.KBD
#define	I	<D>
#include I
#endif /* __STDC__ */
#define	i386


#ifdef notdef
struct key {
	u_short action;		/* how this key functions */
	char	ascii[8];	/* ascii result character indexed by shifts */
};
#endif

u_char shfts, ctls, alts, caps, num, stp;

#define	KBDATAP		0x60	/* kbd data port */
#define	KBSTATUSPORT	0x61	/* kbd status */
#define	KBSTATP		0x64	/* kbd status port */
#define	KBINRDY		0x01
#define	KBOUTRDY	0x02

int
kbd(noblock)
	int noblock;
{
	u_char dt, brk, act;
	
loop:
	if (noblock) {
		if ((inb(KBSTATP) & KBINRDY) == 0)
			return (-1);
	} else while((inb(KBSTATP) & KBINRDY) == 0)
		;
	dt = inb(KBDATAP);

	brk = dt & 0x80;	/* brk == 1 on key release */
	dt = dt & 0x7f;		/* keycode */

	act = action[dt];
	if (act&SHF)
		shfts = brk ? 0 : 1;
	if (act&ALT)
		alts = brk ? 0 : 1;
	if (act&NUM)
		if (act&L) {
			/* NUM lock */
			if(!brk)
				num = !num;
		} else
			num = brk ? 0 : 1;
	if (act&CTL)
		ctls = brk ? 0 : 1;
	if (act&CPS)
		if (act&L) {
			/* CAPS lock */
			if(!brk)
				caps = !caps;
		} else
			caps = brk ? 0 : 1;
	if (act&STP)
		if (act&L) {
			if(!brk)
				stp = !stp;
		} else
			stp = brk ? 0 : 1;

	if(ctl && alts && dt == 83)
		/* Give up if we see ctl-alt-del */
		_stop("kbd: saw ctl-alt-del");
	if ((act&ASCII) && !brk) {
		u_char chr;

		if (shfts)
			chr = shift[dt];
		else if (ctls)
			chr = ctl[dt];
		else
			chr = unshift[dt];

		if (alts)
			chr |= 0x80;

		if (caps && (chr >= 'a' && chr <= 'z'))
			chr -= 'a' - 'A' ;
		return (chr);
	}
	goto loop;
}

scankbd() {
	return (kbd(1) != -1);
}

kbdreset()
{
	u_char c;
	int i = 1000000;

	/* Enable interrupts and keyboard controller */
	while (inb(KBSTATP) & KBOUTRDY)
		;
	outb(KBSTATP,0x60);
	while (inb(KBSTATP) & KBOUTRDY)
		;
	outb(KBDATAP,0x4D);

	/* Start keyboard stuff RESET */
	while (inb(KBSTATP) & KBOUTRDY)
		;	/* wait input ready */
	outb(KBDATAP,0xFF);	/* RESET */

	while (((c = inb(KBDATAP)) != 0xFA) && (i-- > 0))
		;
}

u_char getchar() {
	u_char c;

	c = kbd(0);
	if (c == '\025' || c == '\027' || c == '\b' || c == '\177')
		return(c);
	if (c == '\r')
		c = '\n';
	putchar(c);
	return(c);
}

reset_cpu() {

	while (inb(KBSTATP) & KBOUTRDY);	/* wait input ready */
	outb(KBSTATP,0xFE);	/* Reset Command */
	wait(4000000);
	/* NOTREACHED */
}
