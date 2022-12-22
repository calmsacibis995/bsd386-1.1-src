/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

static const char rcsid[] = "$Id: uname.c,v 1.1.1.1 1993/03/08 06:43:54 polk Exp $";
static const char copyright[] = "Copyright (c) 1993 Berkeley Software Design, Inc.";

/* uname - print name of current BSD system, and other interesting stuff
 * vix 16feb93 [original, for bsdi and jay libove]
 */

#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>

#define NODENAME_F	0x01
#define	SYSNAME_F	0x02
#define RELEASE_F	0x04
#define	VERSION_F	0x08
#define	MACHINE_F	0x10
#define	ALL__F		0x1F
#define	DEFAULT__F	SYSNAME_F

static char *ProgName;

static void
Usage()
{
	fprintf(stderr, "usage: %s [-snrvma]\n", ProgName);
	exit(1);
}

main(argc, argv)
	int argc;
	char *argv[];
{
	struct utsname u;
	int c, f;

	ProgName = argv[0];
	f = 0;
	while ((c = getopt(argc, argv, "snrvma")) != EOF) {
		switch (c) {
		case 's':
			f |= SYSNAME_F;
			break;
		case 'n':
			f |= NODENAME_F;
			break;
		case 'r':
			f |= RELEASE_F;
			break;
		case 'v':
			f |= VERSION_F;
			break;
		case 'm':
			f |= MACHINE_F;
			break;
		case 'a':
			f = ALL__F;
			break;
		default:
			Usage();
			/*NOTREACHED*/
		}
	}

	if (uname(&u) < 0) {
		perror("uname");
		exit(1);
		/*NOTREACHED*/
	}

	if (!f) {
		f = DEFAULT__F;
	}

#define P(F,M)				\
	if (f & F) {			\
		fputs(u.M, stdout);	\
		f &= ~F;		\
		if (f) putchar(' ');	\
	}

	P(SYSNAME_F, sysname);
	P(NODENAME_F, nodename);
	P(RELEASE_F, release);
	P(VERSION_F, version);
	P(MACHINE_F, machine);
	putchar('\n');

	exit(0);
}
