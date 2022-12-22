/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: swapgeneric.c,v 1.8 1992/10/25 21:21:10 karels Exp $
 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
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
 *	@(#)swapgeneric.c	7.5 (Berkeley) 5/7/91
 */

#include "sys/param.h"
#include "sys/conf.h"
#include "sys/buf.h"
#include "sys/systm.h"
#include "sys/reboot.h"
#include "sys/device.h"

#include "i386/isa/isavar.h"

/*
 * Generic configuration;  all in one.
 * This works only if GENERIC is defined.
 */
#ifndef GENERIC
#error CONFIGURATION ERROR: using swap generic without GENERIC defined
CONFIGURATION_ERROR
CONFIGURATION_ERROR_using_swap_generic_without_GENERIC_defined
Non-generic kernels should use configurations like "root on wd0a", etc.
#endif

dev_t	rootdev = NODEV;
dev_t	dumpdev = NODEV;
int	nswap;
struct	swdevt swdevt[] = {
	{ -1,	1,	0 },
	{ 0,	0,	0 },
};
int	dmmin, dmmax, dmtext;

extern	struct cfdriver	wdcd;
extern	struct cfdriver	fdcd;
extern	struct cfdriver	sdcd;	
extern	struct cfdriver	mcdcd;	
extern	struct cfdata cfdata[];

struct	genericconf {
	struct cfdriver *gc_driver;
	dev_t	gc_root;
} genericconf[] = {
	{ &wdcd,	makedev(0, 0),	},
	{ &fdcd,	makedev(2, 0),	},
	{ &sdcd,	makedev(4, 0),	},
	{ &mcdcd,	makedev(6, 0),	},
	{ 0 },
};

setconf()
{
	struct cfdata *dvp;
	register struct genericconf *gc;
	register char *cp;
	int namelen, unit, swaponroot = 0;

	if (rootdev != NODEV)
		goto doswap;
	unit = 0;
	if (boothowto & RB_ASKNAME) {
		char name[128];
retry:
		printf("root device? ");
		gets(name);
		for (gc = genericconf; gc->gc_driver; gc++) {
			namelen = strlen(gc->gc_driver->cd_name);
			cp = &name[namelen];
			if (strncmp(name, gc->gc_driver->cd_name,
			    namelen) == 0 && *cp >= '0' && *cp <= '9')
			    	goto gotit;
		}
		printf("use one of:");
		for (gc = genericconf; gc->gc_driver; gc++)
			printf(" %s%%d", gc->gc_driver->cd_name);
		printf("\n");
		goto retry;
gotit:
		while (*cp >= '0' && *cp <= '9')
			unit = 10 * unit + *cp++ - '0';
		if (*cp == '*')
			swaponroot++;
		goto found;
	}
	for (gc = genericconf; gc->gc_driver; gc++) {
		for (dvp = cfdata; dvp->cf_driver; dvp++) {
			if (dvp->cf_fstate != FSTATE_FOUND)
				continue;
			if (dvp->cf_unit == unit && dvp->cf_driver ==
			    gc->gc_driver) {
				printf("root on %s0\n", 
				    gc->gc_driver->cd_name);
				goto found;
			}
		}
	}
	printf("no suitable root\n");
	asm("hlt");
found:
	gc->gc_root = makedev(major(gc->gc_root), unit * 8);	/* XXX */
	rootdev = gc->gc_root;
doswap:
	swdevt[0].sw_dev = dumpdev =
	    makedev(major(rootdev), minor(rootdev) + 1);
	/* swap size and dumplo set during autoconfigure */
	if (swaponroot)
		rootdev = dumpdev;
}

gets(cp)
	char *cp;
{
	register char *lp;
	register c;

	lp = cp;
	for (;;) {
		cnputc(c = cngetc());
		switch (c) {
		case '\n':
		case '\r':
			*lp++ = '\0';
			return;
		case '\b':
		case '\177':
			if (lp > cp) {
				lp--;
				cnputc(' ');
				cnputc('\b');
			}
			continue;
		case '#':
			lp--;
			if (lp < cp)
				lp = cp;
			continue;
		case '@':
		case 'u'&037:
			lp = cp;
			cnputc('\n');
			continue;
		default:
			*lp++ = c;
		}
	}
}
