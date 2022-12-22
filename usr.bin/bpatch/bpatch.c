/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1991 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "from @(#)dmesg.c	5.9 (Berkeley) 5/2/91";
#endif /* not lint */

/*
 * bpatch: binary patch
 * Patches a.out files, including kernel, or /dev/kmem for running kernel.
 * Usage:
 *	bpatch [ type-opt ] [ -r ] location [ value ]
 *
 *	type-options (chose at most one, 'l' is default):
 *	    -b	byte as number
 *	    -c	byte as character
 *	    -w	short/u_short
 *	    -l	long/u_long
 *	    -s	string
 *
 *	Other options:
 *	    -r		patch running kernel rather than a.out
 *	    -N namelist	name for kernel namelist file
 *	    -M memfile	name for memory file, implies -r
 *
 *	location
 *	    a symbol name, or a decimal/hex/octal number
 *
 *	value
 *	    depending on the type, a character, string,
 *	    or a decimal/hex/octal number.
 *	    If none, the current value is printed.
 */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <time.h>
#include <a.out.h>
#include <kvm.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <paths.h>

#ifndef	_PATH_KERNEL
#define	_PATH_KERNEL	"/bsd"	/* XXX */
#endif

struct nlist nl[] = {
#define	X_SYSBASE	0
	{ "SYSTEM" },
#define	X_VAR		1
	{ "" },
#define	X_ETEXT		2
	{ "_etext" },
#define	X_EDATA		3
	{ "_edata" },
#define	X_VERSION	4
	{ "_version" },
	{ NULL },
};

void usage();
void err __P((const char *, ...));

enum { UNSPEC, BYTE, CHAR, WORD, LONG, STRING } type = UNSPEC;

#define	MAXSTRING	1024

u_char	cbuf;
u_short	wbuf;
u_long	lbuf;
u_char	sbuf[MAXSTRING + 1];
u_char	vbuf[MAXSTRING + 1];
int	checkversion = 1;	/* check version strings, namelist vs kmem */

