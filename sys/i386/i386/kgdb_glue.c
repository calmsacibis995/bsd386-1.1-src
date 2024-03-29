/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: kgdb_glue.c,v 1.7 1993/09/08 21:27:36 karels Exp $
 *
 * This code is derived from software from the Lawrence Berkeley Laboratory.
 * This version of the kgdb protocol is far from correct, and a new version
 * is now in beta test.  Any serious use or modifications should be done
 * using the new version, which has a much better protocol.
 */

/*
 * Copyright (c) 1991 Regents of the University of California.
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
 *
 *	@(#)kgdb_glue.c	7.2 (Berkeley) 5/13/91
 */

/*
 * This file must be compiled with gcc -fno-defer-pop.
 */

#ifdef KGDB

#include "param.h"
#include "machine/frame.h"
#include "machine/reg.h"

#ifndef lint
static char rcsid[] =
    "@(#) $Header: /bsdi/MASTER/BSDI_OS/sys/i386/i386/kgdb_glue.c,v 1.7 1993/09/08 21:27:36 karels Exp $";
#endif

#define KGDB_STACKSIZE 0x800
#define KGDB_STACKWORDS (KGDB_STACKSIZE / sizeof(u_long))

u_long kgdb_stack[KGDB_STACKWORDS];

#define getsp(v) asm volatile ("movl %%esp,%0" : "=r" (v))
#define setsp(v) asm volatile ("movl %0,%%esp" :: "r" (v))

static inline void
copywords(src, dst, nbytes)
	register u_long *src, *dst;
	register u_int nbytes;
{
	u_long *limit = src + (nbytes / sizeof(u_long));

	do {
		*dst++ = *src++;
	} while (src < limit);
	if (nbytes & 2)
		*(u_short *)dst = *(u_short *)src;
}

void
kgdb_trap_glue(frame)
	struct trapframe frame;
{
	u_long osp, nsp;

	/*
	 * On internal traps, the hardware doesn't push ss and esp.
	 */
	u_int fsize = sizeof frame - 2 * sizeof (int);

	/*
	 * After a kernel mode trap, the saved sp doesn't point to the right
	 * place.  The correct value is the top of the frame.
	 */
	frame.tf_isp = (u_long)&frame + fsize;

	/*
	 * Copy the interrupt context and frame to the new stack.
	 * We're throwing away trap()'s frame since we're going to do
	 * our own iret.
	 */
	nsp = (u_long)&kgdb_stack[KGDB_STACKWORDS] - fsize;

	copywords((u_long *)&frame, (u_long *)nsp, fsize);

	getsp(osp);
	setsp(nsp);

	if (kgdb_trap(frame.tf_trapno, (struct frame *)nsp) == 0) {
		/*
		 * Get back on kernel stack.  This thread of control
		 * will return back up through trap().  If kgdb_trap()
		 * returns 0, it didn't handle the trap at all so
		 * the stack is still intact and everything will
		 * unwind okay from here up.
		 */
		setsp(osp);
		return;
	}
	/*
	 * Copy back context, which has possibly changed.  Even the
	 * sp might have changed.
	 */
	osp = ((struct trapframe *)nsp)->tf_isp - fsize;
	copywords((u_long *)nsp, (u_long *)osp, fsize);
	setsp(osp);

	/*
	 * Restore the possible new context from frame.
	 */
	asm volatile ("pop %es; pop %ds; popal; nop; addl $8,%esp; iret");
}
#endif
