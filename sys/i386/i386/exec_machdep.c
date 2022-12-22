/*-
 * Copyright (c) 1991, 1992, 1993 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: exec_machdep.c,v 1.18 1993/12/18 18:03:03 karels Exp $
 */

#include "param.h"
#include "systm.h"
#include "proc.h"
#include "exec.h"
#include "user.h"
#include "vnode.h"

#include "vm/vm.h"

#include "machine/cpu.h"
#include "machine/psl.h"
#include "machine/segments.h"

#include "text.h"

#ifdef COFF

#include "malloc.h"
#include "mman.h"
#include "vm/vm_kern.h"
#include "machine/coff_compat.h"

static int exec_coff_binary __P((struct text *));

#endif

/*
 * Given the initial page of an executable file,
 * find out what format it uses and convert the header
 * into a series of load commands.
 * This function may call machine-independent code
 * to handle formats common to all architectures
 * (e.g. interpreted scripts, 0413 binaries, etc.).
 */
int
analyze_exec_header(xp)
	struct text *xp;
{
	int (*handler) __P((struct text *));
	int magic;
	int error = 0;

	/*
	 * We loop here because the specific exec_*() function
	 * may not complete the analysis with just one pass.
	 * A script requires at least two passes (to start an interpreter);
	 * conceivably shared library loads may need multiple passes too.
	 */
	do {
		if (xp->x_size < sizeof (unsigned long))
			return (ENOEXEC);

		/*
		 * On the 386, the magic number currently lies
		 * in the first two bytes.
		 */
		magic = *(unsigned short *) xp->x_header;

		switch (magic) {

		/* interpreters (note byte order dependency) */

		case '#' | '!' << 8:
			handler = exec_interpreter;
			break;

		/* BSD native formats */

		case QMAGIC:
			handler = exec_compact_demand_load_binary;
			break;

		case ZMAGIC:
			handler = exec_demand_load_binary;
			break;

		case NMAGIC:
			handler = exec_shared_binary;
			break;

		case OMAGIC:
			handler = exec_unshared_binary;
			break;

		/* foreign formats */

#ifdef COFF
		case COFF_MAGIC:
			handler = exec_coff_binary;
			break;
#endif

		default:
			return (ENOEXEC);
		}
	} while ((error = (*handler)(xp)) == EAGAIN);

	return (error);
}

#ifdef COFF

static struct exec_load *
load_init(op, v, size, prot)
	int op;
	vm_offset_t v;
	vm_size_t size;
	vm_prot_t prot;
{
	struct exec_load *lp;

	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	lp->el_op = op;
	lp->el_vnode = 0;
	lp->el_offset = 0;
	lp->el_address = v;
	lp->el_length = size;
	lp->el_prot = prot;
	lp->el_attr = MAP_COPY;
	lp->el_next = 0;
	return (lp);
}

static struct exec_load **
load_zero(v, size, lpp)
	vm_offset_t v;
	vm_size_t size;
	struct exec_load **lpp;
{

	*lpp = load_init(EXEC_ZERO, v, size, VM_PROT_ALL);
	return (&(*lpp)->el_next);
}

static struct exec_load **
load_clear(v, size, lpp)
	vm_offset_t v;
	vm_size_t size;
	struct exec_load **lpp;
{

	*lpp = load_init(EXEC_CLEAR, v, size, VM_PROT_ALL);
	return (&(*lpp)->el_next);
}

static struct exec_load **
load_file(off, vp, v, size, prot, lpp)
	off_t off;
	struct vnode *vp;
	vm_offset_t v;
	vm_size_t size;
	vm_prot_t prot;
	struct exec_load **lpp;
{
	struct exec_load *lp;

	lp = load_init(EXEC_MAP, v, size, prot);
	lp->el_vnode = vp;
	VREF(vp);
	lp->el_offset = off;
	*lpp = lp;
	return (&lp->el_next);
}

/*
 * Load a shared library given a COFF shared library section.
 * We recurse by calling analyze_exec_header().
 */
static struct exec_load **
load_shlib(xp, csp, lpp)
	struct text *xp;
	struct coff_scnhdr *csp;
	struct exec_load **lpp;
{
	struct exec_load *lp;
	struct shlib_entry *sebuf, *se, *last_se;
	struct text *lxp;
	int resid = 0;