main(argc, argv)
	int argc;
	char **argv;
{
	register int ch, newl, skip;
	char *namelist = _PATH_KERNEL, *core, *file;
	struct exec exec;
	int writing = 0, fd, nfd;
	void *p;
	off_t loc;
	char *ep;
	size_t len;

	core = NULL;
	while ((ch = getopt(argc, argv, "bcwlsrM:N:")) != EOF)
		switch (ch) {
		case 'b':
			if (type != UNSPEC)
				usage();
			type = BYTE;
			p = &cbuf;
			len = sizeof(cbuf);
			break;
		case 'c':
			if (type != UNSPEC)
				usage();
			type = CHAR;
			p = &cbuf;
			len = sizeof(cbuf);
			break;
		case 'w':
			if (type != UNSPEC)
				usage();
			type = WORD;
			p = &wbuf;
			len = sizeof(wbuf);
			break;
		case 'l':
			if (type != UNSPEC)
				usage();
			type = LONG;
			p = &lbuf;
			len = sizeof(lbuf);
			break;
		case 's':
			if (type != UNSPEC)
				usage();
			type = STRING;
			p = sbuf;
			len = MAXSTRING;
			break;
		case 'r':
			if (core == NULL)
				core = _PATH_KMEM;
			break;
		case 'M':
			core = optarg;
			checkversion = 0;
			break;
		case 'N':
			namelist = optarg;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if (type == UNSPEC) {
		type = LONG;
		p = &lbuf;
		len = sizeof(lbuf);
	}

	if (argc < 1 || argc > 2)
		usage();
	writing = (argc == 2);

	if (core)
		file = core;
	else
		file = namelist;
	if (writing) {
		if ((fd = open(file, O_RDWR)) == -1)
			err("%s: %s", file, strerror(errno));
	} else {
		if ((fd = open(file, O_RDONLY)) == -1)
			err("%s: %s", file, strerror(errno));
		checkversion = 0;
	}

	nl[X_VAR].n_un.n_name = argv[0];
	if (nlist(namelist, nl) == -1)
		err("nlist: %s: %s",  file, strerror(errno));

	/*
	 * See whether value in argv[0] is numeric or symbolic.
	 */
	errno = 0; 
	loc = strtoul(argv[0], &ep, 0);
	if (errno != ERANGE && *ep == '\0') {
		/* value is number; check whether text/data/otherwise */
		if (nl[X_ETEXT].n_type && loc <= nl[X_ETEXT].n_value)
			nl[X_VAR].n_type = N_TEXT;
		else if (nl[X_EDATA].n_type && loc <= nl[X_EDATA].n_value)
			nl[X_VAR].n_type = N_DATA;
		else
			err("%s: can't determine segment for offset",
			    namelist);
	} else {
		/*
		 * Value must be symbolic.  If not found,
		 * try prepending underscore.
		 */
		if (nl[X_VAR].n_type == 0) {
			nl[X_VAR].n_un.n_name = malloc(strlen(argv[0] + 2));
			if (nl[X_VAR].n_un.n_name)
				sprintf(nl[X_VAR].n_un.n_name, "_%s", argv[0]);
			(void) nlist(namelist, nl);
		}
		if (nl[X_VAR].n_type == 0)
			err("%s not defined in namelist", argv[0]);
        	loc = nl[X_VAR].n_value;
	}

	/*
	 * If patching the binary file, need to figure out file offsets.
	 * If checking version, also need exec header.
	 */
	if (core == NULL || checkversion) {
		if (core == NULL)
			nfd = fd;
		else if ((nfd = open(namelist, O_RDONLY)) == -1)
			err("%s: open: %s", namelist, strerror(errno));
		(void) lseek(nfd, 0, SEEK_SET);
		if (read(nfd, (void *)&exec, sizeof(exec)) != sizeof(exec))
			err("%s: can't read exec header", namelist);
		if (N_BADMAG(exec))
			err("%s: bad number in exec header", namelist);
	}
	if (core == NULL) {
		/* printf("loc %x, sysbase %x\n",
		    loc, nl[X_SYSBASE].n_value); */
		if (nl[X_SYSBASE].n_type)
			loc -= nl[X_SYSBASE].n_value;
		switch (nl[X_VAR].n_type & N_TYPE) {
		case N_TEXT:
			loc = loc - N_TXTADDR(exec) + N_TXTOFF(exec);
			/* printf("loc %x, txtaddr %x, txtoff %x\n",
			    loc, N_TXTADDR(exec), N_TXTOFF(exec)); */
			break;
		case N_DATA:
			loc = loc - N_DATADDR(exec) + N_DATOFF(exec);
			/*printf("loc %x, dataddr %x, datoff %x\n",
			    loc, N_DATADDR(exec), N_DATOFF(exec)); */
			break;
		default:
			err("%s: offset not in file", argv[1]);
			/* NOTREACHED */
		}
	}
 
 	if (!writing) {
 		if (lseek(fd, loc, SEEK_SET) == -1)
 			err("%s: lseek: %s", file, strerror(errno));
        	if (read(fd, p, len) != len)
 			err("%s: can't read at %s", file, argv[0]);
        	switch (type) {
        	case BYTE:
        		(void) printf("0x%x = %d\n", cbuf, cbuf);
        		break;
        	case CHAR:
        		(void) printf("%c\n", cbuf, cbuf);
        		break;
        	case WORD:
        		(void) printf("0x%x = %d\n", wbuf, wbuf);
        		break;
        	case LONG:
        		(void) printf("0x%lx = %d\n", lbuf, lbuf);
        		break;
        	case STRING:
        		(void) printf("%s\n", sbuf);
        		break;
        	}
	} else {
		if (checkversion && core && nl[X_VERSION].n_type) {
			off_t nloc;

			/* check version strings in a.out and memory */
			nloc = nl[X_VERSION].n_value;
			if (nl[X_SYSBASE].n_type)
				nloc -= nl[X_SYSBASE].n_value;
			nloc = nloc - N_DATADDR(exec) + N_DATOFF(exec);
			if (lseek(nfd, nloc, SEEK_SET) == -1)
				err("%s: lseek to version: %s",
				    namelist, strerror(errno));
			if (read(nfd, vbuf, MAXSTRING) != MAXSTRING)
				err("%s: can't read version", namelist);
			if (lseek(fd, nl[X_VERSION].n_value, SEEK_SET) == -1)
				err("%s: lseek to version: %s",
				    core, strerror(errno));
			if (read(fd, sbuf, MAXSTRING) != MAXSTRING)
				err("%s: can't read version", core);
			if (strcmp(vbuf, sbuf))
				err("version string mismatch in %s and %s",
				    namelist, core);
		}
		errno = 0;
        	switch (type) {
        	case BYTE:
        		lbuf = strtol(argv[1], NULL, 0);
        		if (lbuf > UCHAR_MAX || errno == ERANGE)
        			err("%s: value out of range", argv[1]);
        		cbuf = lbuf;
        		break;
        	case CHAR:
        		cbuf = argv[1][0];
        		break;
        	case WORD:
        		lbuf = strtol(argv[1], NULL, 0);
        		if (lbuf > USHRT_MAX || errno == ERANGE)
        			err("%s: value out of range", argv[1]);
        		wbuf = lbuf;
        		break;
        	case LONG:
        		lbuf = strtol(argv[1], NULL, 0);
        		if (errno == ERANGE)
        			err("%s: value out of range", argv[1]);
        		break;
        	case STRING:
        		p = argv[1];
        		len = strlen(p) + 1;
        		break;
        	}
 		if (lseek(fd, loc, SEEK_SET) == -1)
 			err("%s: lseek: %s", file, strerror(errno));
        	if (write(fd, p, len) != len)
 			err("%s: can't write at %s", file, argv[0]);
	}
	exit(0);
}

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

void
#if __STDC__
err(const char *fmt, ...)
#else
err(fmt, va_alist)
	char *fmt;
        va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	(void) fprintf(stderr, "bpatch: ");
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void) fprintf(stderr, "\n");
	exit(1);
	/* NOTREACHED */
}

void
usage()
{
	(void) fprintf(stderr,
"usage: bpatch [-b|c|w|l|s] [-r] [-M core] [-N system] var [ val ]\n");
	exit(1);
}
