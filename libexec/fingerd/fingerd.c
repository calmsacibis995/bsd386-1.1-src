/*	BSDI $Id: fingerd.c,v 1.2 1991/12/14 20:26:49 donn Exp $	*/
/*
 * Copyright (c) 1983 The Regents of the University of California.
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
"@(#) Copyright (c) 1983 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static const char sccsid[] = "@(#)fingerd.c	5.6 (Berkeley) 6/1/90";
static const char rcsid[] = "BSDI $Id: fingerd.c,v 1.2 1991/12/14 20:26:49 donn Exp $";
#endif /* not lint */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pathnames.h"

#ifdef LOGGING
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

void fatal __P((const char *));

#define	LINELEN		1024
#define	ENTRIES		50

int
main()
{
	register FILE *fp;
	register int ch;
	register char *lp;
	int p[2];
	char **ap, *av[ENTRIES + 1], line[LINELEN + 1];

#ifdef LOGGING					/* unused for now */
	struct sockaddr_in sin;
	int sval = sizeof sin;
	struct hostent *hp;
	FILE *fplog;

	if (getpeername(0, (struct sockaddr *)&sin, &sval) < 0)
		fatal("getpeername");
#endif

	if (!fgets(line, sizeof(line), stdin))
		exit(1);

#ifdef LOGGING
	/*
	 * _PATH_FINGER_LOG must exist & be owned by whoever this process
	 * runs as (usually `nobody') for this fopen to succeed.
	 */
	if ((fplog = fopen(_PATH_FINGER_LOG, "a")) != NULL) {
		struct timeval tp;
		char *lastp = &line[strlen(line)];

		(void) gettimeofday(&tp, (struct timezone *)0);
		hp = gethostbyaddr((char *) &sin.sin_addr,
		     sizeof(sin.sin_addr), sin.sin_family);
		(void) fprintf(fplog, "%.15s %s: ", ctime(&tp.tv_sec)+4,
			       (hp == NULL)? inet_ntoa(sin.sin_addr):
					     hp->h_name);
		if (lastp[-1] != '\n')
			*lastp++ = '\n';
		fwrite(line, sizeof line[0], lastp - line, fplog);
		(void) fclose(fplog);
	}
#endif

	av[0] = "finger";
	for (lp = line, ap = &av[1];;) {
		*ap = strtok(lp, " \t\r\n");
		if (!*ap)
			break;
		/* RFC742: "/[Ww]" == "-l" */
		if ((*ap)[0] == '/' && ((*ap)[1] == 'W' || (*ap)[1] == 'w'))
			*ap = "-l";
		if (++ap == av + ENTRIES)
			break;
		lp = NULL;
	}

	if (pipe(p) < 0)
		fatal("pipe");

	switch (fork()) {
	case 0:
		(void)close(p[0]);
		if (p[1] != 1) {
			(void)dup2(p[1], 1);
			(void)close(p[1]);
		}
		execv(_PATH_FINGER, av);
		_exit(1);
	case -1:
		fatal("fork");
	}
	(void)close(p[1]);
	if (!(fp = fdopen(p[0], "r")))
		fatal("fdopen");
	while ((ch = getc(fp)) != EOF) {
		if (ch == '\n')
			putchar('\r');
		putchar(ch);
	}
	exit(0);
}

void
fatal(msg)
	const char *msg;
{

	fprintf(stderr, "fingerd: %s: %s\r\n", msg, strerror(errno));
	exit(1);
}
