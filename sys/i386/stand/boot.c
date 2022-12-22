/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: boot.c,v 1.16 1994/01/05 18:46:24 karels Exp $
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
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1990 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)boot.c	7.3 (Berkeley) 5/4/91";
#endif /* not lint */

#include "param.h"
#include "reboot.h"
#include <a.out.h>
#include <setjmp.h>
#include <string.h>
#include <i386/isa/isa.h>
#include "saio.h"

/*
 * Boot program... arguments from lower-level bootstrap determine
 * whether boot stops to ask for system name and which device
 * boot comes from.
 */

#define MAXBOOT		(BRELOC - IOM_SIZE - 8*1024)
#define FLOPPYDEV 2	/* from conf.c */

char	line[100];
char	default_boot[] = DEFAULTBOOT;
extern	int opendev, bootdev, cyloffset;
extern	jmp_buf exception;
extern	int bootdebug;

struct bootparams {
	struct	bootparamhdr hdr;
	u_char	data[BOOT_MAXPARAMS];
} bootparams = {
	{ BOOT_MAGIC, sizeof(struct bootparamhdr) },
};
caddr_t ebootparams = (caddr_t) &bootparams + sizeof(struct bootparamhdr);

void boot_file __P((int io, int howto));
void makebootline __P((void));

void
main(howto, dev, off, bhdr)
	int howto, dev, off;
	struct bootparamhdr bhdr;
{
	int io;
	int n;
	char *cp;
	char *defboot = default_boot, defbuf[512];
	char *fp = NULL, *ep, *lp;
	struct bootparam *bp;

	if ((dev&B_MAGICMASK) == B_DEVMAGIC) {
		bootdev = dev;
		cyloffset = off;
		makebootline();
#ifdef CHANGEFLOPPY
		if (B_TYPE(bootdev) == FLOPPYDEV)
			howto = RB_SINGLE|RB_ASKNAME;
#endif
	} else {
		howto = RB_SINGLE|RB_ASKNAME;
		cyloffset = 0;
	}
	if (bhdr.b_magic == BOOT_MAGIC) {
		setbootparams(&bhdr);
		for (bp = B_FIRSTPARAM(&bootparams.hdr); bp;
		    bp = B_NEXTPARAM(&bootparams.hdr, bp))
		    	switch (bp->b_type) {
			case B_BIOSGEOM:
				setbiosgeom((struct biosgeom *) B_DATA(bp));
				break;
#ifdef BSDGEOM
			case B_BSDGEOM:
				setbsdgeom((struct bsdgeom *) B_DATA(bp));
				break;
#endif
			default:
				break;
		}
	}

	/*
	 * If the file /etc/boot.default exists,
	 * we read it for the default boot path.
	 * We do minimal checking; if the file is not empty,
	 * we do what it says.
	 */
	if ((io = open("/etc/boot.default", 0)) >= 0) {
		if ((n = read(io, defbuf, sizeof(defbuf))) > 0) {
			fp = defbuf;
			ep = fp + n;
		}
		close(io);
	}
	if (_setjmp(exception)) {
		close(io);
		printf("- load aborted\n");
		howto = RB_SINGLE|RB_ASKNAME;
		cyloffset = 0; 
		fp = NULL;
	}
		
	for (;;) {
		line[0] = '\0';
		lp = line;
		if (fp) {
			lp = fp;
			if (*fp != '-') {
				defboot = fp;
				howto = RB_AUTOBOOT;
			}
			for (; fp < ep; fp++) {
				if (*fp == '\n') {
					*fp++ = 0;
					break;
				}
			}
			if (fp >= ep)
				fp = NULL;
			printf("Boot: %s\n", lp);
		} else if (howto & RB_ASKNAME) {
			printf("Boot: ");
			gets(line);

			cyloffset = 0;
		}

		if (*lp == 0) {
			lp = defboot;
			printf("Boot: %s\n", lp);
		}

		if (*lp == '-') {
			dobootopts(lp);
			continue;
		}

		/* process additional flags if any */
		if (cp = (char *)index(lp, ' ')) {
			if (bootflags(cp, &howto) == -1) {
				howto |= RB_ASKNAME;
				continue;
			}
			*cp = '\0';
		}

		if ((io = open(lp, 0)) < 0)
			printf("can't open %s: %s\n", lp,
			    strerror(errno));
		else
			boot_file(io, howto);

		howto = RB_SINGLE|RB_ASKNAME;
		cyloffset = 0; 
	}
}

