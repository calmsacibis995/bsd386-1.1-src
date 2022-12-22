/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: process_machdep.c,v 1.8 1993/02/21 03:31:28 karels Exp $
 */

#include "param.h"
#include "proc.h"
#include "user.h"
#include "machine/frame.h"
#include "machine/psl.h"

#define IPCREG /* define ipcreg_t and ipcreg_s in reg.h */
#include "machine/reg.h"

extern char kstack[];

/*
 * Machine-dependent tracing functions.
 */

/*
 * Modify the process so that it will trap
 * after executing the next instruction.
 * On the 386, we just set the trace bit.
 * On some architectures, we adjust the PC queue (and set SSSTEP in p_flags).
 */
void
ptrace_single_step(p)
	struct proc *p;
{
	struct pcb *pcbp = &p->p_addr->u_pcb;
	caddr_t regs =
		((caddr_t) p->p_regs - (caddr_t) kstack) + (caddr_t) p->p_addr;

	if (pcbp->pcb_flags & FM_TRAP)
		((struct trapframe *) regs)->tf_eflags |= PSL_T;
	else
		((struct syscframe *) regs)->sf_eflags |= PSL_T;
}

/*
 * Clean up after a single-step.
 * Not used on the 386.
 */
void
ptrace_fix_step(p)
	struct proc *p;
{
}

/*
 * Set the user mode pc of the process to this new value.
 * No sanity checking; if the parent wants the child to die
 * with an illegal pc, so be it.
 */
void
ptrace_set_pc(p, addr)
	struct proc *p;
	void *addr;
{
	struct pcb *pcbp = &p->p_addr->u_pcb;
	caddr_t regs =
		((caddr_t) p->p_regs - (caddr_t) kstack) + (caddr_t) p->p_addr;

	if (pcbp->pcb_flags & FM_TRAP)
		((struct trapframe *) regs)->tf_eip = (int) addr;
	else
		((struct syscframe *) regs)->sf_eip = (int) addr;
}

/*
 * Make sure that the user doesn't read or write areas
 * of the kernel stack and u area that they shouldn't.
 * Some tests are more restrictive if we know the user is writing.
 * Return EIO if we daren't permit the operation.
 */
int
ptrace_check_u(p, offset, writing, data)
	struct proc *p;
	int offset;
	int writing;
	int data;
{
	struct pcb *pcbp = &p->p_addr->u_pcb;
	int *regs;
	int nregs;
	int regnum;
	int flagsnum;
	int i;

	if (pcbp->pcb_flags & FM_TRAP) {
		regs = tipcreg;
		nregs = tNIPCREG;
		flagsnum = tEFLAGS;
	} else {
		regs = sipcreg;
		nregs = sNIPCREG;
		flagsnum = sEFLAGS;
	}

	if (offset < 0 || offset & 03 ||
	    offset + sizeof(int) > UPAGES * CLBYTES)
		return (EIO);

	if (writing) {
		regnum = (offset - ((caddr_t)p->p_regs - (caddr_t)kstack)) /
		    sizeof(int);

		if (regnum == flagsnum) {
			if ((data & PSL_USERCLR) != 0 ||
			    (data & PSL_USERSET) != PSL_USERSET ||
			    (data & PSL_IOPL &&
			    (p->p_md.md_flags & MDP_IOPL) == 0))
				return (EPERM);
			return (0);
		}

		for (i = 0; i < nregs; i++)
			if (regs[i] == regnum)
				return (0);
		return (EIO);
	}

	return (0);
}
