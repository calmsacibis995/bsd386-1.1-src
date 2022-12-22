/*	BSDI	$Id: fmt.c,v 1.2 1994/01/05 00:14:43 polk Exp $	*/

/*-
 * Copyright (c) 1992 The Regents of the University of California.
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
static char sccsid[] = "@(#)fmt.c	5.1 (Berkeley) 4/3/92";
#endif /* not lint */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * XXX
 * This is a stub until marc does the real one.
 */
static char *
shquote(argv)
	register char **argv;
{
	register char **p, *dst, *src;
	char *buf;
	int len;

	if (*argv == 0) {
		buf = malloc(1);
		buf[0] = 0;
		return (buf);
	}
	len = 0;
	for (p = argv; (src = *p++) != 0; ) {
		if (*src == 0)
			continue;
		len += strlen(src) + 1;
	}
	buf = malloc(len);
	dst = buf;
	for (p = argv; (src = *p++) != 0; ) {
		if (*src == 0)
			continue;
		while ((*dst++ = *src++) != 0)
			continue;
		dst[-1] = ' ';
	}
	dst[-1] = 0;
	return (buf);
}

static char *
cmdpart(arg0)
	char *arg0;
{
	register char *cp;

	return ((cp = rindex(arg0, '/')) != NULL ? cp + 1 : arg0);
}

char *
fmt_argv(argv, cmd, maxlen)
	register char **argv;
	register char *cmd;
	int maxlen;
{
	register int len;
	register char *ap, *cp, *arg0;

	if (argv == 0 || argv[0] == 0) {
		if (cmd == NULL)
			return ("");
		ap = NULL;
		len = maxlen + 3;
	} else {
		ap = shquote(argv);
		len = strlen(ap) + maxlen + 4;
	}
	if ((cp = malloc(len)) == NULL)
		return (NULL);
	if (ap == NULL)
		sprintf(cp, "(%.*s)", maxlen, cmd);
	else if (strncmp(cmdpart(argv[0]), cmd, maxlen) != 0)
		sprintf(cp, "%s (%.*s)", ap, maxlen, cmd);
	else
		(void) strcpy(cp, ap);
	if (ap)
		free(ap);
	return (cp);
}