/*
 * Process boot flags.
 * Supports either getopt-style boolean flags, or a single numeric parameter,
 * with either specifying the complete flags if present (ignoring previous).
 * Use original flags if no flags are actually present.
 * Could use getopt, but would have to construct argc/argv and do most of
 * these checks anyway.
 */
bootflags(cp, pflags)
	char *cp;
	int *pflags;
{
	int first = 1;
	int flags = 0;

	while (*cp) {
		while (*cp == ' ')
			cp++;
		if (*cp == '\0')
			break;
		if (*cp == '-') {
			first = 0;
			while (*++cp)
				switch (*cp) {
				case ' ':
					goto nextarg;
				case 'a':
					flags |= RB_ASKNAME;
					break;
				case 'd':
					flags |= RB_KDB;
					break;
				case 'r':
					flags |= RB_DFLTROOT;
					break;
				case 's':
					flags |= RB_SINGLE;
					break;
				case 'w':
					flags |= RB_RWROOT;
					break;
				default:
					goto usage;
			}
			continue;
		}
		if (first && *cp >= '0' && *cp <= '9') {
			*pflags = strtol(cp, 0, 0);
			return (0);
		}
		goto usage;

	nextarg: ;
	}
	if (first == 0)
		*pflags = flags;
	return (0);

usage:
	printf("usage: boot-file [ -adrsw ]\n");
	return (-1);
}

dobootopts(cp)
	char *cp;
{
	struct bootparam param;
	int val;

	if (strncmp(cp, "-autodebug", sizeof("-autodebug" - 1)) == 0) {
		if (cp = (char *)index(cp, ' ')) {
			val = strtol(cp, 0, 0);
			param.b_type = B_AUTODEBUG;
			param.b_len = sizeof(param) + sizeof(val);
			addbootparam(&param, (void *) &val);
		} else
			printf("usage: -autodebug number\n");
		return;
	}
	if (strncmp(cp, "-bootdebug", sizeof("-bootdebug" - 1)) == 0) {
		if (cp = (char *)index(cp, ' '))
			bootdebug = strtol(cp, 0, 0);
		else
			printf("usage: -bootdebug number\n");
		return;
	}
}

setbootparams(bhp)
	struct bootparamhdr *bhp;
{

	if (bhp->b_len <= sizeof(bootparams)) {
		bcopy(bhp, &bootparams, bhp->b_len);
		ebootparams = (caddr_t) &bootparams + bootparams.hdr.b_len;
	} else
		printf("setbootparams: parameters too large\n");
}

addbootparam(bp, val)
	struct bootparam *bp;
	void *val;
{

	if (ebootparams + bp->b_len <=
	    (caddr_t) &bootparams + sizeof(bootparams)) {
		bcopy(bp, ebootparams, sizeof(*bp));
		bcopy(val, ebootparams + sizeof(*bp), bp->b_len - sizeof(*bp));
		ebootparams += bp->b_len;
		bootparams.hdr.b_len += bp->b_len;
	} else
		printf("boot parameters too large\n");
}

