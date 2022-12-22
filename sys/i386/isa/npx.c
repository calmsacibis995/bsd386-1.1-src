/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: npx.c,v 1.10 1994/01/30 04:19:02 karels Exp $
 */

/*-
 * Copyright (c) 1990 William Jolitz.
 * Copyright (c) 1991 The Regents of the University of California.
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
 *	@(#)npx.c	7.2 (Berkeley) 5/12/91
 */
#include "param.h"
#include "systm.h"
#include "conf.h"
#include "file.h"
#include "proc.h"
#include "user.h"
#include "device.h"
#include "machine/cpu.h"
#include "machine/trap.h"
#include "ioctl.h"

#include "../i386/specialreg.h"
#include "../i386/fp_reg.h"
#include "isavar.h"
#include "icu.h"

/*
 * 387 and 287 Numeric Coprocessor Extension (NPX) Driver.
 */

struct npx_softc {
	struct  device sc_dev;  /* base device */
	struct  isadev sc_id;   /* ISA device */
	struct  intrhand sc_ih; /* interrupt vectoring */
};

int	npxprobe(), npxintr();
void	npxattach();
int	npxdna __P((struct trapframe *));

extern	int (*dnatrap)  __P((struct trapframe *));

struct cfdriver npxcd =
	{ NULL, "npx", npxprobe, npxattach, sizeof(struct npx_softc) };

struct	proc *npxproc;	/* process who owns device, otherwise zero */

/*
 * Probe routine - look for device, set emulator bit if not present
 */
npxprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	static int status, control;

#ifdef lint
	npxintr();
#endif

	/* insure EM bit off */
	load_cr0(rcr0() & ~CR0_EM);	/* stop emulating */
	asm("fninit");	/* put device in known state */

	/* check for a proper status of zero */
	status = 0xa5a5;	
	asm("fnstsw %0" : "=m" (status));

	if (status == 0) {

		/* good, now check for a proper control word */
		control = 0xa5a5;	
		asm("fnstcw %0" : "=m" (control));

		if ((control&0x103f) == 0x3f) {
			/* then we have a numeric coprocessor */
			/*
			 * On 486 and better, use numeric exception
			 * rather than external interrupts for exceptions.
			 * Otherwise, FP coproc is always at intr 13.
			 */
			if (cpu > CPU_386) {
				ia->ia_irq = 0;
				load_cr0(rcr0() | CR0_NE);
			} else if (ia->ia_irq == IRQUNK)
				ia->ia_irq = IRQ13;
			/* turn CR0_EM back on in npxattach() */
			return (1);
		}
	}

	/* insure EM bit on */
	load_cr0(rcr0() | CR0_EM);	/* start emulating */
	return (0);
}

/*
 * Attach routine - announce which it is, and wire into system
 */
void
npxattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct npx_softc *sc = (struct npx_softc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int npxintr();
	
	printf("\n");
	npxinit(curproc, FPC_DEFAULT);
	/* check for ET bit to decide 387/287 */
	/*outb(0xb1,0);		/* reset processor */
	load_cr0(rcr0() | CR0_EM);	/* start emulating */
	isa_establish(&sc->sc_id, &sc->sc_dev);
	if (ia->ia_irq) {
		sc->sc_ih.ih_fun = npxintr;
		sc->sc_ih.ih_arg = NULL;
		intr_establish(ia->ia_irq, &sc->sc_ih, DV_COPROC);
	}
	dnatrap = npxdna;
}

/*
 * Initialize floating point unit when first using floating point
 * in a process.  Assumes that CR0_EM is not set.
 */
npxinit(p, control)
	struct proc *p;
	int control;
{

	asm volatile ("fninit");
	asm volatile ("fldcw %0" : : "m" (control));
}

/*
 * Handle npx coprocessor interrupt.
 * Save the floating point state for possible debugger or core dump.
 * For now we just call psignal, as we may be called from an inconvenient
 * state, such as in kernel mode or part-way into a trap.
 * Later, we will defer exception processing until it is known
 * to be safe.
 */
npxintr(framep)
	struct intrframe *framep;
{
	struct trapframe *tfp = (struct trapframe *) &framep->if_es;

	outb(0xf0,0);		/* reset processor */

	/*
	 * npxproc may be NULL if the process that caused
	 * the exception has already exited
	 */
	if (npxproc) {
		/*
		 * Sync state in process context structure,
		 * in advance of debugger/process looking for it.
		 * We re-load the npx state, as fnsave reinitializes
		 * the fp state.  The pending exception is left pending
		 * until the program explicitly clears it or exits.
		 */
		asm volatile ("fnsave %0" :
		    "=m" (npxproc->p_addr->u_pcb.pcb_savefpu));
		asm volatile ("frstor %0" : :
		    "m" (npxproc->p_addr->u_pcb.pcb_savefpu));

		psignal(npxproc, SIGFPE);
	}
	return (1);
}

/*
 * Implement device not available (DNA) exception
 */
npxdna(tfp)
	struct trapframe *tfp;
{

	load_cr0(rcr0() & ~CR0_EM);	/* stop emulating */
	if (npxproc && npxproc != curproc)
		asm volatile ("fnsave %0" :
		    "=m" (npxproc->p_addr->u_pcb.pcb_savefpu));
	if (npxproc != curproc) {
		if ((curpcb->pcb_flags & FP_WASUSED) == 0) {
			curpcb->pcb_flags |= FP_WASUSED;
			npxinit(curproc, FPC_DEFAULT);
		} else
			asm volatile ("frstor %0" : :
			    "m" (curpcb->pcb_savefpu));
		npxproc = curproc;
	}
	return (0);
}
