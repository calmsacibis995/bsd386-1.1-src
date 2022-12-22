/*	BSDI $Id: acu.c,v 1.2 1994/01/28 00:30:29 sanders Exp $	*/

/*
 * This code is derived from tthe tip file acu.c
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

#ifndef lint
static char sccsid[] = "@(#)acu.c	5.8 (Berkeley) 3/2/91";
#endif /* not lint */

#include "ppp.h"

static acu_t *acu = NOACU;
static int conflag;
static void acuabort();
static acu_t *acutype();
static jmp_buf jmpbuf;

/*
 * Establish connection for ppp
 *
 * If DU is true, we should dial an ACU whose type is AT.
 * The phone numbers are in PN, and the call unit is in CU.
 */
char *
Connect(dv)
	char *dv;
{
	register char *cp = PN;
	char *phnum, string[256];
	FILE *fd;
	int tried = 0;
	char sc;

	if (!DU)
		return (NOSTR);
	if (CU != 0)
		dv = CU;

	signal(SIGINT, acuabort);
	signal(SIGQUIT, acuabort);
	if (setjmp(jmpbuf)) {
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		fprintf(stderr, "ppp: call aborted\n");
		if (acu != NOACU) {
			if (conflag)
				disconnect(NOSTR);
			else
				(*acu->acu_abort)();
		}
		return ("interrupt");
	}

	if (AT == 0 || (acu = acutype(AT)) == NOACU)
		return ("unknown ACU type");

	while (*cp) {
		for (phnum = cp; *cp && *cp != ','; cp++)
			;
		sc = *cp;
		if (sc)
			*cp++ = '\0';
			
		if (conflag = (*acu->acu_dialer)(phnum, dv)) {
			if (sc)
				cp[-1] = sc;
			signal(SIGINT, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			return (NOSTR);
		}
		tried++;
	}
	if (sc)
		cp[-1] = sc;
	if (tried) {
		(*acu->acu_abort)();
		return ("call failed");
	} else
		return ("missing phone number");
}

disconnect(reason)
	char *reason;
{
	if (!conflag)
		return;
	(*acu->acu_disconnect)();
}

static void
acuabort(s)
{
	signal(s, SIG_IGN);
	longjmp(jmpbuf, 1);
}

static acu_t *
acutype(s)
	register char *s;
{
	register acu_t *p;
	extern acu_t acutable[];

	for (p = acutable; p->acu_name != '\0'; p++)
		if (!strcmp(s, p->acu_name))
			return (p);
	return (NOACU);
}
