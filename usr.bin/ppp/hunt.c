/*	BSDI $Id: hunt.c,v 1.1.1.1 1993/03/09 15:14:43 polk Exp $	*/

/*
 * This code is derived from tip source file hunt.c
 *
 *
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

#include "ppp.h"

int FD;
char *uucplock;

static	jmp_buf deadline;
static	int deadfl;

void
dead()
{
	deadfl = 1;
	longjmp(deadline, 1);
}

char *
hunt()
{
	volatile char *cp;
	volatile sig_t f;
	static char buf[100];
	extern char *index();
	char *pp;

	f = signal(SIGALRM, dead);
	for (cp = DV ; cp != 0 ; ) {
		if (pp = index(cp, ','))
			*pp = 0;
		strncpy(buf, cp, sizeof buf);
		if (pp) {
			*pp = ',';
			cp = pp + 1;
		} else
			cp = 0;

		deadfl = 0;
		uucplock = rindex(buf, '/')+1;
		if (uu_lock(uucplock) < 0)
			continue;

		/*
		 * Straight through call units, such as the BIZCOMP,
		 * VADIC and the DF, must indicate they're hardwired in
		 *  order to get an open file descriptor placed in FD.
		 */
		if (setjmp(deadline) == 0) {
			alarm(10);
			FD = open(buf, O_RDWR | O_NONBLOCK);
		}
		alarm(0);
		if (FD < 0) {
			perror(buf);
			deadfl = 1;
		}
		if (!deadfl) {
			set_clocal(FD);
			fcntl(FD, F_SETFL, 0); /* clear O_NONBLOCK */
			ioctl(FD, TIOCEXCL, 0);
			signal(SIGALRM, SIG_DFL);
			return (buf);
		}
		(void)uu_unlock(uucplock);
	}
	signal(SIGALRM, f);
	return (0);
}
