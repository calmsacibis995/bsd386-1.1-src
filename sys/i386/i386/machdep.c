/*-
 * Copyright (c) 1992, 1993, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: machdep.c,v 1.46 1994/01/30 04:31:37 karels Exp $
 */

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
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
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include "param.h"
#include "systm.h"
#include "signalvar.h"
#include "kernel.h"
#include "map.h"
#include "proc.h"
#include "user.h"
#include "buf.h"
#include "reboot.h"
#include "conf.h"
#include "file.h"
#include "callout.h"
#include "malloc.h"
#include "mbuf.h"
#include "msgbuf.h"
#include "exec.h"
#include "sysinfo.h"
#include "net/netisr.h"

#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"

vm_map_t buffer_map;
extern vm_offset_t avail_end;

#include "machine/cpu.h"
#include "machine/reg.h"
#include "machine/psl.h"
#include "specialreg.h"
#include "fp_reg.h"
#include "../isa/isa.h"
#include "../isa/timerreg.h"
#include "../isa/rtc.h"

#ifdef SYSVSHM
#include "shm.h"
#endif

struct proc *npxproc;

/*
 * Declare this as initialized data so we can patch it.
 */
int	nswbuf = 0;

int	msgbufmapped;		/* set when safe to use msgbuf */

/*
 * Machine-dependent startup code
 */
int	boothowto = 0;
long	dumplo;
int	physmem, maxmem;
extern	int bootdev;

int cpuspeed = 10;		/* default speed, will try to guess real */
extern int cpu;			/* cpu type */
extern struct sysinfo sysinfo;
extern	char sysinfostrings[], machine[], sys_model[], ostype[], osrelease[];
extern	int sys_modelbufsize;
extern	int sys_modelbufsize;
extern	int maxbufmem;
extern	int nmbclusters;

cpu_startup(firstaddr)
	int firstaddr;
{
	register unsigned i;
	register caddr_t v;
	vm_offset_t minaddr, maxaddr;
	vm_size_t size;

	/*
	 * Initialize error message buffer (at end of core).
	 */
	/* avail_end was pre-decremented in pmap_bootstrap to compensate */
	for (i = 0; i < btoc(sizeof (struct msgbuf)); i++)
		pmap_enter(pmap_kernel(), msgbufp + i * NBPG,
		    avail_end + i * NBPG, VM_PROT_ALL, TRUE);
	msgbufmapped = 1;

#ifdef KDB
	kdb_init();			/* startup kernel debugger */
#endif
	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
	printf("real mem = %d\n", ctob(maxmem));

	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is in "firstaddr".
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * As pages of memory are allocated and cleared,
	 * "firstaddr" is incremented.
	 */

	/*
	 * Make two passes.  The first pass calculates how much memory is
	 * needed and allocates it.  The second pass assigns virtual
	 * addresses to the various data structures.
	 */
	firstaddr = 0;
again:
	v = (caddr_t)firstaddr;

#define	valloc(name, type, num) \
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = maxproc * 10);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
	if (nswbuf == 0) {
		/*
		 * Allocate enough swap buffers so that 5% of memory
		 * could be paged at once, with an average transfer size
		 * of MAXBSIZE.  Force the number to be even for historical
		 * reasons.
		 */
		nswbuf = (physmem / (20 * MAXBSIZE / NBPG)) &~ 1;
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
		else if (nswbuf < 50)
			nswbuf = 50;		/* more sanity */
	}
	valloc(swbuf, struct buf, nswbuf);

	/*
	 * End of first pass, size has been calculated so allocate memory
	 */
	if (firstaddr == 0) {
		size = (vm_size_t) v;
		firstaddr = (int)kmem_alloc(kernel_map, round_page(size));
		if (firstaddr == 0)
			panic("startup: no room for tables");
		goto again;
	}
	/*
	 * End of second pass, addresses have been assigned
	 */
	if ((vm_size_t)(v - firstaddr) != size)
		panic("startup: table size inconsistency");
#if 0	/* we don't use these currently */
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, TRUE);
	/*
	 * Allocate a submap for physio
	 */
	phys_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, TRUE);
#endif

	/*
	 * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
	 * we use the more space efficient malloc in place of kmem_alloc.
	 */
	mclrefcnt = (char *)malloc(nmbclusters + CLBYTES / MCLBYTES,
	    M_MBUF, M_NOWAIT);
	bzero(mclrefcnt, nmbclusters + CLBYTES / MCLBYTES);
	mb_map = kmem_suballoc(kernel_map, (vm_offset_t)&mbutl, &maxaddr,
	    nmbclusters * MCLBYTES, FALSE);
	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	printf("avail mem = %d\n", ptoa(vm_page_free_count));
	printf("buffer cache = %d\n", maxbufmem);

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Configure the system.
	 */
	configure();

	/* set remaining system parameters */
	sysinfo.sys_hz = hz;
	if (phz)
		sysinfo.sys_phz = phz;
	else
		sysinfo.sys_phz = hz;

}

#ifdef PGINPROF
/*
 * Return the difference (in microseconds)
 * between the  current time and a previous
 * time as represented  by the arguments.
 * If there is a pending clock interrupt
 * which has not been serviced due to high
 * ipl, return error code.
 */
/*ARGSUSED*/
vmtime(otime, olbolt, oicr)
	register int otime, olbolt, oicr;
{

	return (((time.tv_sec-otime)*60 + lbolt-olbolt)*16667);
}
#endif

struct sigframe {
	int	sf_signum;
	int	sf_code;
	struct	sigcontext *sf_scp;
	sig_t	sf_handler;
	int	sf_eax;	
	int	sf_edx;	
	int	sf_ecx;	
	struct	save87 sf_fpu;
	struct	sigcontext sf_sc;
} ;

#ifndef NO_KSTACK
extern int kstack[];
#endif

/*
 * Send an interrupt to process.
 *
 * Stack is set up to allow sigcode stored
 * in u. to call routine, followed by kcall
 * to sigreturn routine below.  After sigreturn
 * resets the signal mask, the stack, and the
 * frame pointer, it returns to the user
 * specified pc, psl.
 */
