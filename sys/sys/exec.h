/*	BSDI $Id: exec.h,v 1.5 1994/02/06 20:10:18 karels Exp $	*/

/*-
 * Copyright (c) 1982, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * (c) UNIX System Laboratories, Inc.  All or some portions of this file
 * are derived from material licensed to the University of California by
 * American Telephone and Telegraph Co. or UNIX System Laboratories, Inc.
 * and are reproduced herein with the permission of UNIX System Laboratories,
 * Inc.
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
 *	@(#)exec.h	7.5 (Berkeley) 2/15/91
 */

#ifndef	_EXEC_H_
#define	_EXEC_H_

/* Header prepended to each a.out file. */
struct exec {
#if !defined(vax) && !defined(tahoe) && !defined(i386)
unsigned short	a_mid;		/* machine ID */
unsigned short	a_magic;	/* magic number */
#else
	 long	a_magic;	/* magic number */
#endif
unsigned long	a_text;		/* text segment size */
unsigned long	a_data;		/* initialized data size */
unsigned long	a_bss;		/* uninitialized data size */
unsigned long	a_syms;		/* symbol table size */
unsigned long	a_entry;	/* entry point */
unsigned long	a_trsize;	/* text relocation size */
unsigned long	a_drsize;	/* data relocation size */
};
#define	a_machtype	a_mid	/* SUN compatibility */

/* a_magic */
#define	OMAGIC		0407	/* old impure format */
#define	NMAGIC		0410	/* read-only text */
#define	ZMAGIC		0413	/* demand load format */
#define	QMAGIC		0314	/* compact demand load format */

/* a_mid */
#define	MID_ZERO	0	/* unknown - implementation dependent */
#define	MID_SUN010	1	/* sun 68010/68020 binary */
#define	MID_SUN020	2	/* sun 68020-only binary */
#define	MID_HP200	200	/* hp200 (68010) BSD binary */
#define	MID_HP300	300	/* hp300 (68020+68881) BSD binary */
#define	MID_HPUX	0x20C	/* hp200/300 HP-UX binary */
#define	MID_HPUX800     0x20B   /* hp800 HP-UX binary */

/*
 * The following structure is found at the top of the user stack of each
 * user process. The ps program uses it to locate argv and environment
 * strings. Programs that wish ps to display other information may modify
 * it; normally ps_argv points to argv[0], and ps_argc is the same
 * as the program's argc.  The fields ps_envp and ps_nenv are the
 * equivalent for the environment.
 */
struct ps_strings {
	char	**ps_argv;	/* first of 0 or more argument pointers */
	int	ps_argc;	/* the number of argument pointers */
	char	**ps_envp;	/* first of 0 or more environment pointers */
	int	ps_nenv;	/* the number of environment pointers */
};

/* address of ps_strings structure (in user space) */
#define		PS_STRINGS	(USRSTACK - sizeof(struct ps_strings))

#endif /* !_EXEC_H_ */
