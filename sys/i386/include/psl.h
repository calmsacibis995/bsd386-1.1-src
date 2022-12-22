/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: psl.h,v 1.5 1993/08/23 01:09:15 karels Exp $
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)psl.h	5.2 (Berkeley) 1/18/91
 */

/*
 * 386 processor status longword, aka eflags.
 */
#define	PSL_C		0x00000001	/* carry bit */
#define	PSL_PF		0x00000004	/* parity bit */
#define	PSL_AF		0x00000010	/* bcd carry bit */
#define	PSL_Z		0x00000040	/* zero bit */
#define	PSL_N		0x00000080	/* negative bit */
#define	PSL_ALLCC	0x000000d5	/* all cc bits - unlikely */
#define	PSL_T		0x00000100	/* trace enable bit */
#define	PSL_I		0x00000200	/* interrupt enable bit */
#define	PSL_D		0x00000400	/* string instruction direction bit */
#define	PSL_V		0x00000800	/* overflow bit */
#define	PSL_IOPL	0x00003000	/* i/o privilege level */
#define	PSL_NT		0x00004000	/* nested task bit */
#define	PSL_RF		0x00010000	/* restart flag bit */
#define	PSL_VM		0x00020000	/* virtual 8086 mode bit */
#define	PSL_AC		0x00040000	/* alignment check */
#define	PSL_ID		0x00200000	/* CPUID instruction supported */

#define PSL_MBZ		0xfff88028	/* intel reserved bits */
#define PSL_MBO		0x00000002	/* "must be one" */

/*
 * These definitions limit the bits that the user can control in the
 * flags register in sigreturn and in ptrace.  Additionally, the
 * PSL_VM bit can be set by carefully constructing a special sigcontext
 * and trap frame.  A mechanism should exist for setting PSL_AC.
 * Note that the PSL_IOPL bit should always be clear for processes
 * that have not been granted IOPL privileges.
 */
#define	PSL_USERSET	(PSL_MBO | PSL_I)
#define	PSL_USERCLR	(PSL_MBZ | PSL_VM)
