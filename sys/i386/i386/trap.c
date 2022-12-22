/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: trap.c,v 1.28 1993/12/18 19:22:14 karels Exp $
 */
 
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the University of Utah, and William Jolitz.
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
 *	@(#)trap.c	7.4 (Berkeley) 5/13/91
 */

/*
 * 386 Trap and System call handling
 */

#include "param.h"
#include "systm.h"
#include "proc.h"
#include "user.h"
#include "seg.h"
#include "acct.h"
#include "kernel.h"
#ifdef KTRACE
#include "ktrace.h"
#endif

#include "machine/cpu.h"
#include "machine/psl.h"
#include "machine/reg.h"

#include "vm/vm_param.h"
#include "vm/pmap.h"
#include "vm/vm_map.h"
#include "vmmeter.h"

#include "machine/trap.h"
#include "machine/dbg.h"


struct	sysent sysent[];
int	nsysent;
unsigned rcr2();
#ifdef CHKCR3
extern int chkcr3_enabled;
#endif

char	badtrap[] = "reserved type";
char	*trap_type[] = {
	 badtrap,			/* T_RESADFLT	0 */
	"illegal instruction",		/* T_PRIVINFLT	1 */
	 badtrap,			/* T_RESOPFLT	2 */
	"breakpoint instruction",	/* T_BPTFLT	3 */
	 badtrap,			/* T_UNUSED	4 */
	 badtrap,			/* T_SYSCALL	5 */
	"arithmetic trap",		/* T_ARITHTRAP	6 */
	"ast",				/* T_ASTFLT	7 */
	 badtrap,			/* T_SEGFLT	8 */
	"protection fault",		/* T_PROTFLT	9 */
	"trace trap",			/* T_TRCTRAP	10 */
	 badtrap,			/* T_UNUSED2	11 */
	"page fault",			/* T_PAGEFLT	12 */
	 badtrap,			/* T_TABLEFLT	13 */
	"alignment fault",		/* T_ALIGNFLT	14 */
	 badtrap,			/* T_KSPNOTVAL	15 */
	 badtrap,			/* T_BUSERR	16 */
	 badtrap,			/* T_KDBTRAP	17 */
	"integer divide fault",		/* T_DIVIDE	18 */
	"non-maskable intr",		/* T_NMI	19 */
	"overflow trap",		/* T_OFLOW	20 */
	"bounds check",			/* T_BOUND	21 */
	"device not available fault",	/* T_DNA	22 */
	"double fault",			/* T_DOUBLEFLT	23 */
	"fp segment overrun",		/* T_FPOPFLT	24 */
	"invalid tss fault",		/* T_TSSFLT	25 */
	"segment not present fault",	/* T_SEGNPFLT	26 */
	"stack fault",			/* T_STKFLT	27 */
	 badtrap,			/* T_RESERVED_15	28 */
	 badtrap,			/* T_RESERVED_18	29 */
	 badtrap,			/* T_RESERVED_19	30 */
	 badtrap,			/* T_RESERVED_20	31 */
	 badtrap,			/* T_RESERVED_21	32 */
	 badtrap,			/* T_RESERVED_22	33 */
	 badtrap,			/* T_RESERVED_23	34 */
	 badtrap,			/* T_RESERVED_24	35 */
	 badtrap,			/* T_RESERVED_25	36 */
	 badtrap,			/* T_RESERVED_26	37 */
	 badtrap,			/* T_RESERVED_27	38 */
	 badtrap,			/* T_RESERVED_39	39 */
	 badtrap,			/* T_RESERVED_29	40 */
	 badtrap,			/* T_RESERVED_30	41 */
	 badtrap,			/* T_RESERVED_31	42 */
};
#define	TRAP_TYPES	(sizeof trap_type / sizeof(trap_type[0]))

/*
 * With floating-point emulator configured, default
 * device-not-available trap handler is initialized in the emulator.
 * Without it, the default is to return a SIGFPE.
 * If we find a coprocessor, its attach function will take over
 * the dna trap.
 */
int (*dnatrap)  __P((struct trapframe *));

extern void iret_from_trap();
extern void lret_from_syscall();

/*
 * trap(frame):
 *	Exception, fault, and trap interface to BSD kernel. This
 * common code is called from assembly language IDT gate entry
 * routines that prepare a suitable stack frame, and restore this
 * frame after the exception has been processed. Note that the
 * effect is as if the arguments were passed call by reference.
 */

void
trap(frame)
	volatile struct trapframe frame;
{
	register int i;
	register struct proc *p = curproc;
	struct timeval syst;
	int ucode, type, code, eva;
	extern int cold;

	cnt.v_trap++;
	if (curpcb->pcb_onfault && frame.tf_trapno != 0xc) {
		frame.tf_eip = (int)curpcb->pcb_onfault;
		return;
	}
	eva = rcr2();
	type = frame.tf_trapno;
	code = frame.tf_err;
	
#include "isa.h"
#if	NISA > 0
	if (type == T_NMI) {
		/* machine/parity/power fail/"kitchen sink" faults */
		if (isa_nmi(code) == 0)
			return;
		else
			goto we_re_toast;
	}
#endif
	if (cold)
		goto we_re_toast;
#ifdef DEBUG
	if (curproc == 0 || curpcb == 0 || frame.tf_trapno == T_DOUBLEFLT)
		goto we_re_toast;
#endif

	/*
	 * Clear nested trap.  For some reason, the hardware does not
	 * protect this bit, but we never want it on anyway.  The
	 * other kernel entries are safe:  PSL_NT is cleared
	 * by the hardware on interrupts and INT instructions, and
	 * it is entirely ignored by lcall/lret.  (Though we
	 * will need to clear it in syscall if we ever unifiy the
	 * return paths, see comment near lret in locore.)
	 */
	frame.tf_eflags &= ~PSL_NT;

	syst = p->p_stime;

	/*
	 * We assume that all floating-point exceptions are the fault
	 * of the user process currently owning the coprocessor.
	 */
	if (ISPL(frame.tf_cs) == SEL_UPL || frame.tf_eflags & PSL_VM ||
	    type == T_ARITHTRAP) {
		type |= T_USER;
		p->p_regs = (int *)&frame;
		curpcb->pcb_flags |= FM_TRAP;	/* used by sendsig */
	}

	ucode = 0;
	switch (type) {

	default:
	we_re_toast:
#ifdef KDB
		if (kdb_trap(&psl))
			return;
#endif

		printf("trap type %d code = %x eip = %x cs = %x eflags = %x ",
			frame.tf_trapno, frame.tf_err, frame.tf_eip,
			frame.tf_cs, frame.tf_eflags);
		printf("cr2 %x\n", eva);
		type &= ~T_USER;
		if ((unsigned)type < TRAP_TYPES)
			panic(trap_type[type]);
		panic("trap");
		/*NOTREACHED*/

	case T_PROTFLT: 
		/* 
		 * We can get a kernel mode trap doing the return to
		 * user mode if the new user sp or ip is bad - for
		 * example, if we just loaded them to deliver a signal.
		 * We have to kill the process, since we are running a
		 * recursive invocation of trap and it is not possible
		 * to use sendsig.
		 */
		if (frame.tf_eip == (int) iret_from_trap ||
		    frame.tf_eip == (int) lret_from_syscall) {
			fatalsig(p, SIGSEGV);
			return;
		}
		goto we_re_toast;

		/* 
		 * Believe it or not, the NT bit in the flags register
		 * is not privileged, so any user can set it and try
		 * to make us do a task switch to tss.tss_link.
		 * We leave that field 0, so this causes a 
		 * user mode T_TSSFLT (in the current implementation).
		 * The manual mentions that future chips may return
		 * T_SEGNPFLT in this case, but we can handle them
		 * together anyway.
		 */

	case T_TSSFLT|T_USER:
	case T_SEGNPFLT|T_USER:
		i = SIGBUS;
		ucode = code + BUS_SEGM_FAULT;
		break;

	case T_PRIVINFLT|T_USER:	/* privileged instruction fault */
	case T_RESADFLT|T_USER:		/* reserved addressing fault */
	case T_RESOPFLT|T_USER:		/* reserved operand fault */
		ucode = type &~ T_USER;
		i = SIGILL;
		break;

	case T_ASTFLT|T_USER:		/* Allow process switch */
	case T_ASTFLT:
		astpending = 0;
		if ((p->p_flag & SOWEUPC) && p->p_stats->p_prof.pr_scale) {
			addupc(frame.tf_eip, &p->p_stats->p_prof, 1);
			p->p_flag &= ~SOWEUPC;
		}
		goto out;

	case T_DNA|T_USER:
		/*
		 * check for a transparent fault
		 * (due to context switch "late")
		 * or emulation trap
		 */
		if (dnatrap)
			i = (dnatrap)((struct trapframe *) &frame);
		else
			i = SIGFPE;
		switch (i) {
		case 0:
			curpcb->pcb_flags &= ~FM_TRAP;	/* used by sendsig */
			return;
		case SIGFPE:
			ucode = FPE_FPU_NP_TRAP;
			break;	
		case SIGSEGV:	
			ucode = T_FPOPFLT;	/* coprocessor operand fault */
			break;
		case SIGILL:	
			ucode = T_PRIVINFLT;	/* like exception 6 */
			break;
		default:
			printf("dna %d: ", i);
			panic("bad dnatrap");
		}
		break;

	case T_PROTFLT|T_USER:		/* protection fault */
	case T_STKFLT|T_USER:
		if (frame.tf_eflags & PSL_VM) {
			if ((i = emulate_vm86(&frame)) == 0) {
				curpcb->pcb_flags &= ~FM_TRAP;
				return;
			}
			break;
		}
		/* FALL THROUGH */

	case T_BOUND|T_USER:
	case T_OFLOW|T_USER:
	case T_FPOPFLT|T_USER:		/* coprocessor operand fault */
		i = SIGSEGV;
		break;

	case T_ALIGNFLT|T_USER:
		i = SIGBUS;
		break;

	case T_DIVIDE|T_USER:
		ucode = FPE_INTDIV_TRAP;
		i = SIGFPE;
		break;

	case T_ARITHTRAP|T_USER:
		/* save FPU state for a possible core dump */
		asm volatile ("fnsave %0" : "=m" (curpcb->pcb_savefpu));
		asm volatile ("frstor %0" : : "m" (curpcb->pcb_savefpu));

		ucode = code;
		i = SIGFPE;
		break;

	case T_PAGEFLT:			/* allow page faults in kernel mode */
	case T_PAGEFLT|T_USER:		/* page fault */
	    {
		register vm_offset_t va;
		register struct vmspace *vm = p->p_vmspace;
		register vm_map_t map;
		int rv;
		vm_prot_t ftype;
		extern vm_map_t kernel_map;
		unsigned nss, v, extend;

		va = trunc_page((vm_offset_t)eva);
		/*
		 * It is only a kernel address space fault iff:
		 * 	1. (type & T_USER) == 0  and
		 * 	2. pcb_onfault not set or
		 *	3. pcb_onfault set but supervisor space fault
		 * The last can occur during an exec() copyin where the
		 * argument space is lazy-allocated.
		 */
		if (type == T_PAGEFLT && va >= KERNBASE)
			map = kernel_map;
		else
			map = &vm->vm_map;
		if (code & PGEX_W)
			ftype = VM_PROT_READ | VM_PROT_WRITE;
		else
			ftype = VM_PROT_READ;

#ifdef DEBUG
		if (map == kernel_map && va == 0) {
			printf("trap: bad kernel access at %x\n", va);
			goto we_re_toast;
		}
#endif
		/*
		 * XXX: rude hack to make stack limits "work"
		 */
		if ((caddr_t)va >= vm->vm_maxsaddr && va < USRSTACK &&
		    map != kernel_map) {
			nss = roundup(USRSTACK - (unsigned)va, CLBYTES);
			if (nss > p->p_rlimit[RLIMIT_STACK].rlim_cur) {
				rv = KERN_FAILURE;
				goto nogo;
			}
			/*
			 * Here's a rude hack to cut down on stack VM.
			 * Profiling shows that we spend far too much time
			 * removing unused stack PTEs,
			 * so we try to be miserly with the address space,
			 * parcelling out stack in STACKINCR chunks.
			 * Round the amount required at the moment to
			 * a multiple of STACKINCR, but don't let the stack
			 * go below vm->vm_maxsaddr.
			 */
			if (vm->vm_ssize > 0 &&
			    va < USRSTACK - ctob(vm->vm_ssize)) {
				nss = roundup(nss, STACKINCR);
				v = MAX(USRSTACK - nss,
				    (u_long) vm->vm_maxsaddr);
				extend = (USRSTACK - v) - ctob(vm->vm_ssize);
				if (vm_allocate(&vm->vm_map, &v, extend, 0) !=
				    KERN_SUCCESS)
					goto nogo;
				vm->vm_ssize = btoc(USRSTACK - v);
			}
		}

		/* check if page table is mapped, if not, fault it first */
#define pde_v(v) (PTD[((v)>>PD_SHIFT)&1023].pd_v)
		if (!pde_v(va)) {
			v = trunc_page(vtopte(va));
			rv = vm_fault(map, v, ftype, FALSE);
			if (rv != KERN_SUCCESS)
				goto nogo;
			/* check if page table fault, increment wiring */
			vm_map_pageable(map, v, v + PAGE_SIZE, FALSE);
		} else
			v = 0;
		rv = vm_fault(map, va, ftype, FALSE);
		if (rv == KERN_SUCCESS) {
			/*
			 * Increment wiring of page table page,
			 * if not done above when creating page table page.
			 */
			if (!v && map != kernel_map) {
				v = trunc_page(vtopte(va));
				vm_map_pageable(map, v, v + PAGE_SIZE, FALSE);
			}
			if (type == T_PAGEFLT)
				return;
			goto out;
		}
nogo:
		if (type == T_PAGEFLT) {
			if (curpcb->pcb_onfault) {
				frame.tf_eip = (int)curpcb->pcb_onfault;
				return;
			}
			printf("vm_fault(%x, %x, %x, 0) -> %x\n",
			       map, va, ftype, rv);
			printf("  type %x, code %x\n",
			       type, code);
			goto we_re_toast;
		}
		i = SIGSEGV;
		break;
	    }

	case T_TRCTRAP:	 /* trace trap -- someone single stepping lcall's */
		frame.tf_eflags &= ~PSL_T;
		curpcb->pcb_flags |= PCB_TRACE_SYSCALL;
		return;
	
	case T_BPTFLT|T_USER:		/* bpt instruction fault */
	case T_TRCTRAP|T_USER:		/* trace trap */
		frame.tf_eflags &= ~PSL_T;
		i = SIGTRAP;
		break;

	}

	trapsignal(p, i, ucode);
	if ((type & T_USER) == 0)
		return;
out:
	while (i = CURSIG(p))
		psig(i);
	p->p_pri = p->p_usrpri;
#ifdef CHKCR3
	if (chkcr3_enabled)
		chkcr3(p->p_addr->u_pcb.pcb_tss.tss_cr3, p);
#endif
	if (want_resched) {
		int pl;
		/*
		 * Since we are curproc, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		pl = splclock();
		setrq(p);
		p->p_stats->p_ru.ru_nivcsw++;
		swtch();
		splx(pl);
		while (i = CURSIG(p))
			psig(i);
	}
	if (p->p_stats->p_prof.pr_scale) {
		int ticks;
		struct timeval *tv = &p->p_stime;

		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks) {
#ifdef PROFTIMER
			extern int profscale;
			addupc(frame.tf_eip, &p->p_stats->p_prof,
			    ticks * profscale);
#else
			addupc(frame.tf_eip, &p->p_stats->p_prof, ticks);
#endif
		}
	}
	curpri = p->p_pri;
	curpcb->pcb_flags &= ~FM_TRAP;	/* used by sendsig */
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
void
syscall(frame)
	volatile struct syscframe frame;
{
	register caddr_t params;
	register int i;
	register struct sysent *callp;
	register struct proc *p = curproc;
	struct timeval syst;
	int error, opc;
	int args[8], rval[2];
	u_int code;

#ifdef lint
	r0 = 0; r0 = r0; r1 = 0; r1 = r1;
#endif
	cnt.v_syscall++;
	syst = p->p_stime;
	if ((frame.sf_eflags&PSL_VM) != 0 && ISPL(frame.sf_cs) != SEL_UPL)
		panic("syscall");

	code = frame.sf_eax;
	p->p_regs = (int *)&frame;
	params = (caddr_t)frame.sf_esp + sizeof (int);

	/*
	 * Reconstruct pc, assuming lcall $X,y is 7 bytes, as it is always.
	 */
	opc = frame.sf_eip - 7;
	if (code == 0) {			/* indir */
		code = fuword(params);
		params += sizeof (int);
	}
	if (code >= nsysent)
		callp = &sysent[0];		/* indir (illegal) */
	else
		callp = &sysent[code];

	if ((i = callp->sy_narg * sizeof (int)) &&
	    (error = copyin(params, (caddr_t)args, (u_int)i))) {
		frame.sf_eax = error;
		frame.sf_eflags |= PSL_C;	/* carry bit */
#ifdef KTRACE
		if (KTRPOINT(p, KTR_SYSCALL))
			ktrsyscall(p->p_tracep, code, callp->sy_narg, &args);
#endif
		goto done;
	}
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSCALL))
		ktrsyscall(p->p_tracep, code, callp->sy_narg, &args);
#endif
	rval[0] = 0;
	rval[1] = frame.sf_edx;
	error = (*callp->sy_call)(p, args, rval);
	if (error == ERESTART)
		frame.sf_eip = opc;
	else if (error != EJUSTRETURN) {
		if (error) {
			frame.sf_eax = error;
			frame.sf_eflags |= PSL_C;	/* carry bit */
		} else {
			frame.sf_eax = rval[0];
			frame.sf_edx = rval[1];
			frame.sf_eflags &= ~PSL_C;	/* carry bit */
		}
	}
	/* else if (error == EJUSTRETURN) */
		/* nothing to do */
done:
	/*
	 * Reinitialize proc pointer `p' as it may be different
	 * if this is a child returning from fork syscall.
	 */
	p = curproc;
	while (i = CURSIG(p))
		psig(i);
	p->p_pri = p->p_usrpri;
	if (want_resched) {
		int pl;
		/*
		 * Since we are curproc, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		pl = splclock();
		setrq(p);
		p->p_stats->p_ru.ru_nivcsw++;
		swtch();
		splx(pl);
		while (i = CURSIG(p))
			psig(i);
	}
	if (p->p_stats->p_prof.pr_scale) {
		int ticks;
		struct timeval *tv = &p->p_stime;

		ticks = ((tv->tv_sec - syst.tv_sec) * 1000 +
			(tv->tv_usec - syst.tv_usec) / 1000) / (tick / 1000);
		if (ticks) {
#ifdef PROFTIMER
			extern int profscale;
			addupc(opc, &p->p_stats->p_prof, ticks * profscale);
#else
			addupc(opc, &p->p_stats->p_prof, ticks);
#endif
		}
	}

	if (curpcb->pcb_flags & PCB_TRACE_SYSCALL) {
		curpcb->pcb_flags &= ~PCB_TRACE_SYSCALL;
		frame.sf_eflags |= PSL_T;
	}

	curpri = p->p_pri;
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p->p_tracep, code, error, rval[0]);
#endif
}

struct connect_area {
	int	int_state;
	int	need_interrupt;
	int	disk_connect_area;
};

#define	CLI	0xfa
#define	STI	0xfb
#define	PUSHF	0x9c
#define	POPF	0x9d
#define	INTn	0xcd
#define	IRET	0xcf

inline static int
MAKE_ADDR(int sel, int off)
{

	return ((sel << 4) + off);
}

inline static void
PUSH(u_short x, struct trapframe_vm86 *frame)
{

	frame->tf_sp -= 2;
	susword((void *) MAKE_ADDR(frame->tf_ss, frame->tf_sp), x);
}

inline static u_short
POP(struct trapframe_vm86 *frame)
{
	u_short x = fusword((void *) MAKE_ADDR(frame->tf_ss, frame->tf_sp));

	frame->tf_sp += 2;
	return (x);
}

emulate_vm86(tf)
	struct trapframe_vm86 *tf;
{
	int intnum, addr;
	int int_state, old_int_state;
	struct proc *p = curproc;
	struct connect_area *connect_area;

	if (p->p_md.md_connarea == (void *) -1)
		return (SIGBUS);
	connect_area = (struct connect_area *)p->p_md.md_connarea;
	old_int_state = int_state = fusword((void *)&connect_area->int_state);
	addr = MAKE_ADDR(tf->tf_cs, tf->tf_ip);

	switch (fubyte((void *)addr)) {
	case CLI:
		int_state = 0;
		tf->tf_ip++;
		break;
	case STI:
		int_state = PSL_I;
		tf->tf_ip++;
		break;
	case PUSHF:
		PUSH((tf->tf_eflags86 & ~PSL_I) | int_state | PSL_IOPL, tf);
		tf->tf_ip++;
		break;
	case POPF:
		int_state = POP(tf);
		tf->tf_eflags86 = (int_state & ~PSL_IOPL) | PSL_I;
		int_state &= PSL_I;
		tf->tf_ip++;
		break;
	case INTn:
		intnum = fubyte((void *)(addr + 1));
		PUSH((tf->tf_eflags86 & ~PSL_I) | int_state | PSL_IOPL, tf);
		PUSH(tf->tf_cs, tf);
		PUSH(tf->tf_ip + 2, tf);
		int_state = 0;
		tf->tf_eflags86 &= ~PSL_T;
		tf->tf_cs = fusword((void *)(intnum * 4 + 2));
		tf->tf_ip = fusword((void *)(intnum * 4));
		break;
	case IRET:
		tf->tf_ip = POP(tf);
		tf->tf_cs = POP(tf);
		int_state = POP(tf);
		tf->tf_eflags86 = (int_state & ~PSL_IOPL) | PSL_I;
		int_state &= PSL_I;
		break;
	default:
		return (SIGBUS);
	}
	if (old_int_state != int_state)
		susword((void *)&connect_area->int_state, int_state);
	if (old_int_state == 0 && int_state == PSL_I &&
	    fusword((void *)&connect_area->need_interrupt))
		return (SIGIO);
	else
		return (0);
}
