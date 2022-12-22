/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: frame.h,v 1.2 1992/07/27 23:02:17 karels Exp $
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
 *	@(#)frame.h	5.2 (Berkeley) 1/18/91
 */

/*
 * System stack frames.
 */

/*
 * Exception/Trap Stack Frame
 */

struct trapframe {
	int	tf_es;
	int	tf_ds;
	int	tf_edi;
	int	tf_esi;
	int	tf_ebp;
	int	tf_isp;
	int	tf_ebx;
	int	tf_edx;
	int	tf_ecx;
	int	tf_eax;
	int	tf_trapno;
	/* below portion defined in 386 hardware */
	int	tf_err;
	int	tf_eip;
	int	tf_cs;
	int	tf_eflags;
	/* below only when transitting rings (e.g. user to kernel) */
	int	tf_esp;
	int	tf_ss;
};

/* Interrupt stack frame */

struct intrframe {
	int	if_vec;
	int	if_ppl;
	int	if_es;
	int	if_ds;
	int	if_edi;
	int	if_esi;
	int	if_ebp;
	int	:32;
	int	if_ebx;
	int	if_edx;
	int	if_ecx;
	int	if_eax;
	int	:32;		/* for compat with trap frame - trapno */
	int	:32;		/* for compat with trap frame - err */
	/* below portion defined in 386 hardware */
	int	if_eip;
	int	if_cs;
	int	if_eflags;
	/* below only when transitting rings (e.g. user to kernel) */
	int	if_esp;
	int	if_ss;
};

/*
 * Call Gate/System Call Stack Frame
 */

struct syscframe {
	int	sf_edi;
	int	sf_esi;
	int	sf_ebp;
	int	:32;		/* redundant save of isp */
	int	sf_ebx;
	int	sf_edx;
	int	sf_ecx;
	int	sf_eax;
	int	sf_eflags;
	/* below portion defined in 386 hardware */
/*	int	sf_args[N]; 	/* if call gate copy args enabled!*/
	int	sf_eip;
	int	sf_cs;
	/* below only when transitting rings (e.g. user to kernel) */
	int	sf_esp;
	int	sf_ss;
};

/*
 * Superset of trap frame, for traps from virtual-8086 mode
 */
struct trapframe_vm86 {
	int	tf_es386;
	int	tf_ds386;
	u_short	tf_di;
	u_short	tf_di_unused;
	u_short	tf_si;
	u_short	tf_si_unused;
	u_short	tf_bp;
	u_short	tf_bp_unused;
	int	tf_isp386;
	u_short	tf_bx;
	u_short	tf_bx_unused;
	u_short	tf_dx;
	u_short	tf_dx_unused;
	u_short	tf_cx;
	u_short	tf_cx_unused;
	u_short	tf_ax;
	u_short	tf_ax_unused;
	int	tf_trapno;
	/* below portion defined in 386 hardware */
	int	tf_err;
	u_short	tf_ip;
	u_short	tf_ip_unused;
	u_short	tf_cs;
	u_short	tf_cs_unused;
	u_short	tf_eflags86;
	u_short tf_eflags_unused;
	/* below only when transitting rings (e.g. user to kernel) */
	u_short	tf_sp;
	u_short	tf_sp_unused;
	u_short	tf_ss;
	u_short	tf_ss_unused;
	/* below only when transitting from vm86 */
	u_short	tf_es;
	u_short	tf_es_unused;
	u_short	tf_ds;
	u_short	tf_ds_unused;
	u_short	tf_fs;
	u_short	tf_fs_unused;
	u_short	tf_gs;
	u_short	tf_gs_unused;
};