	/* read the shared library section from the binary */
	MALLOC(sebuf, struct shlib_entry *, csp->s_size, M_TEMP, M_WAITOK);
	if (vn_rdwr(UIO_READ, xp->x_vnode, (caddr_t)sebuf, csp->s_size,
	    (off_t)csp->s_scnptr, UIO_SYSSPACE, (IO_UNIT|IO_NODELOCKED),
	    xp->x_proc->p_ucred, &resid, xp->x_proc))
	    	/* XXX how do we pass the errno back? */
	    	return (0);

	/* create an alternate text structure for the library or libraries */
	MALLOC(lxp, struct text *, sizeof *lxp, M_TEMP, M_WAITOK);
	*lxp = *xp;
	lxp->x_flags = X_PATH_SYSSPACE;
	lxp->x_vnode = 0;
	lxp->x_header = 0;
	lxp->x_load_commands = 0;

	/* loop over library names, analyzing and collecting load commands */
	last_se = (struct shlib_entry *)((caddr_t)sebuf + csp->s_size);
	for (se = sebuf;
	    se < last_se;
	    se = (struct shlib_entry *)((long *)se + se->entsz)) {
		if (se->entsz <= 0 || se->pathndx < 2 ||
		    se->pathndx >= se->entsz) {
		    	lpp = 0;
		    	break;
		}
		lxp->x_path = (char *)((long *)se + se->pathndx);
		if (exec_lookup(lxp) || analyze_exec_header(lxp)) {
			lpp = 0;
			break;
		}
		if (lp = lxp->x_load_commands) {
			*lpp = lp;
			while (lp->el_next)
				lp = lp->el_next;
			lpp = &lp->el_next;
		}
		if (lxp->x_header && vm_deallocate(kernel_map,
		    (vm_offset_t)lxp->x_header, CLBYTES))
		        panic("load_shlib delete header");
		lxp->x_header = 0;
		lxp->x_load_commands = 0;
		if (lxp->x_vnode)
			vput(lxp->x_vnode);
		lxp->x_vnode = 0;
	}

	FREE(lxp, M_TEMP);
	FREE(sebuf, M_TEMP);

	/* XXX we can lose errno info here */
	return (lpp);
}

static int
load_emulator(xp, lpp)
	struct text *xp;
	struct exec_load **lpp;
{
	static const char emulator[] = COFF_EMULATOR;
	struct text *lxp;
	struct exec_load *lp;
	vm_offset_t o, v;
	int error;

	MALLOC(lxp, struct text *, sizeof *lxp, M_TEMP, M_WAITOK);
	*lxp = *xp;
	lxp->x_flags = X_PATH_SYSSPACE;
	lxp->x_path = (char *)emulator;
	lxp->x_vnode = 0;
	lxp->x_header = 0;
	lxp->x_load_commands = 0;

	error = exec_lookup(lxp);
	if (!error)
		error = analyze_exec_header(lxp);
	if (!error && (lp = lxp->x_load_commands) == 0)
		error = ENOEXEC;
	if (!error && lxp->x_flags & X_ENTRY) {
		/*
		 * XXX Relocate the segments...
		 * This is a hack for QMAGIC/ZMAGIC emulators.
		 * If the address of a segment is less than
		 * the address of the entry point, rounded down
		 * to a 4 MB boundary, we boost it over that boundary.
		 */
		o = lxp->x_entry & ~PDROFSET;
		for (; lp; lp = lp->el_next)
			if (lp->el_op == EXEC_ZERO &&
			    lp->el_address ==
			    (vm_offset_t)USRSTACK - INITSSIZ) {
				lp->el_address = o - COFF_STACKSIZE;
				lp->el_length = COFF_STACKSIZE;
			} else if (lp->el_address < o &&
			    (v = lp->el_address + o) < VM_MAXUSER_ADDRESS)
			    	lp->el_address = v;
		exec_set_entry(xp, lxp->x_entry);
	}
	if (!error)
		*lpp = lxp->x_load_commands;
	if (lxp->x_header && vm_deallocate(kernel_map,
	    (vm_offset_t)lxp->x_header, CLBYTES))
		panic("load_emulator delete header");
	if (lxp->x_vnode)
		vput(lxp->x_vnode);