void
sendsig(catcher, sig, mask, code)
	sig_t catcher;
	int sig, mask;
	unsigned code;
{
	register struct proc *p = curproc;
	register int *regs;
	register struct sigframe f, *fp;
	struct sigacts *ps = p->p_sigacts;
	int oonstack, frmtrap;
	int frmvm86, userframesize;
#ifdef COPY_SIGCODE
	extern int szsigcode;
#endif

	regs = p->p_regs;
        oonstack = ps->ps_onstack;
	frmtrap = curpcb->pcb_flags & FM_TRAP;
	if (frmtrap && (regs[tEFLAGS] & PSL_VM)) {
		frmvm86 = 1;
		userframesize = sizeof(struct sigframe) +
		    sizeof(struct trapframe_vm86);
	} else {
		frmvm86 = 0;
		userframesize = sizeof(struct sigframe);
        }
	/*
	 * Allocate and validate space for the signal handler
	 * context. Note that if the stack is in data space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
        if (!ps->ps_onstack && (ps->ps_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)(ps->ps_sigsp - userframesize);
                ps->ps_onstack = 1;
	} else if (frmvm86) {		/* XXX why check? */
		fatalsig(p, SIGILL);
		return;
	} else {
		if (frmtrap)
			fp = (struct sigframe *)(regs[tESP] -
			    sizeof(struct sigframe));
		else
			fp = (struct sigframe *)(regs[sESP] -
			    sizeof(struct sigframe));
	}

 	if (!grow(p, (unsigned)fp) ||
 	    !useracc((caddr_t)fp, userframesize, B_WRITE)) {
  		/*
  		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		fatalsig(p, SIGILL);
		return;
	}

        /*
         * Copy trap frame on top of the user stack.
         */
	if (frmvm86)
		copyout(regs, (void *)((unsigned)fp + sizeof(struct sigframe)),
		    sizeof(struct trapframe_vm86));

	/* 
	 * Build the argument list for the signal handler.
	 */
	f.sf_signum = sig;
	f.sf_code = code;
	f.sf_scp = &fp->sf_sc;
	f.sf_handler = catcher;

	/* save scratch registers */
	if (frmtrap) {
		f.sf_eax = regs[tEAX];
		f.sf_edx = regs[tEDX];
		f.sf_ecx = regs[tECX];
	} else {
		f.sf_eax = regs[sEAX];
		f.sf_edx = regs[sEDX];
		f.sf_ecx = regs[sECX];
	}

	/*
	 * Save floating point state on the user stack.
	 * We also restore the default control word.
	 * XXX This saves users who got nailed in the middle
	 * of floating-to-int operations, but may screw users
	 * who intelligently manipulate the control word...
	 */
	if ((rcr0() & CR0_EM) == 0 && npxproc) {
		/* CR0_EM is off only if we have hardware floating point */
		asm volatile ("fnsave %0" : "=m" (curpcb->pcb_savefpu));
		npxinit(curproc, FPC_DEFAULT);
	}
	f.sf_fpu = curpcb->pcb_savefpu;
	curpcb->pcb_savefpu.sv_env.en_cw = FPC_DEFAULT;
	curpcb->pcb_savefpu.sv_env.en_sw = 0;
	curpcb->pcb_savefpu.sv_env.en_tw = -1;

	/*
	 * Build the signal context to be used by sigreturn.
	 */
	f.sf_sc.sc_onstack = oonstack;
	f.sf_sc.sc_mask = mask;
	if (frmtrap) {
		f.sf_sc.sc_sp = regs[tESP];
		f.sf_sc.sc_fp = regs[tEBP];
		f.sf_sc.sc_pc = regs[tEIP];
		f.sf_sc.sc_ps = regs[tEFLAGS];
		regs[tESP] = (int)fp;
		regs[tEIP] = PS_STRINGS - roundup(szsigcode, sizeof (double));
		if (frmvm86) {
			extern int _ucodesel, _udatasel;

			regs[tEFLAGS] = PSL_USERSET;
			regs[tCS] = _ucodesel;
			regs[tDS] = _udatasel;
			regs[tES] = _udatasel;
			regs[tSS] = _udatasel;
		}
	} else {
		f.sf_sc.sc_sp = regs[sESP];
		f.sf_sc.sc_fp = regs[sEBP];
		f.sf_sc.sc_pc = regs[sEIP];
		f.sf_sc.sc_ps = regs[sEFLAGS];
		regs[sESP] = (int)fp;
		regs[sEIP] = PS_STRINGS - roundup(szsigcode, sizeof (double));
	}

	copyout(&f, fp, sizeof f);
}

/*
 * System call to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * stack state from context left by sendsig (above).
 * Return to previous pc and psl as specified by
 * context left by sendsig. Check carefully to
 * make sure that the user has not modified the
 * psl to gain improper priviledges or to cause
 * a machine fault.
 *
 * Ugly hack: we don't bother to use copyin() to
 * recover the sigframe because we won't take a
 * write fault on the stack (avoiding 386 COW problems).
 */
