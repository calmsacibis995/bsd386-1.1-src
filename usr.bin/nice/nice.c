/*
 * Copyright (c) 1989 The Regents of the University of California.
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
"@(#) Copyright (c) 1989 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)nice.c	5.4 (Berkeley) 6/1/90";
#endif /* not lint */

#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <ctype.h>

#define	DEFNICE	10

/* ARGSUSED */
main(argc, argv)
	int argc;
	char **argv;
{
	extern int errno;
	int niceness;
	char *strerror();

	if (!argv[1])
		usage();

	niceness = DEFNICE;
	if (argv[1][0] == '-')
		if (isdigit(argv[1][1]) ||
		    (argv[1][1] == '-' && isdigit(argv[1][2]))) {
			niceness = atoi(argv[1] + 1);
			++argv;
		} else {
			(void)fprintf(stderr, "nice: illegal option -- %c\n",
			    argv[1][1]);
			usage();
		}

	errno = 0;
	niceness += getpriority(PRIO_PROCESS, 0);
	if (errno) {
		(void)fprintf(stderr, "nice: getpriority: %s\n",
		    strerror(errno));
		exit(1);
	}
	if (setpriority(PRIO_PROCESS, 0, niceness)) {
		(void)fprintf(stderr,
		    "nice: setpriority: %s\n", strerror(errno));
		exit(1);
	}
	execvp(argv[1], &argv[1]);
	(void)fprintf(stderr,
	    "nice: %s: %s\n", argv[1], strerror(errno));
	exit(1);
}

usage()
{
	(void)fprintf(stderr,
	    "nice [ -# ] command [ options ] [ operands ]\n");
	exit(1);
}