	FREE(lxp, M_TEMP);
	return (error);
}

/*
 * Load a foreign SCO/Interactive COFF binary.
 * Such binaries are implicitly loaded with a shared library
 * that traps system calls and translates them to BSD calls.
 */
int
exec_coff_binary(xp)
	struct text *xp;
{
	struct coff_filehdr *cfp = (struct coff_filehdr *)xp->x_header;
	struct coff_aouthdr *cap = (struct coff_aouthdr *)&cfp[1];
	struct coff_scnhdr *csp, *last_csp;
	struct exec_load *lp, **lpp;
	vm_offset_t v;
	vm_size_t s;
	vm_prot_t prot;
	int error = 0;

	/*
	 * Sanity checking.
	 */
	if (xp->x_size < sizeof (*cfp))
	    	/* header not all present */
	    	return (ENOEXEC);
	if (cfp->f_nscns > (unsigned)0x80000000 / sizeof (struct coff_scnhdr))
		/* ridiculously large section count */
		return (ENOEXEC);
	if (cfp->f_nscns < 3)
		/* at least TEXT, DATA, BSS, per iBCS2 p5-1 */
		return (ENOEXEC);
	if (cfp->f_opthdr < sizeof (*cap))
		/* consistency check */
		return (ENOEXEC);
#if 0
	if (xp->x_size < sizeof (*cfp) + cfp->f_opthdr +
	    cfp->f_nscns * sizeof (struct coff_scnhdr))
	    	/* file too small to contain the full COFF header */
	    	return (ENOEXEC);
#endif
	if ((cfp->f_flags & COFF_F_EXEC) == 0)
		/* executables and shared libs must set this, iBCS2 p4-4 */
		return (ENOEXEC);

	/* XXX limitations of current implementation */
	if (sizeof (*cfp) + cfp->f_opthdr +
	    cfp->f_nscns * sizeof (struct coff_scnhdr) > CLBYTES)
	    	/* XXX header too big; should re-map x_header here */
	    	return (ENOEXEC);
	if (cap->magic != COFF_ZMAGIC && cap->magic != COFF_LMAGIC)
	    	/* XXX no support for COFF_OMAGIC or COFF_NMAGIC now */
	    	return (ENOEXEC);

	/*
	 * Loop over COFF sections and load them.
	 */
	csp = (struct coff_scnhdr *)
	    ((caddr_t)cfp + sizeof (*cfp) + cfp->f_opthdr);
	last_csp = &csp[cfp->f_nscns];
	lpp = &xp->x_load_commands;
	if (lp = *lpp) {
		/* XXX maybe it's time for bidirectional exec_load lists? */
		while (lp->el_next)
			lp = lp->el_next;
		lpp = &lp->el_next;
	}
	xp->x_dsize = 0;	/* build this incrementally */

