/* Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 * 
 * Derived from software contributed to Berkeley by Peter McIlroy.
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

/* BSDI $Id: tmp.c,v 1.3 1992/09/17 03:11:28 sanders Exp $ */

#define _NAME_TMP "sort.XXXXXXXX"

#include <sys/param.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "pathnames.h"

char * __P(mktemp(char *));
char *envtmp;

FILE *ftmp()
{
	extern char *envtmp;
	sigset_t set, oset;
	static int first = 0;
	FILE *fd;
	char pathb[MAXPATHLEN], *path;
	path = pathb;

	if (!first && !envtmp) {
		envtmp = getenv("TMPDIR");
		first = 1;
	}
	if (envtmp)
		(void)snprintf(path,
		    sizeof(pathb), "%s/%s", envtmp, _NAME_TMP);
	else {
		bcopy(_PATH_SORTTMP, path, sizeof(_PATH_SORTTMP));
	}
	sigfillset(&set);
	(void)sigprocmask(SIG_BLOCK, &set, &oset);
	path = mktemp(path);
	if (!path)
		err("mktemp failed: %s", strerror(errno));
	if (!(fd = fopen(path, "w+r")))
		err("fopen failed: %s: %s", path, strerror(errno));
	(void)unlink(path);

	(void)sigprocmask(SIG_SETMASK, &oset, NULL);
	return(fd);
};
