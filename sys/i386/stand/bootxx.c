/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: bootxx.c,v 1.10 1994/01/05 18:48:38 karels Exp $
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
 *	@(#)bootxx.c	5.2 (Berkeley) 4/28/91
 */

#include "param.h"
#include <a.out.h>
#include "saio.h"
#include "reboot.h"
#include "disklabel.h"

char *bootprog = "/boot";
extern int opendev, bootdev, cyloffset;
extern struct disklabel disklabel;

/*
 * We grab the bios disk parameters early in bootwd, before clearing bss;
 * biosgeom must be in data space so it doesn't get cleared after being set.
 * These are then passed on to boot.  Note that we do this regardless
 * of whether we boot from wd-type disks, as we might boot initially
 * from another disk type.
 */
struct bootparams {
	struct	bootparamhdr hdr;
	struct	bootparam geomhdr;
	struct	biosgeom biosgeom[2];
	struct	bootparam biosinfohdr;
	struct	biosinfo biosinfo;
#ifdef BSDGEOM
	struct	bootparam bsdgeomhdr;
	struct	bsdgeom bsdgeom;
#endif
} bootparams = {
	{ BOOT_MAGIC, sizeof(bootparams) },
	{ B_BIOSGEOM, sizeof(struct bootparam) + 2 * sizeof(struct biosgeom) },
	{ 0 },
	{ B_BIOSINFO, sizeof(struct bootparam) + sizeof(struct biosinfo) },
	{ 0, },
#ifdef BSDGEOM
	{ B_BSDGEOM, sizeof(struct bootparam) + sizeof(struct bsdgeom) },
	{ 0, },
#endif
};

struct biosinfo *biosinfo = &bootparams.biosinfo;
#ifdef BSDGEOM
struct bsdgeom *bsdgeom = &bootparams.bsdgeom;
#endif

/*
 * Stuff to handle BIOS interrupt table with 16-bit "far" pointers;
 * used to fetch the BIOS disk parameters.
 */
struct farptr {
	u_short	off;
	u_short seg;
};

#define	FAR2PTR(far)	(((u_long)((far)->seg) << 4) + (far)->off)

/*
 * Called before main, and before bss is cleared, to grab the bios
 * disk parameters.  We pass these to boot and the kernel for use
 * as default geometry.
 */
getbiosgeom()
{
	struct biosgeom *p;

	if (p = (struct biosgeom *) FAR2PTR((struct farptr *) BIOSGEOM0))
		bootparams.biosgeom[0] = *p;
	if (p = (struct biosgeom *) FAR2PTR((struct farptr *) BIOSGEOM1))
		bootparams.biosgeom[1] = *p;
	/* save BIOS keyboard status for numlock */
	bootparams.biosinfo.kbflags = *BIOSKBFLAGP;
}

void boot_file __P((int io, int howto));

/*
 * Boot program... loads /boot out of filesystem indicated by arguements.
 * We assume an autoboot unless we detect a misconfiguration.
 */
void
main(howto, dev, off)	/* was (dev_type, unit, off) */
	int howto, dev, off;
{
	register struct disklabel *lp;
	register struct partition *pp;
	register int io, part;

	bootdev = dev;
	cyloffset = off;

	/*
	 * Compute partition number from offset.
	 * The units of "off" might be cylinders or sectors,
	 * depending on the type of disk; the block-0 bootstrap
	 * doesn't have complete geometry info.
	 */
	lp = &disklabel;
	pp = &disklabel.d_partitions[0];
	if (lp->d_magic == DISKMAGIC) {
	    for (part = 0; part < MAXPARTITIONS; part++, pp++)
		if (pp->p_offset == off * lp->d_secpercyl ||
		    pp->p_offset == off) {
			/*
			 * Fill in partition.
			 * Currently, PC has no way of booting
			 * from alternate adaptors or controllers.
			 */
			bootdev = MAKEBOOTDEV(B_TYPE(bootdev),
			    B_ADAPTOR(bootdev), B_CONTROLLER(bootdev),
			    B_UNIT(bootdev), part);
#ifdef BSDGEOM
			bootparams.bsdgeom.bsd_startsec = pp->p_offset;
#endif
			break;
		}
	}

	printf("loading %s", bootprog);
	io = open(bootprog, 0);
	if (io >= 0)
		boot_file(io, howto);
	_stop(": not found\n");
}

/*ARGSUSED*/
void
boot_file(io, howto)
	int io, howto;
{
	struct exec x;
	register int i;
	char *addr;

	i = read(io, (char *)&x, sizeof x);
	if (i != sizeof(x) ||
	    (x.a_magic != 0407 && x.a_magic != 0413 && x.a_magic != 0410))
		_stop(": bad format\n");

	if (x.a_magic == 0413 && lseek(io, CLBYTES, 0) == -1)
		goto shread;

	if (read(io, (char *)0, x.a_text) != x.a_text)
		goto shread;

	addr = (char *)x.a_text;
	if (x.a_magic == 0413 || x.a_magic == 0410)
		while ((int)addr & CLOFSET)
			*addr++ = 0;

	if (read(io, addr, x.a_data) != x.a_data)
		goto shread;

	addr += x.a_data;
	for (i = 0; i < x.a_bss; i++)
		*addr++ = 0;

 	printf("\n");
 	(*((int (*)()) x.a_entry))(howto, bootdev, cyloffset, bootparams);
	_stop("Boot returned?\n");
shread:
	_stop(": short read\n");
}
