/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

static const char rcsid[] = "$Id: vgafont.c,v 1.1.1.1 1993/03/09 15:13:03 polk Exp $";
static const char copyright[] = "Copyright (c) 1993 Berkeley Software Design, Inc.";

/*
 * Load font to EGA or VGA
 *
 * Format of the font file:
 *
 *      # comment
 *      hex-char-code: hex-byte hex-byte hex-byte...
 */

#include <stdio.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "vgaioctl.h"
#include "pathnames.h"

void setvga __P((void));
void resetvga __P((void));
int  inb __P((unsigned));
void outb __P((unsigned, int));
int  gethex __P((char **));

int   vgafd;            /* vga file descriptor */
char *vgabase;          /* vga base memory address */

/*
 * The main routine
 */
main(ac, av)
	int ac;
	char **av;
{
	FILE *ff;
	char iline[256];
	char fn[256];
	int lineno;
	char *p;
	char *pl;
	int ccode, nbytes;
	char *getenv(), *index();
	char *fontname = NULL;

	if (ac > 2) {
		fprintf(stderr, "Usage: vgafont [fontname]\n");
		exit(1);
	}

	if (ac == 2)
		fontname = av[1];
	else {
		fontname = getenv("VGAFONT");
		if (fontname == NULL)
			fontname = "default";
	}

	if (index(fontname, '/') == NULL) {
		(void) snprintf(fn, sizeof fn, "%s/%s",
				_PATH_VGAFONT, fontname);
		fontname = fn;
	}

	if( (ff = fopen(fontname, "r")) == NULL ) {
		fprintf(stderr, "vgafont: cannot open font file %s\n", fontname);
		exit(1);
	}

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	setvga();

	/*
	 * Read the font file
	 */
	lineno = 0;
	while (fgets(iline, sizeof iline, ff) != NULL) {
		lineno++;
		p = iline;
		ccode = gethex(&p);
		if (ccode == -1) {
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == '#' || *p == '\0' || *p == '\n')
				continue;
		syntax:
			resetvga();
			fprintf(stderr, "vgafont: syntax error: %s, line %d\n",
					fontname, lineno);
			exit(1);
		}
		if (ccode & ~0xff) {
			resetvga();
			fprintf(stderr, "vgafont: bad character code %x: %s, line %d\n",
					ccode, fontname, lineno);
			exit(1);
		}
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p++ != ':')
			goto syntax;

		pl = vgabase + ccode * 32;

		nbytes = 0;
		while ((ccode = gethex(&p)) != -1) {
			if (++nbytes > 32) {
				resetvga();
				fprintf(stderr, "vgafont: too much rows per character: %s, line %d\n",
						fontname, lineno);
				exit(1);
			}
			if (ccode & ~0xff) {
				resetvga();
				fprintf(stderr, "vgafont: too much columns per character: %s, line %d\n",
						fontname, lineno);
				exit(1);
			}
			*pl++ = ccode;
		}
		while (nbytes++ < 32)
			*pl++ = 0;
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p != '#' && *p != '\0' && *p != '\n')
			goto syntax;
	}
	resetvga();
	exit(0);
}

int
inb(port)
	unsigned port;
{
	unsigned char x;

	asm volatile("inb %1, %0" : "=a" (x) : "d" ((short)port));
	return ((int) x);
}

void
outb(port, val)
	unsigned port;
	int val;
{
	asm volatile("outb %0, %1" : : "a" ((char)val), "d" ((short)port));
}

int src_mm;

/*
 * Enable font loading mode
 */
void
setvga()
{
	vgafd = open(_PATH_VGA, 2);
	if (vgafd < 0) {
		fprintf(stderr, "vgafont: cannot open %s\n", _PATH_VGA);
		exit(1);
	}
	vgabase = (char *) 0x8000000;
	if (ioctl(vgafd, VGAIOCMAP, &vgabase) < 0) {
		fprintf(stderr, "vgafont: cannot map VGA memory\n");
		exit(1);
	}

	/* Read GC memory mode register */
	outb(0x3ce, 0x6);
	src_mm = inb(0x3cf);
	if (src_mm == 0xff)
		src_mm = 0xe;   /* EGA, sigh; assume color monitor  */

	/* Relocate VGA memory to 64K at a0000 */
	outb(0x3ce, 0x6);       /* GC Misc register */
	outb(0x3cf, 0x4);

	/* Turn on sequential access to the VRAM */
	outb(0x3c4, 0x4);       /* SEQ Memory Mode register */
	outb(0x3c5, 0x6);

	/* Enable memory plane 2 */
	outb(0x3c4, 0x2);       /* SEQ Plane Enable register */
	outb(0x3c5, 0x4);
}

/*
 * Disable font loading mode
 */
void
resetvga()
{
	/* Enable memory planes 0 and 1 */
	outb(0x3c4, 0x2);       /* SEQ Plane Enable register */
	outb(0x3c5, 0x3);

	/* Turn off sequential access to the VRAM */
	outb(0x3c4, 0x4);       /* SEQ Memory Mode register */
	outb(0x3c5, 0x2);

	/* Relocate VGA memory to 32K at b8000 */
	outb(0x3ce, 0x6);       /* GC Misc register */
	outb(0x3cf, src_mm);

	/* Unmap VRAM */
	(void) ioctl(vgafd, VGAIOCUNMAP, &vgabase);
	close(vgafd);
}

/*
 * Get a hexadecimal number
 */
int
gethex(pp)
	char **pp;
{
	register int n = 0;
	register int c;
	register char *s;

	s = *pp;
	while (*s == ' ' || *s == '\t')
		s++;
	for (;;) {
		c = *s;
		if (c >= '0' && c <= '9')
			n = (c - '0') | (n << 4);
		else if(c >= 'A' && c <= 'F')
			n = (c - 'A' + 10) | (n << 4);
		else if(c >= 'a' && c <= 'f')
			n = (c - 'a' + 10) | (n << 4);
		else {
			if (s == *pp)
				n = -1;
			*pp = s;
			return (n);
		}
		s++;
	}
	/*NOTREACHED*/
}
