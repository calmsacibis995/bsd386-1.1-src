/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: trap.h,v 1.4 1992/11/20 13:44:42 karels Exp $
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
 *	@(#)trap.h	5.4 (Berkeley) 5/9/91
 */

/*
 * Trap type values
 * also known in trap.c for name strings
 * those marked XXX are not currently used
 */

#define	T_RESADFLT	0	/* reserved addressing fault XXX */
#define	T_PRIVINFLT	1	/* illegal instruction fault */
#define	T_RESOPFLT	2	/* reserved operand fault XXX */
#ifdef KERNEL
#define	T_BPTFLT	3	/* breakpoint instruction */
#define	T_UNUSED	4	/* unused XXX */
#define	T_SYSCALL	5	/* system call (kcall) XXX */
#define	T_ARITHTRAP	6	/* arithmetic trap */
#define	T_ASTFLT	7	/* system forced exception */
#define	T_SEGFLT	8	/* segmentation (limit) fault XXX*/
#define	T_PROTFLT	9	/* protection fault */
#define	T_TRCTRAP	10	/* trace trap */
#define	T_UNUSED2	11	/* unused XXX */
#endif
#define	T_PAGEFLT	12	/* page fault */
#ifdef KERNEL
#define	T_TABLEFLT	13	/* page table fault XXX */
#endif
#define	T_ALIGNFLT	14	/* alignment fault */
#ifdef KERNEL
#define	T_KSPNOTVAL	15	/* kernel stack pointer not valid XXX */
#define	T_BUSERR	16	/* bus error XXX */
#define	T_KDBTRAP	17	/* kernel debugger trap XXX */
#define	T_DIVIDE	18	/* integer divide fault */
#define	T_NMI		19	/* non-maskable interrupt */
#define	T_OFLOW		20	/* overflow trap */
#define	T_BOUND		21	/* bounds check */
#define	T_DNA		22	/* device not available fault */
#define	T_DOUBLEFLT	23	/* double fault */
#endif
#define	T_FPOPFLT	24	/* fp coprocessor segment overrun */
#ifdef KERNEL
#define	T_TSSFLT	25	/* invalid tss fault */
#endif
#define	T_SEGNPFLT	26	/* segment not present fault */
#define	T_STKFLT	27	/* stack fault */
#define	T_RESERVED_15	28	/* reserved exeception 15 */
#ifdef KERNEL
#define	T_RESERVED_18	29	/* reserved exeception 18 */
#define	T_RESERVED_19	30	/* reserved exeception 19 */
#define	T_RESERVED_20	31	/* reserved exeception 20 */
#define	T_RESERVED_21	32	/* reserved exeception 21 */
#define	T_RESERVED_22	33	/* reserved exeception 22 */
#define	T_RESERVED_23	34	/* reserved exeception 23 */
#define	T_RESERVED_24	35	/* reserved exeception 24 */
#define	T_RESERVED_25	36	/* reserved exeception 25 */
#define	T_RESERVED_26	37	/* reserved exeception 26 */
#define	T_RESERVED_27	38	/* reserved exeception 27 */
#define	T_RESERVED_28	39	/* reserved exeception 28 */
#define	T_RESERVED_29	40	/* reserved exeception 29 */
#define	T_RESERVED_30	41	/* reserved exeception 30 */
#define	T_RESERVED_31	42	/* reserved exeception 31 */

/* Trap's coming from user mode */
#define	T_USER	0x100
#endif /* KERNEL */

/* definitions for <sys/signal.h> */
#define	    ILL_RESAD_FAULT	T_RESADFLT
#define	    ILL_PRIVIN_FAULT	T_PRIVINFLT
#define	    ILL_RESOP_FAULT	T_RESOPFLT
#define	    ILL_ALIGN_FAULT	T_ALIGNFLT
#define	    ILL_FPOP_FAULT	T_FPOPFLT	/* coproc operand fault */

/* codes for SIGFPE/ARITHTRAP */
#define	    FPE_INTOVF_TRAP	0x1	/* integer overflow */
#define	    FPE_INTDIV_TRAP	0x2	/* integer divide by zero */
#define	    FPE_FLTDIV_TRAP	0x3	/* floating/decimal divide by zero */
#define	    FPE_FLTOVF_TRAP	0x4	/* floating overflow */
#define	    FPE_FLTUND_TRAP	0x5	/* floating underflow */
#define	    FPE_FPU_NP_TRAP	0x6	/* floating point unit not present */
#define	    FPE_SUBRNG_TRAP	0x7	/* subrange out of bounds */

/* codes for SIGBUS; these should be SIGSEGV */
#define	    BUS_PAGE_FAULT	T_PAGEFLT	/* page fault protection base */
#define	    BUS_SEGNP_FAULT	T_SEGNPFLT	/* segment not present */
#define	    BUS_STK_FAULT	T_STKFLT	/* stack segment */
#define	    BUS_SEGM_FAULT	T_RESERVED_15	/* segment protection base */