	for (; csp < last_csp; ++csp) {

		if (csp->s_flags & COFF_STYP_FEATURES) {
			if (csp->s_flags & (COFF_STYP_DSECT|COFF_STYP_NOLOAD))
				/* dummy or unloadable section */
				continue;
			if (csp->s_flags & COFF_STYP_GROUP)
				/* XXX don't understand this yet */
				return (ENOEXEC);
			if (csp->s_flags & COFF_STYP_PAD)
				/* XXX is this the right way? */
				continue;
		}

		switch (csp->s_flags) {

		case COFF_STYP_TEXT:
			prot = VM_PROT_READ|VM_PROT_EXECUTE;
			goto textdata;

		case COFF_STYP_DATA:
			prot = VM_PROT_ALL;
		textdata:
			v = i386_trunc_page(csp->s_vaddr);
			s = i386_round_page(csp->s_vaddr + csp->s_size) - v;
			lpp = load_zero(v, s, lpp);
			lpp = load_file(i386_trunc_page(csp->s_scnptr),
			    xp->x_vnode, v, s, prot, lpp);
			if (prot == VM_PROT_ALL) {
				xp->x_daddr = (caddr_t)v;
				xp->x_dsize += howmany(s, CLBYTES);
			} else {
				xp->x_taddr = (caddr_t)v;
				xp->x_tsize = howmany(s, CLBYTES);
			}
			break;

		case COFF_STYP_BSS:
			/* XXX ISC binaries don't set s_vaddr for bss? */
			/* XXX requires data to precede bss */
			/* XXX assumes no overlap with a following section */
			v = i386_round_page(csp->s_vaddr);
			if (s = v - csp->s_vaddr)
				lpp = load_clear(csp->s_vaddr, s, lpp);
			s = i386_round_page(csp->s_vaddr + csp->s_size) - v;
			if (s) {
				lpp = load_zero(v, s, lpp);
				xp->x_dsize += howmany(s, CLBYTES);
			}
			break;

		case COFF_STYP_LIB:
			if (cap->magic == COFF_LMAGIC)
			    	/* XXX don't permit recursion (yet) */
			    	return (ENOEXEC);

			if (csp->s_size == 0)
				break;
			if ((unsigned)csp->s_size > NBPG)
				/* XXX guard against attack */
				return (ENOEXEC);

			if ((lpp = load_shlib(xp, csp, lpp)) == 0)
				return (ENOEXEC);
			break;

		case COFF_STYP_INFO:
			/* ignore it */
			break;

		default:
			return (ENOEXEC);
			break;
		}
	}

	/* XXX honor setuid bits only on emulator file... */
	if (cap->magic != COFF_LMAGIC) {
		/* create a stack segment */
		lpp = load_zero(USRSTACK - INITSSIZ, INITSSIZ, lpp);

		error = load_emulator(xp, lpp);
	}
	if (!error)
		exec_set_entry(xp, cap->entry);
	return (error);
}

#endif /* COFF */

/*
 * Set up the initial register and signal state for the new image.
 * We don't yet support other OS emulations.
 */
void
exec_set_state(xp)
	struct text *xp;
{
	extern union descriptor ldt[];
	int argc = 0;
	struct proc *p = xp->x_proc;
	struct exec_arg *eap;
	struct syscframe *sfp = (struct syscframe *) p->p_regs;
	struct ps_strings pss;

#ifdef COPY_SIGCODE
	extern char sigcode[];
	extern int szsigcode;

	copyout(sigcode, xp->x_stack, szsigcode);
#endif

	/*
	 * Compute argc so we can put it on the stack.
	 * This is redundant -- the startup code computes this and
	 * throws it away!
	 */
	for (eap = xp->x_args; eap->ea_string; eap = eap->ea_next)
		++argc;

	pss.ps_argv = (char **)xp->x_stack_top;
	pss.ps_argc = argc;
	pss.ps_envp = pss.ps_argv + argc + 1;
	pss.ps_nenv = xp->x_arg_count - (argc + 2);
	copyout(&pss, (caddr_t)PS_STRINGS, sizeof (pss));

	xp->x_stack_top -= sizeof (argc);
	copyout(&argc, xp->x_stack_top, sizeof (argc));

	sfp->sf_edi = 0;
	sfp->sf_esi = 0;
	sfp->sf_ebp = (int) xp->x_stack_top;
	sfp->sf_ebx = 0;
	sfp->sf_edx = 0;
	sfp->sf_ecx = 0;
	sfp->sf_eax = 0;
	sfp->sf_eip = xp->x_entry;
	sfp->sf_esp = (int) xp->x_stack_top;

	if (p->p_flag & STRC)
		/* Simulate an exect(), for gdb's sake. */
		sfp->sf_eflags |= PSL_T;
		
	/*
	 * Initialize floating point state; use default on first access.
	 */
	p->p_addr->u_pcb.pcb_flags &= ~FP_WASUSED;

	/*
	 * Reset the default system call descriptor.
	 * We may have been using redirected system calls.
	 * XXX save a syscall and set up COFF binaries here?
	 */
	ldt[LDEFCALLS_SEL] = ldt[L43BSDCALLS_SEL];
	curpcb->pcb_ldtdefcall = ldt[LDEFCALLS_SEL];
}