/* ARGSUSED */
sigreturn(p, uap, retval)
	struct proc *p;
	struct args {
		struct sigcontext *sigcntxp;
	} *uap;
	int *retval;
{
	register struct sigcontext *scp;
	register struct sigframe *fp;
	register int *regs = p->p_regs;

	fp = (struct sigframe *) regs[sESP];
	if (useracc((caddr_t)fp, sizeof (*fp), 0) == 0)
		return (EINVAL);
	if (fp->sf_sc.sc_ps & PSL_VM && useracc((caddr_t)fp + sizeof (*fp),
	    sizeof(struct trapframe_vm86), 0) == 0)
		return (EINVAL);

	if (fp->sf_sc.sc_ps & PSL_VM) {
		/*
		 * If the signal context claims we are going back to v86
		 * mode, make sure the flags register that the processor
		 * will see agrees.  Otherwise, we'll get a general 
		 * protection trap when trying to load trash into the
		 * cs at the iret in swicth_to_vm86.
		 */
		struct trapframe_vm86 *tfvm86;
		u_int *flagsp;

		tfvm86 = (struct trapframe_vm86 *)((caddr_t)fp + sizeof (*fp));
		flagsp = (u_int *)&tfvm86->tf_eflags86;

		if ((*flagsp & PSL_VM) == 0)
			return (EINVAL);

		*flagsp = (*flagsp & ~PSL_USERCLR) | PSL_USERSET | PSL_VM;
		if ((p->p_md.md_flags & MDP_IOPL) == 0)
			*flagsp &= ~PSL_IOPL;
	}

	/* restore scratch registers */
	regs[sEAX] = fp->sf_eax;
	regs[sEDX] = fp->sf_edx;
	regs[sECX] = fp->sf_ecx;

	/*
	 * Restore floating point state.
	 */
	curpcb->pcb_savefpu = fp->sf_fpu;
	if ((rcr0() & CR0_EM) == 0 && npxproc)
		/* XXX does this cause an exception if one was pending? */
		asm volatile ("frstor %0" : : "m" (curpcb->pcb_savefpu));

	scp = fp->sf_scp;
	if (useracc((caddr_t)scp, sizeof (*scp), 0) == 0)
		return (EINVAL);

        p->p_sigacts->ps_onstack = scp->sc_onstack & 01;
	p->p_sigmask = scp->sc_mask &~ sigcantmask;
	if (fp->sf_sc.sc_ps & PSL_VM) {
		if (p->p_md.md_connarea == 0)
		  	p->p_md.md_connarea = (void *) fp->sf_eax;
		switch_to_vm86((caddr_t)fp + sizeof (*fp),
#ifndef NO_KSTACK
		       (unsigned int) kstack + UPAGES * NBPG -
#else
		       (unsigned int) p->p_addr + UPAGES * NBPG -
#endif
		       sizeof(struct trapframe_vm86),
		       sizeof(struct trapframe_vm86));
		/* NOTREACHED */
	}

	regs[sEBP] = scp->sc_fp;
	regs[sESP] = scp->sc_sp;
	regs[sEIP] = scp->sc_pc;
	regs[sEFLAGS] = (fp->sf_sc.sc_ps & ~PSL_USERCLR) | PSL_USERSET;
	if ((p->p_md.md_flags & MDP_IOPL) == 0)
		regs[sEFLAGS] &= ~PSL_IOPL;
	return (EJUSTRETURN);
}

int	waittime = -1;

boot(arghowto)
	int arghowto;
{
	register long dummy;		/* XXX r12 is reserved */
	register int howto;		/* XXX r11 == how to boot */
	register int devtype;		/* XXX r10 == major of root dev */
	extern int cold;
	extern char *panicstr;
	extern long numbufcache;
	extern struct proc *prevproc;
	static int atshutdowndone;

	/* take a snap shot before clobbering any registers */
	if (curproc)
		savectx(curproc->p_addr, 0);

	howto = arghowto;
	if ((howto&RB_NOSYNC) == 0 && waittime < 0 && bhhead) {
		register struct buf *bp;
		int iter, nbusy;

		waittime = 0;
		(void) splnet();
		printf("syncing disks... ");
		if (curproc == NULL) {
			curproc = prevproc;
			curpcb = &curproc->p_addr->u_pcb;
		}
		cold = 1;	/* at least chilly, can't sleep normally */
		/*
		 * Release inodes held by texts before update.
		 */
		if (panicstr == 0)
			vnode_pager_umount(NULL);
		sync((struct proc *)NULL, (void *)NULL, (int *)NULL);

		for (iter = 0; iter < 20; iter++) {
		restart:
			nbusy = numbufcache;
			for (bp = bhhead; bp; bp = bp->b_next) {
				int s = splbio();

				/* avoid races */
				if (bp->b_flags & B_BUSY) {
					splx(s);
					goto restart;
				}

				/* shouldn't happen? */
				if (bp->b_flags & B_DELWRI) {
					bremfree(bp);
					bp->b_flags |= B_BUSY;
					splx(s);
					printf("<d>");	/* XXX */
					bawrite(bp);
					goto restart;
				}
				splx(s);
				--nbusy;
			}
			if (nbusy <= 0)
				break;
			printf("%d ", nbusy);
			DELAY(40000 * iter);
		}
		if (nbusy > 0)
			printf("giving up\n");
		else
			printf("done\n");
		DELAY(10000);			/* wait for printf to finish */
	}
	splhigh();
	devtype = major(rootdev);
	if (howto & RB_DUMP)
		dumpsys();	
	/* now do other shutdown activity, including devices */
	if (atshutdowndone++ == 0)
		doatshutdown();
	if (howto&RB_HALT) {
		printf("\nSystem is halted; %s\n\n",
		    "hit reset, turn power off, or press return to reboot");
		splx(0xfffd);	/* all but keyboard XXX */
		while (getchar() != '\n')
			;
	}
#ifdef lint
	printf("%d %d %d\n", dummy, arghowto, devtype);
#endif
	reset_cpu();
	for (;;)
		;
	/*NOTREACHED*/
}

int	dumpmag = 0x8fca0101;	/* magic number for savecore */
int	dumpsize = 0;		/* also for savecore */
/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
dumpsys()
{

	if (dumpdev == NODEV) {
		/* pause about 15 sec. so panic message can be read */
		DELAY(15*1000000);
		return;
	}
	dumpsize = maxmem;
	printf("\ndumping to dev %x, offset %d\n", dumpdev, dumplo);
	printf("dump ");
	switch ((*bdevsw[major(dumpdev)].d_dump)(dumpdev)) {

	case 0:
		printf("succeeded\n");
		/* pause about 5 sec. more so panic message can be read */
		DELAY(5*1000000);
		return;
		
	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	default:
		printf("unknown error\n");
		break;
	}
	printf("\n\n");
	/* pause about 15 sec. so panic message can be read */
	DELAY(15*1000000);
}

#ifdef HZ
/*
 * microtime function for non-default values of HZ;
 * locore.s contains a more accurate function for the 
 * default value.
 */
microtime(tvp)
	register struct timeval *tvp;
{
	register int u, s;
	register volatile struct timeval *p = &time;
	static struct timeval lasttime;

	do {
		s = p->tv_sec;
		u = p->tv_usec;
	} while (u != p->tv_usec || s != p->tv_sec);
	if (u == lasttime.tv_usec && s == lasttime.tv_sec && ++u >= 1000000) {
		u -= 1000000;
		s++;
	}
	lasttime.tv_sec = s;
	lasttime.tv_usec = u;
	tvp->tv_sec = s;
	tvp->tv_usec = u;
}
#endif

