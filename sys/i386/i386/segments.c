/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: segments.c,v 1.1 1993/11/12 01:57:23 karels Exp $
 */

#include "param.h"
#include "systm.h"
#include "proc.h"
#include "machine/pcb.h"
#include "machine/vmlayout.h"
#include "machine/segments.h"

extern union descriptor gdt[];
extern union descriptor ldt[];

/*
 * Given a selector, return a descriptor.
 */
int
getdescriptor(p, uap, retval)
	struct proc *p;
	struct args {
		int	selector;
		union	descriptor *desc;
	} *uap;
	int *retval;
{
	union descriptor d;
	int i = IDXSEL(uap->selector);
	int error;

	if (ISLDT(uap->selector)) {
		if (i >= NLDT)
			return (EINVAL);
		d = ldt[i];
	} else {
		if (i >= NGDT)
			return (EINVAL);
		d = gdt[i];
	}

	if (error = copyout(&d, uap->desc, sizeof d))
		return (error);

	return (0);
}

/*
 * Given a selector, set a descriptor.
 * Currently only works for LDEFCALLS_SEL.
 */
int
setdescriptor(p, uap, retval)
	struct proc *p;
	struct args {
		int	selector;
		union	descriptor *desc;
	} *uap;
	int *retval;
{
	union descriptor d;
	vm_offset_t v;
	int i = IDXSEL(uap->selector);
	int error;

	/* change this when we can set other descriptors */
	if (!ISLDT(uap->selector) || i != LDEFCALLS_SEL)
		return (EINVAL);

	if (error = copyin(uap->desc, &d, sizeof d))
		return (error);

	/*
	 * Rules:
	 *	We can match an existing kernel syscall gate.
	 *	We can install a user call gate:
	 *		to the standard code segment
	 *		at the user privilege level
	 *		with 'present' bit set
	 *		and offset within bounds.
	 * Note that the RPL of the selector within the call gate
	 * is not used, and the stack copy count isn't used
	 * when the call doesn't cross protection levels.
	 */
	if (bcmp(&d, &ldt[L43BSDCALLS_SEL], sizeof d) == 0 ||
#if 0
	    bcmp(&d, &ldt[LSYS5CALLS_SEL], sizeof d) == 0 ||
#endif
	    (d.d_type == SDT_SYS386CGT &&
	     ISLDT(d.gd.gd_selector) &&
	     IDXSEL(d.gd.gd_selector) == LUCODE_SEL &&
	     d.gd.gd_dpl == SEL_UPL &&
	     d.gd.gd_p == 1 &&
	     (v = d.gd.gd_looffset + (d.gd.gd_hioffset << 16)) >=
	     VM_MIN_ADDRESS && v < VM_MAXUSER_ADDRESS)) {
		ldt[i] = d;
		curpcb->pcb_ldtdefcall = d;
		return (0);
	}

	return (EINVAL);
}