void
boot_file(io, howto)
	int io, howto;
{
	struct exec x;
	int i;
	char *addr, c;

	i = read(io, (char *)&x, sizeof x);
	if (i != sizeof x ||
	    (x.a_magic != 0407 && x.a_magic != 0413 && x.a_magic != 0410)) {
		printf("Bad format\n");
		return;
	}

	if (x.a_text + x.a_data >= MAXBOOT) {
		printf("File too large (%dK limit)\n", MAXBOOT / 1024);
		return;
	}
		
	printf("%d", x.a_text);
	if (x.a_magic == 0413 && lseek(io, CLBYTES, 0) == -1)
		goto shread;
	if (x.a_text > IOM_BEGIN) {
		if (read(io, (char *)0, IOM_BEGIN) != IOM_BEGIN)
			goto shread;
		if (read(io, (char *)IOM_END, x.a_text - IOM_BEGIN) !=
		    x.a_text - IOM_BEGIN)
			goto shread;
	} else if (read(io, (char *)0, x.a_text) != x.a_text)
		goto shread;

	addr = (char *)x.a_text;
	if (addr >= (char *)IOM_BEGIN)
		addr += IOM_SIZE;
	if (x.a_magic == 0413 || x.a_magic == 0410)
		while ((int)addr & CLOFSET)
			*addr++ = 0;
	if (addr == (char *)IOM_BEGIN)
		addr == (char *)IOM_END;
	printf("+%d", x.a_data);
	if (addr < (char *)IOM_BEGIN && addr + x.a_data > (char *)IOM_BEGIN) {
		if (read(io, addr, IOM_BEGIN - (int)addr) !=
		    IOM_BEGIN - (int)addr)
			goto shread;
		if (read(io, (char *) IOM_END,
		    x.a_data - (IOM_BEGIN - (int)addr)) !=
		    x.a_data - (IOM_BEGIN - (int)addr)) 
			goto shread;
	} else if (read(io, addr, x.a_data) != x.a_data)
		goto shread;

	addr += x.a_data;
	if (addr >= (char *)IOM_BEGIN)
		addr += IOM_SIZE;
	printf("+%d", x.a_bss);
	/*
	 * Kludge: if program including bss is too large, don't clear bss;
	 * currently only kernels are that large, and the kernel clears bss.
	 * (So does srt0, at least currently)
	 */
	if (x.a_text + x.a_data + x.a_bss >= MAXBOOT)
		x.a_bss = MAXBOOT - (x.a_text + x.a_data);
	if (addr < (char *)IOM_BEGIN && addr + x.a_bss > (char *)IOM_BEGIN) {
		bzero(addr, IOM_BEGIN - (int)addr);
		bzero((char *)IOM_END, x.a_bss - (IOM_BEGIN - (int)addr));
	} else
		bzero(addr, x.a_bss);

	/* mask high order bits corresponding to relocated system base */
	x.a_entry &= 0x00ffffff;
	printf(" start 0x%x", x.a_entry);

	/* pause briefly in case user wants to abort */
	for (i = 0; i < 100000; i++)
		if (c = scankbd())
			_longjmp(exception,1);
	printf("\n");

	close(io);		/* after last longjmp */
#ifdef CHANGEFLOPPY
	/*
	 * If we booted from a floppy device, we ask about changing 
	 * floppies (so you can mount an alternate root disk)
	 * before continuing.
	 */
	if (B_TYPE(opendev) == FLOPPYDEV) {
		printf("File loaded -- change diskettes if necessary, then hit return");
		while (getchar() != '\n')
			;
	}
#endif

	i = (*((int (*)()) x.a_entry))(howto, opendev, cyloffset, bootparams);

	if (i)
		printf("exit %d\n", i); 
	return;
shread:
	printf("Short read\n");
	return;
}

void
makebootline()
{
	static char x[] = "0123456789abcdef";
	int param[4];
	char linebuf[sizeof line];
	register int i;
	register char *cp = linebuf;
	register char *cp2;

	param[0] = B_ADAPTOR(bootdev);
	param[1] = B_CONTROLLER(bootdev);
	param[2] = B_UNIT(bootdev);
	param[3] = B_PARTITION(bootdev);

	for (cp2 = devsw[B_TYPE(bootdev)].dv_name; *cp2; *cp++ = *cp2++)
		;

	*cp++ = '(';

	for (i = 0; i < sizeof param / sizeof param[0]; ++i) {
		if (i > 0)
			*cp++ = ',';
		if (param[i] >= 0xa) {
			*cp++ = '0';
			*cp++ = 'x';
		}
		if (param[i] >= 0x10) {
			*cp++ = x[param[i] >> 4];
			param[i] &= 0xf;
		}
		*cp++ = x[param[i]];
	}

	*cp++ = ')';

	for (cp2 = default_boot; *cp++ = *cp2++; )
		;

	for (cp = line, cp2 = linebuf; *cp++ = *cp2++; )
		;
}