initcpu()
{
}


/*
 * Initialize 386 and configure to run kernel
 */

/*
 * Initialize segments & interrupt table
 */

union descriptor gdt[NGDT];

/* interrupt descriptor table */
struct gate_descriptor idt[32+16];

/* local descriptor table */
union descriptor ldt[NLDT];

struct	i386tss	tss, panic_tss;
char emergency_stack[512];

extern  struct user *proc0paddr;

/* software prototypes -- in more palatable form */
struct soft_segment_descriptor gdt_segs[] = {
	/* Null Descriptor */
{	0x0,			/* segment base address  */
	0x0,			/* length - all address space */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0,0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Code Descriptor for kernel */
{	0x0,			/* segment base address  */
	0xfffff,		/* length - all address space */
	SDT_MEMERA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0,0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
	/* Data Descriptor for kernel */
{	0x0,			/* segment base address  */
	0xfffff,		/* length - all address space */
	SDT_MEMRWA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0,0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
	/* LDT Descriptor */
{	(int) ldt,			/* segment base address  */
	sizeof(ldt)-1,		/* length - all address space */
	SDT_SYSLDT,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0,0,
	0,			/* unused - default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Null Descriptor - Placeholder */
{	0x0,			/* segment base address  */
	0x0,			/* length - all address space */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0,0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Panic Tss Descriptor */
{	(int) &panic_tss,		/* segment base address  */
	sizeof(tss)-1,		/* length - all address space */
	SDT_SYS386TSS,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0,0,
	0,			/* unused - default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Stack Descriptor for kernel */
{	0x0,			/* segment base address  */
	((unsigned) (USRSTACK+NBPG) >> PGSHIFT)-1,/* set red zone in kstack */
	SDT_MEMRWDA,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0,0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
	/* Proc 0 Tss Descriptor */
{	(int) &tss,			/* segment base address  */
	sizeof(tss)-1,		/* length - all address space */
	SDT_SYS386TSS,		/* segment type */
	0,			/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0,0,
	0,			/* unused - default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ }};

struct soft_segment_descriptor ldt_segs[] = {
	/* Null Descriptor - overwritten by call gate */
{	0x0,			/* segment base address  */
	0x0,			/* length - all address space */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0,0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Null Descriptor - overwritten by call gate */
{	0x0,			/* segment base address  */
	0x0,			/* length - all address space */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0,0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Null Descriptor - overwritten by call gate */
{	0x0,			/* segment base address  */
	0x0,			/* length - all address space */
	0,			/* segment type */
	0,			/* segment descriptor priority level */
	0,			/* segment descriptor present */
	0,0,
	0,			/* default 32 vs 16 bit size */
	0  			/* limit granularity (byte/page units)*/ },
	/* Code Descriptor for user */
{	0x0,			/* segment base address  */
	btoc(VM_MAXUSER_ADDRESS)-1,/* length - all of user space */
	SDT_MEMERA,		/* segment type */
	SEL_UPL,		/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0,0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ },
	/* Data Descriptor for user */
{	0x0,			/* segment base address  */
	btoc(VM_MAXUSER_ADDRESS)-1,/* length - all of user space */
	SDT_MEMRWA,		/* segment type */
	SEL_UPL,		/* segment descriptor priority level */
	1,			/* segment descriptor present */
	0,0,
	1,			/* default 32 vs 16 bit size */
	1  			/* limit granularity (byte/page units)*/ } };

/* table descriptors - used to load tables by microp */
struct region_descriptor r_gdt = {
	sizeof(gdt)-1,(char *)gdt
};

struct region_descriptor r_idt = {
	sizeof(idt)-1,(char *)idt
};

setidt(idx, func, typ, dpl, sel)
	char *func;
{
	struct gate_descriptor *ip = idt + idx;

	ip->gd_looffset = (int)func;
	ip->gd_selector = sel;
	ip->gd_stkcpy = 0;
	ip->gd_xx = 0;
	ip->gd_type = typ;
	ip->gd_dpl = dpl;
	ip->gd_p = 1;
	ip->gd_hioffset = ((int)func)>>16 ;
}

#define	IDTVEC(name)	__CONCAT(X, name)
extern	IDTVEC(div), IDTVEC(dbg), IDTVEC(nmi), IDTVEC(bpt), IDTVEC(ofl),
	IDTVEC(bnd), IDTVEC(ill), IDTVEC(dna), IDTVEC(dble), IDTVEC(fpusegm),
	IDTVEC(tss), IDTVEC(missing), IDTVEC(stk), IDTVEC(prot),
	IDTVEC(page), IDTVEC(rsvd), IDTVEC(fpu), IDTVEC(align),
	IDTVEC(rsvd1), IDTVEC(rsvd2), IDTVEC(rsvd3), IDTVEC(rsvd4),
	IDTVEC(rsvd5), IDTVEC(rsvd6), IDTVEC(rsvd7), IDTVEC(rsvd8),
	IDTVEC(rsvd9), IDTVEC(rsvd10), IDTVEC(rsvd11), IDTVEC(rsvd12),
	IDTVEC(rsvd13), IDTVEC(rsvd14), IDTVEC(rsvd14), IDTVEC(syscall);

int lcr0(), lcr3(), rcr0(), rcr2();
int _udatasel, _ucodesel, _gsel_tss;

double_fault_handler()
{

	printf("double fault: eip=%x esp=%x\n", tss.tss_eip, tss.tss_esp);
	panic("double fault");
}

struct biosinfo *biosinfo;

checkbootparams(bhp)
	struct bootparamhdr *bhp;
{
	struct bootparam *bp;
	extern int autodebug;

	if (bhp->b_magic == BOOT_MAGIC) {
		for (bp = B_FIRSTPARAM(bhp); bp; bp = B_NEXTPARAM(bhp, bp))
		    	switch (bp->b_type) {
			case B_BIOSGEOM:
				setbiosgeom((struct biosgeom *) B_DATA(bp));
				break;
			case B_BIOSINFO:
				biosinfo = (struct biosinfo *) B_DATA(bp);
				break;
#ifdef BSDGEOM
			case B_BSDGEOM:
				setbsdgeom((struct bsdgeom *) B_DATA(bp));
				break;
#endif
			case B_AUTODEBUG:
				autodebug = *(int *) B_DATA(bp);
				break;
			default:
				break;
		}
	}
}

init386(first)
{
	extern ssdtosd(), lgdt(), lidt(), lldt(), etext; 
	int x, i;
	int sel;
	unsigned biosbasemem, biosextmem;
	struct gate_descriptor *gdp;
#ifndef COPY_SIGCODE
	extern int sigcode,szsigcode;
#endif

	proc0.p_addr = proc0paddr;

	/* make gdt memory segments */
	gdt_segs[GCODE_SEL].ssd_limit = btoc((int) &etext + NBPG);
	for (x=0; x < NGDT; x++) ssdtosd(gdt_segs+x, gdt+x);
	/* Note. eventually want private ldts per process */
	for (x=0; x < 5; x++) ssdtosd(ldt_segs+x, ldt+x);

	/* exceptions */
	sel = GSEL(GCODE_SEL, SEL_KPL);
	setidt(0, &IDTVEC(div),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(1, &IDTVEC(dbg),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(2, &IDTVEC(nmi),  SDT_SYS386TGT, SEL_KPL, sel);
 	setidt(3, &IDTVEC(bpt),  SDT_SYS386TGT, SEL_UPL, sel);
	setidt(4, &IDTVEC(ofl),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(5, &IDTVEC(bnd),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(6, &IDTVEC(ill),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(7, &IDTVEC(dna),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(9, &IDTVEC(fpusegm),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(10, &IDTVEC(tss), SDT_SYS386TGT, SEL_KPL, sel);
	setidt(11, &IDTVEC(missing),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(12, &IDTVEC(stk), SDT_SYS386TGT, SEL_KPL, sel);
	setidt(13, &IDTVEC(prot),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(14, &IDTVEC(page),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(15, &IDTVEC(rsvd),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(16, &IDTVEC(fpu),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(17, &IDTVEC(align),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(18, &IDTVEC(rsvd1),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(19, &IDTVEC(rsvd2),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(20, &IDTVEC(rsvd3),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(21, &IDTVEC(rsvd4),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(22, &IDTVEC(rsvd5),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(23, &IDTVEC(rsvd6),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(24, &IDTVEC(rsvd7),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(25, &IDTVEC(rsvd8),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(26, &IDTVEC(rsvd9),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(27, &IDTVEC(rsvd10),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(28, &IDTVEC(rsvd11),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(29, &IDTVEC(rsvd12),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(30, &IDTVEC(rsvd13),  SDT_SYS386TGT, SEL_KPL, sel);
	setidt(31, &IDTVEC(rsvd14),  SDT_SYS386TGT, SEL_KPL, sel);

	sel = GSEL(GPANIC_SEL, SEL_KPL);
	setidt(8, 0,  SDT_SYSTASKGT, SEL_KPL, sel);	/* double fault */

#include	"isa.h"
#if	NISA >0
	(void) isa_probe();
	/*
	 * Set up ICUs in autoconfig mode.
	 * Note that this does not yet enable interrupts,
	 * as we are not yet prepared for them.
	 */
	isa_autoirq();	
#endif

	lgdt(gdt, sizeof(gdt)-1);
	lidt(idt, sizeof(idt)-1);
	lldt(GSEL(GLDT_SEL, SEL_KPL));

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#define	kbtopg(k)	((k) / (NBPG / 1024))
#define	pgtokb(pg)	((pg) * (NBPG / 1024))
#define BASE_END	640		/* end of base memory, in KB */
#define EXT_BASE	1024		/* start of extended memory, in KB */

	printf("RAM (KB): found %d ", pgtokb(maxmem));

	/* reconcile against BIOS's recorded values in RTC
	 * we trust neither of them, as both can lie!
	 */
	biosbasemem = rtcin(RTC_BASELO)+ (rtcin(RTC_BASEHI)<<8);
	biosextmem = rtcin(RTC_EXTLO)+ (rtcin(RTC_EXTHI)<<8);
	printf("bios %d + %d\n", biosbasemem, biosextmem);
	/* Phoenix BIOS saves 1K, HP Vectra saves 4K; ignore the difference */
	if (biosbasemem >= BASE_END - 4)
		biosbasemem = BASE_END;
#ifdef nomore
	if (biosbasemem == 0xffff || biosextmem == 0xffff) {
		if (maxmem > 0xffc) 	/* ??? */
			maxmem = kbtopg(BASE_END);
	} else if (biosextmem > 0 && biosbasemem == BASE_END) {
		int totbios = kbtopg(EXT_BASE + biosextmem);

		if (totbios < maxmem)
			maxmem = totbios;
	} else
		maxmem = kbtopg(biosbasemem);
#endif
	physmem = maxmem;
	if (maxmem > kbtopg(EXT_BASE))
		physmem -= kbtopg((EXT_BASE - BASE_END));

	/* call pmap initialization to make new kernel address space */
	pmap_bootstrap(first, 0);
	/* now running on new page tables, configured,and u/iom is accessible */

	/* make a initial tss so microp can get interrupt stack on syscall! */
	tss.tss_esp = tss.tss_esp0 = tss.tss_esp1 = tss.tss_esp2 =
		(int) &emergency_stack[sizeof emergency_stack - 24];
	tss.tss_ss = tss.tss_ss0 = tss.tss_ss1 = tss.tss_ss2 =
		GSEL(GSTACK_SEL, SEL_KPL);
	tss.tss_cr3 = IdlePTD;
	tss.tss_eip = (int) double_fault_handler;
	/* interrupts off for panic_tss; eflags not used in main tss */
	tss.tss_eflags = PSL_MBO;
	/* most user registers remain 0 */
	tss.tss_ds = tss.tss_es = tss.tss_fs = tss.tss_gs =
		GSEL(GDATA_SEL, SEL_KPL);
	tss.tss_cs = GSEL(GCODE_SEL, SEL_KPL);
	tss.tss_ldt = GSEL(GLDT_SEL, SEL_KPL);
	panic_tss = tss;
#ifndef NO_KSTACK
	tss.tss_esp0 = (int) kstack + UPAGES*NBPG;	/* XXX */
#endif

	/*
	 * By default, deny user processes access to I/O ports.
	 * However, for the moment, allow access to the VGA ports
	 * (ports 0x3B0 thru 0x3DF) for backward compatibility.
	 */
	tss.tss_ioopt |= ((u_int)&tss.tss_iomap - (u_int)&tss.tss_link) << 16;
	for (i = 0; i < USER_IOPORTS / NBBY; i++)
		if (i < 0x3b0 / NBBY || i > 0x3e0 / NBBY)
			tss.tss_iomap[i] = 0xff; /* disallow these ports */
		else
			tss.tss_iomap[i] = 0x0;  /* allow these ports */

	_gsel_tss = GSEL(GPROC0_SEL, SEL_KPL);
	ltr(_gsel_tss);

	/* make a call gate with which to reenter kernel */
	gdp = &ldt[L43BSDCALLS_SEL].gd;

	x = (int) &IDTVEC(syscall);
	gdp->gd_looffset = x;
	gdp->gd_selector = GSEL(GCODE_SEL,SEL_KPL);
	gdp->gd_stkcpy = 0;
	gdp->gd_type = SDT_SYS386CGT;
	gdp->gd_dpl = SEL_UPL;
	gdp->gd_p = 1;
	gdp->gd_hioffset = x >> 16;

	/* copy down to LDT 0 (LDEFCALLS_SEL), the default */
	ldt[LDEFCALLS_SEL] = ldt[L43BSDCALLS_SEL];

	/* transfer to user mode */

	_ucodesel = LSEL(LUCODE_SEL, SEL_UPL);
	_udatasel = LSEL(LUDATA_SEL, SEL_UPL);

	/* setup proc 0's pcb */
	proc0.p_addr->u_pcb.pcb_flags = 0;
	proc0.p_addr->u_pcb.pcb_ptd = IdlePTD;
	curpcb = &proc0.p_addr->u_pcb;		/* so we can take traps */

	/* save a copy of the default syscall descriptor in the pcb */
	curpcb->pcb_ldtdefcall = ldt[LDEFCALLS_SEL];

	/*
	 * Enable features that aren't available on the 386.
	 * We turn on the write protect bit so that we can prevent
	 * modification of kernel text and also do COW more efficiently,
	 * and we turn on the numeric error bit so that we don't need
	 * an external interrupt to report an FP error.
	 * Disable coprocessor unless/until we find one.
	 */
	i = rcr0() | CR0_EM;	/* start emulating */
	if (cpu > CPU_386)
		i |= CR0_WP;
	lcr0(i);

	/*
	 * Now that we're prepared to take interrupts, enable them.
	 * Note that all IRQs are masked, so the only interrupts
	 * which can arrive now are 'spurious' interrupts on IRQ7 & IRQ15.
	 */
	asm volatile ("sti");
}

/*
 * Main calls consinit to locate the console,
 * but we have already done the work in init386 (calling cninit).
 */
consinit()
{
}

#ifdef CHKCR3
#include "../isa/isa.h"

u_long
get_pmap_pte(pmap, va)
	struct pmap *pmap;
	u_long va;
{
	extern struct pte *pmap_pte();
	struct pte *ptep = pmap_pte(pmap, va);

	if (ptep)
		return *(u_long *)ptep;
	return 0;	/* valid bit clear */
}

int chkcr3_enabled = 0;

chkcr3(newcr3, p)
	unsigned long newcr3;
	struct proc *p;
{
        extern int *vaPTD;
	extern char end, etext, edata;
	char *cp, val;
	u_long *thing, va, pa, papd, papt, papg, pte, *nptep, *kptep;
	struct pmap *pmap = &p->p_vmspace->vm_pmap;
	extern struct pmap *kernel_pmap;
	extern int vm_first_phys, vm_last_phys;

#if 0
	/* is this the current address space anyways */
	if (newcr3 == rcr3())
		return;
#endif

	/* is this proc 0, which is impossible to decipher */
	if (p == &proc0)
		return;

	/* is process loaded and runnable */
	if (!(p->p_flag & SLOAD))
		pg("not loaded");

	/* does new page directory correspond with the process? */
	papd = get_pmap_pte(pmap, pmap->pm_pdir) & PG_FRAME;
	if (papd != newcr3)
		pg("ptd phys addr is %x, should be %x", newcr3, papd);
	if (newcr3 >= IOM_START && newcr3 < IOM_END)
		pg("ptd phys addr %x is in I/O", newcr3);
	if (newcr3 < vm_first_phys || newcr3 >= vm_last_phys)
		pg("ptd phys addr %x is in kernel or nonexistent", newcr3);

	/* is the kernel mapped? (per archtypal map) */
	nptep = (u_long *)&pmap->pm_pdir[KPTDI_FIRST];
	kptep = (u_long *)&kernel_pmap->pm_pdir[KPTDI_FIRST];
	for (; nptep <= (u_long *)&pmap->pm_pdir[KPTDI_LAST]; ++nptep, ++kptep)
		if ((*nptep ^ *kptep) & ~(PG_U|PG_M))
			pg("new pde for kernel %x != standard one %x",
				*nptep, *kptep);

	/* is the kernel map intact? */
	for (va = KERNBASE; va < (u_long) &end; va += NBPG) {
		pte = get_pmap_pte(pmap, va);
		if ((pte & PG_V) == 0) {
			pg("invalid pte %x at va %x in kernel", pte, va);
			break;
		}
	}

	/* is the pc going to be valid ? */
	va = p->p_addr->u_pcb.pcb_tss.tss_eip;
	if (va < KERNBASE || va >= (u_long) &etext)
		pg("new kernel eip %x is outside kernel text", va);

	/* is the stack going to be valid ? */
	va = p->p_addr->u_pcb.pcb_tss.tss_esp;
#ifndef NO_KSTACK
	if (! ((va >= (u_long) kstack && va < (u_long) kstack + i386_ptob(UPAGES)) ||
#else
	if (! (/* (va >= (u_long) kstack && va < (u_long) kstack + i386_ptob(UPAGES)) || */
#endif
	       (va >= (u_long) &edata && va < (u_long) &end)))
		/* stack must be in kstack area or in kernel bss */
		pg("new kernel esp %x is outside kstack and bss", va);
#ifndef NO_KSTACK
	for (va = (u_long) kstack;
	     va < (u_long) kstack + i386_ptob(UPAGES);
	     va += NBPG) {
		pte = get_pmap_pte(pmap, va);
		if ((pte & PG_V) == 0) {
			pg("pte %x for kstack va %x is invalid", pte, va);
			continue;
		}
		pa = pte & PG_FRAME;
		if (pa >= IOM_START && pa < IOM_END)
			pg("pa %x for kstack va %x references i/o space",
				pa, va);
		else if (pa < vm_first_phys || pa >= vm_last_phys)
			pg("pa %x for kstack va %x is outside physical memory space", pa, va);
	}
#endif

	/* don't leave the page tables mapped (paranoia) */
	*(u_long *)&APTDpde = 0;
	tlbflush();
}

#endif /* CHKCR3 */

extern struct pte	*CMAP1, *CMAP2;
extern caddr_t		CADDR1, CADDR2;
/*
 * zero out physical memory
 * specified in relocation units (NBPG bytes)
 */
clearseg(n) {

	*(int *)CMAP2 = PG_V | PG_KW | ctob(n);
	load_cr3(rcr3());
	bzero(CADDR2,NBPG);
	*(int *) CADDR2 = 0;
}

/*
 * copy a page of physical memory
 * specified in relocation units (NBPG bytes)
 */
copyseg(frm, n) {

	*(int *)CMAP2 = PG_V | PG_KW | ctob(n);
	load_cr3(rcr3());
	bcopy((void *)frm, (void *)CADDR2, NBPG);
}

/*
 * copy a page of physical memory
 * specified in relocation units (NBPG bytes)
 */
physcopyseg(frm, to) {

	*(int *)CMAP1 = PG_V | PG_KW | ctob(frm);
	*(int *)CMAP2 = PG_V | PG_KW | ctob(to);
	load_cr3(rcr3());
	bcopy(CADDR1, CADDR2, NBPG);
}

/*
 * insert an element into a queue 
 */
#undef insque
_insque(element, head)
	register struct prochd *element, *head;
{
	element->ph_link = head->ph_link;
	head->ph_link = (struct proc *)element;
	element->ph_rlink = (struct proc *)head;
	((struct prochd *)(element->ph_link))->ph_rlink=(struct proc *)element;
}

/*
 * remove an element from a queue
 */
#undef remque
_remque(element)
	register struct prochd *element;
{
	((struct prochd *)(element->ph_link))->ph_rlink = element->ph_rlink;
	((struct prochd *)(element->ph_rlink))->ph_link = element->ph_link;
	element->ph_rlink = (struct proc *)0;
}

vmunaccess() {}

/*
 * Machine chip & clock speed configuration
 * For now, only distinguish 386 and  486 (no SX).
 */
char	s386[] = "80386";
char	s486[] = "80486";
char	s586[] = "Pentium";

struct cpuident cpuid[] = {
	CPU_386,	0,		s386, 	5,	0, 
	CPU_386,	29460, 		s386,	1,	8,	/* 8 Mhz  */
	CPU_386,	44190,		s386, 	2,	16,	/* 16 Mhz */
	CPU_386,	55237,		s386, 	3,	20,	/* 20 Mhz */
	CPU_386,	77195,		s386, 	5,	25,	/* 25 Mhz */
	CPU_386,	89608,		s386,	6,	33,	/* 33 Mhz */
	CPU_386,	120000,		s386,	7,	40,	/* 40 Mhz */
	CPU_386,	INT_MAX,	s386, 	5,	0,
	CPU_486,	0,		s486, 	8,	0,
	CPU_486,	57750,		s486,	5,	8,	/* 8 Mhz */
	CPU_486,	120000,		s486,	7,	25,	/* 25 Mhz */
	CPU_486,	145250,		s486,	10,	33,	/* 33 Mhz */
	CPU_486,	215000,		s486, 	12,	50,	/* 50 Mhz */
	CPU_486,	INT_MAX,	s486, 	8,	0,
	CPU_586,	INT_MAX,	s586, 	25,	0,	/* unknown */
	-1,		-1, 		0,	-1,	0
};

identifycpu()
{
	register unsigned int tick;
	register int time1, time2;
	register struct cpuident *cp;
	extern int cpu_id;		/* from cpuid instruction */

	/*
	 * The following is obsolete, and remains solely
	 * for printing the estimated CPU speed.
	 */
	tick = 0;
	time1 = readrtcsec();
	if (time1 < 0)
		goto noclock;
	while (time1 == (time2 = (readrtcsec())))
		;
	while (time2 == (readrtcsec()))
		tick++;

noclock:
	for (cp = cpuid; cp->cputype >= 0; cp++) {
		if (cpu == cp->cputype && tick <= cp->cliplevel) {
			printf("cpu = %s ", cp->ident);
			if (cp->mhz)
				printf("(about %d MHz) ", cp->mhz);
			else
				printf("(unknown speed) ");
#ifdef DEBUG
			printf("[got %d ticks] ", tick);
#endif
			if (cpu_id)
				printf("model %d, stepping %d",
				    (cpu_id & 0xf0) >> 4, cpu_id & 0xf);
			printf("\n");
			cpuspeed = cp->speed;
			break;
		}
	}
	if (cp->cputype < 0) {
		printf("Unidentified CPU type and speed\n");
#ifdef DEBUG
		printf("(type %d, %d ticks)\n", cpu, tick);
#endif
	}

	delay_calibrate();

	/*
	 * Set up sys_model string, now that we know.
	 * Initialize invariant parts of sysinfo, including
	 * string offsets.  This should be done elsewhere...
	 * String offsets are relative to the start of the sysinfo
	 * buffer.
	 */
	strncpy(sys_model, cp->ident, sys_modelbufsize);

#define	SETSTROFF(member,string) \
	(sysinfo.member = (char *) ((string) - sysinfostrings + sizeof(struct sysinfo)))

	/* machine info */
	SETSTROFF(sys_machine,machine);
	SETSTROFF(sys_model,sys_model);
	sysinfo.sys_ncpu = 1;
	sysinfo.sys_cpuspeed = cpuspeed;
	sysinfo.sys_physmem = ctob(physmem);
	sysinfo.sys_pagesize = NBPG * CLSIZE;

	/* system info */
	SETSTROFF(sys_ostype,ostype);
	SETSTROFF(sys_osrelease,osrelease);
	sysinfo.sys_os_revision = BSD;
	sysinfo.sys_posix1_version = 0;		/* not quite */
	SETSTROFF(sys_version,version);

	/* system parameters */
	sysinfo.sys_ngroup_max = NGROUPS_MAX;
	sysinfo.sys_argmax = ARG_MAX;
	sysinfo.sys_openmax = OPEN_MAX;		/* XXX */
	sysinfo.sys_childmax = CHILD_MAX;	/* XXX */

	/* local info */
	SETSTROFF(sys_hostname,hostname);
}

readrtcsec()
{
	register int cstatus;

        cstatus = rtcin(RTC_STATUSA);
        if (cstatus == 0xff || cstatus == 0) 
		return (-1);
        while ((cstatus & RTCSA_TUP) == RTCSA_TUP)
                cstatus = rtcin(RTC_STATUSA);
	return (bcd2i(rtcin(RTC_SEC)));
}

int delay_mult = 640;			/* in case used before calibration */

delay(usec)
	int usec;
{
	int thistime;
	volatile int delay_count;

	while (usec > 0) {
		thistime = min(usec, 1000000);

		delay_count = (thistime * delay_mult + 64) / 128;
		while (delay_count-- > 0)
			;

		usec -= thistime;
	}
}

/*
 * The counter chip counts down from about 12000 to 0 every 10 milliseconds.
 * Wrap around is funny, so we use the counter by waiting until it has
 * a high value, then doing a short delay, then reading the counter to
 * get a delta.  To find the right multipler for the DELAY loop, we start
 * at the minimum, and try every value until a request one millisecond delay
 * actually takes >= 1 millisecond.  Here are some measurements:
 *
 *  Machine              Multiplier
 *  TPE 50 MHz 486          537
 *  Bell Tech 16 MHz 386     42
 *
 * It takes about 1 second of real time to find the multiplier on the TPE,
 * and less than a second on the Bell Tech.  In 100 trials, the TPE got
 * 537 or 536 in every trial, and the Bell Tech got 42 in every trial.
 *
 * The granularity is about 100/multiplier, so it is about 2us for a
 * slow machine, and about 200ns for a fast one.
 *
 * The accuracy of the resulting delays is about 1% (assuming the user
 * doesn't change the TURBO switch!)
 */
#define	MIN_MULT	32		/* smallest believable multiplier */
#define	MAX_MULT	10000

delay_calibrate()
{
	int counts_per_millisec;
	int start, end;
	int s;
	char *fail = NULL;

	initrtclock();		/* sets mode 2 */

	s = splhigh();
	/* the following assumes we are using mode 2, not mode 3 */
	counts_per_millisec = (TIMER_FREQ + 500) / 1000;

	for (delay_mult = 1; delay_mult < MAX_MULT; delay_mult++) {
		while ((start = readrtclock()) < 8000 && start != -1)
			;
		delay(1000);
		end = readrtclock();

		if (start == -1 || end == -1) {
			fail = "impossible values from timer 0";
			break;
		}
		if (end > start || (start - end) > counts_per_millisec)
			break;
	}
	splx(s);

	if (fail == NULL) {
		if (delay_mult == MAX_MULT)
			fail = "multiplier hit maximum";
		if (delay_mult < MIN_MULT) {
			printf("delay multiplier %d!\n", delay_mult);
			fail = "unbelievably low multiplier";
		}
	}
	if (fail)
		printf("Warning: delay calibration: %s (timer not working?)\n",
		    fail);
	if (fail || delay_mult < MIN_MULT)
		delay_mult = cpuspeed * 60;	/* rough guess */
/* #ifdef DEBUG */
	printf("delay multiplier %d\n", delay_mult);
/* #endif */
}

/*
 * Given a starting I/O port number and a count, allow user processes
 * to access those ports directly.  Currently applies to all user processes,
 * eventually should apply only to the calling process.
 */
mapioport(base, cnt, p)
	int base, cnt;
	struct proc *p;
{
	register int i;

	if (i = suser(p->p_ucred, &p->p_acflag))
		return (i);
	for (i = base; i < base + cnt && i < USER_IOPORTS; i++)
		tss.tss_iomap[i / NBBY] &= ~(1 << (i % NBBY));
	return (0);
}

/*
 * Given a starting I/O port number and a count, deny user processes
 * direct access to those ports.  Currently applies to all user processes,
 * eventually should apply only to the calling process.
 */
unmapioport(base, cnt, p)
	int base, cnt;
	struct proc *p;
{
	register int i;

	if (i = suser(p->p_ucred, &p->p_acflag))
		return (i);
	for (i = base; i < base + cnt && i < USER_IOPORTS; i++)
		tss.tss_iomap[i / NBBY] |= 1 << (i % NBBY);
	return (0);
}

/*
 * Enable IOPL privilege for the specified process,
 * assumed to be the current process.  This allows the process
 * to access all I/O ports, disable/enable interrupts, and generally
 * raise havoc.
 */
enable_IOPL(p)
	struct proc *p;
{
	int i;

	if (i = suser(p->p_ucred, &p->p_acflag))
		return (i);
	p->p_md.md_flags |= MDP_IOPL;
	p->p_regs[sEFLAGS] |= PSL_IOPL;
	return (0);
}

/*
 * Disable IOPL privilege for the specified process,
 * assumed to be the current process.  This requires no special
 * privilege.
 */
disable_IOPL(p)
	struct proc *p;
{

	p->p_md.md_flags &= ~MDP_IOPL;
	p->p_regs[sEFLAGS] |= PSL_IOPL;
	return (0);
}
