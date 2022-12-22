/*	BSDI $Id: lastcomm.c,v 1.2 1993/12/15 07:38:32 donn Exp $	*/

/*
 * Copyright (c) 1980 Regents of the University of California.
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
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)lastcomm.c	5.11 (Berkeley) 6/1/90";
#endif /* not lint */

/*
 * last command
 */
#include <sys/param.h>
#include <sys/acct.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <struct.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>
#include "pathnames.h"

extern char *devname __P((dev_t, mode_t));

struct	acct buf[8192 / sizeof (struct acct)];

time_t	expand __P((unsigned));
char	*flagbits __P((int));
char	*getdev __P((dev_t));
int	ok __P((char **argv, struct acct *acp));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern int optind;
	extern char *optarg;
	register struct acct *acp;
	register int cc;
	off_t o;
	struct stat sb;
	int ch, fd;
	char *acctfile, *user_from_uid();

	acctfile = _PATH_ACCT;
	while ((ch = getopt(argc, argv, "f:")) != EOF)
		switch((char)ch) {
		case 'f':
			acctfile = optarg;
			break;
		case '?':
		default:
			fputs("lastcomm [ -f file ]\n", stderr);
			exit(1);
		}
	argv += optind;

	fd = open(acctfile, O_RDONLY);
	if (fd < 0) {
		perror(acctfile);
		exit(1);
	}
	(void)fstat(fd, &sb);
	setpassent(1);

	o = sb.st_size;
	for (o -= o % sizeof (buf); o >= 0; o -= sizeof (buf)) {
		(void)lseek(fd, o, L_SET);
		cc = read(fd, buf, sizeof (buf));
		if (cc < 0) {
			perror("read");
			break;
		}
		acp = buf + (cc / sizeof (buf[0])) - 1;
		for (; acp >= buf; acp--) {
			register char *cp;
			time_t x;

			if (acp->ac_comm[0] == '\0')
				(void)strcpy(acp->ac_comm, "?");
			for (cp = &acp->ac_comm[0];
			     cp < &acp->ac_comm[fldsiz(acct, ac_comm)] && *cp;
			     cp++)
				if (!isascii(*cp) || iscntrl(*cp))
					*cp = '?';
			if (*argv && !ok(argv, acp))
				continue;
			x = expand(acp->ac_utime) + expand(acp->ac_stime);
			printf("%-*.*s %s %-*s %-*s %6.2f secs %.16s\n",
				fldsiz(acct, ac_comm), fldsiz(acct, ac_comm),
				acp->ac_comm, flagbits(acp->ac_flag),
				UT_NAMESIZE, user_from_uid(acp->ac_uid, 0),
				UT_LINESIZE, getdev(acp->ac_tty),
				x / (double)AHZ, ctime(&acp->ac_btime));
		}
	}

	return (0);
}

time_t
expand(t)
	unsigned t;
{
	register time_t nt;

	nt = t & 017777;
	t >>= 13;
	while (t) {
		t--;
		nt <<= 3;
	}
	return (nt);
}

char *
flagbits(f)
	register int f;
{
	static char flags[20];
	char *p, *strcpy();

#define	BIT(flag, ch)	if (f & flag) *p++ = ch;
	p = strcpy(flags, "-    ");
	BIT(ASU, 'S');
	BIT(AFORK, 'F');
	BIT(ACOMPAT, 'C');
	BIT(ACORE, 'D');
	BIT(AXSIG, 'X');
	return (flags);
}

int
ok(argv, acp)
	register char *argv[];
	register struct acct *acp;
{
	register char *cp;
	char *user_from_uid();

	do {
		cp = user_from_uid(acp->ac_uid, 0);
		if (!strcmp(cp, *argv)) 
			return(1);
		if ((cp = getdev(acp->ac_tty)) && !strcmp(cp, *argv))
			return(1);
		if (!strncmp(acp->ac_comm, *argv, fldsiz(acct, ac_comm)))
			return(1);
	} while (*++argv);
	return (0);
}

char *
getdev(dev)
	dev_t dev;
{
	char *name;

	if (dev == NODEV)
		return ("__");
	if (name = devname(dev, S_IFCHR))
		return (name);
	return ("??");
}
