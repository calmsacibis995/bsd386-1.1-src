/*
 * Copyright (c) 1991, 1992, 1993 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: kern_exec.c,v 1.42 1993/12/19 05:03:44 karels Exp $
 */

#include "param.h"
#include "systm.h"
#include "filedesc.h"
#include "kernel.h"
#include "proc.h"
#include "mount.h"
#include "malloc.h"
#include "namei.h"
#include "vnode.h"
#include "seg.h"
#include "file.h"
#include "acct.h"
#include "exec.h"
#include "ktrace.h"
#include "resourcevar.h"

#include "machine/cpu.h"
#include "machine/reg.h"

#include "mman.h"
#include "vm/vm.h"
#include "vm/vm_param.h"
#include "vm/vm_map.h"
#include "vm/vm_kern.h"
#include "vm/vm_pager.h"

#include "signalvar.h"
#include "kinfo_proc.h"

#include "text.h"

/*
 * isblank() classifies the characters that separate tokens
 * in the header of an interpreted script.
 */
#define	isblank(c)	((c) == ' ' || (c) == '\t')

/*
 * Execve system call.
 * The kernel loader: brings a new image into memory and executes it.
 */
int
execve(p, uap, retval)
	struct proc *p;
	struct args {
		char	*path;	/* Argument names from POSIX.1 3.1.2.1. */
		char	**argv;
		char	**envp;
	} *uap;
	int *retval;
{
	struct text *xp;
	struct exec_load *lp;
	struct ucred *cr = p->p_ucred;
	vm_map_t map;
	vm_offset_t addr;
	int error;
#ifdef KTRACE
	struct vnode *tracep = p->p_tracep;
#endif
#ifdef COPY_SIGCODE
	extern int szsigcode;
#endif

	/*
	 * POSIX.1 exec errors:
	 *	E2BIG: arg list + env list sizes > ARG_MAX
	 *	EACCES: namei() failure; execute permission missing; or not a
	 *		regular file (but null string (current dir) is ENOENT).
	 *	ENOENT: namei() failure; null path
	 *	ENAMETOOLONG, ENOTDIR: namei() failure
	 *	ENOEXEC: permissions and file type ok, but unrecognized format
	 *	ENOMEM: not enough VM resources available to create image.
	 *
	 * Documented 4.3-tahoe exec errors:
	 *	ELOOP: namei() failure
	 *	EFAULT: format is okay but file is truncated; path, argv or
	 *		envp points to an illegal address
	 *	EIO: error reading image from secondary storage
	 *
	 * Some of the basic operation of execve() is taken from section
	 * 5.8 of the BSD book.  Much of the overhead of dealing with
	 * the old Joy VM is handled automatically by the Mach VM,
	 * which provides some simplification.  Caching of text structures
	 * is less important with Mach VM because sharing is accomplished
	 * automatically when processes page from the same vnode.  When
	 * we introduce shared libraries, non-paged executables may become
	 * more common and then text caching may become more valuable.
	 *
	 * Mike Hibler says that his VM system does not share memory over
	 * vfork() -- the only difference between vfork() and fork() is
	 * that vfork() forces the parent to wait for the child to exec().
	 * Given this, we're better off not 'supporting' vfork() at all
	 * and making it an alias for fork().
	 */

	MALLOC(xp, struct text *, sizeof *xp, M_TEMP, M_WAITOK);
	xp->x_flags = 0;
	xp->x_path = uap->path;
	xp->x_vnode = 0;
	xp->x_header = 0;
	xp->x_stack = (char *) PS_STRINGS;
#ifdef COPY_SIGCODE
	xp->x_stack -= roundup(szsigcode, sizeof (double));
#endif
	xp->x_stack_top = 0;
	xp->x_strings = 0;
	xp->x_string_size = 0;
	xp->x_proc = p;
	xp->x_arg_count = 0;
	xp->x_args = 0;
	xp->x_load_commands = 0;

	if (error = exec_lookup(xp)) {
		FREE(xp, M_TEMP);
		return (error);
	}

	if (error = exec_gather_arguments(xp, uap->argv, uap->envp)) {
		delete_text(xp);
		return (error);
	}

	if (error = analyze_exec_header(xp)) {
		delete_text(xp);
		return (error);
	}

	if (vm_deallocate(kernel_map, (vm_offset_t) xp->x_header, CLBYTES))
		panic("execve deallocate header");
	xp->x_header = 0;

	if ((xp->x_flags & X_ENTRY) == 0) {
		delete_text(xp);
		return (ENOEXEC);
	}

	/*
	 * We're committed to the new image...
	 * Let's change our name.
	 */
	exec_new_pcomm(p, uap->path);

	/*
	 * Destroy the old address space.
	 * We would prefer to create a new one and switch to it,
	 * since that would give us the ability to report errors
	 * in the original address space,
	 * but it's complex and expensive so we punt.
	 */
	map = &p->p_vmspace->vm_map;
	if (vm_map_remove(map, VM_MIN_ADDRESS, VM_MAXUSER_ADDRESS))
		panic("execve deallocate old address space");

	/*
	 * Execute the load commands.
	 * Doug Orr's user mode loader for Mach 3 was the inspiration here,
	 * although this code is considerably cruder.
	 */
	for (lp = xp->x_load_commands; error == 0 && lp; lp = lp->el_next) {
		switch (lp->el_op) {
		case EXEC_ZERO:
			error = exec_zero_segment(p, lp);
			break;
		case EXEC_CLEAR:
			error = exec_clear_segment(p, lp);
			break;
		case EXEC_MAP:
			error = exec_map_segment(p, lp);
			break;
		case EXEC_READ:
			error = exec_read_segment(p, lp);
			break;
		}
	}

	p->p_vmspace->vm_tsize = xp->x_tsize;
	p->p_vmspace->vm_dsize = xp->x_dsize;
	p->p_vmspace->vm_ssize = btoc(INITSSIZ);
	p->p_vmspace->vm_taddr = xp->x_taddr;
	p->p_vmspace->vm_daddr = xp->x_daddr;
	/* XXX ASSUMES STACK GROWS DOWN */
	p->p_vmspace->vm_maxsaddr = (caddr_t)USRSTACK - maxssiz;

	/*
	 * The only place to report errors is our parent.
	 * A segmentation violation seems like an appropriate way
	 * to report an error loading a segment...
	 */
	if (error) {
		psignal(p, SIGSEGV);
		delete_text(xp);
		return (error);
	}

	if (error = exec_install_arguments(xp)) {
		psignal(p, SIGSEGV);
		delete_text(xp);
		return (error);
	}

	/*
	 * Certain resources change state on exec.
	 * We reset caught signals to default behavior, and
	 * we close files that set the descriptor flag FD_CLOEXEC.
	 * Profiling is disabled, as the profiling buffer is gone.
	 * If the new file is setuid or setgid, we change credentials.
	 * Various vmspace parameters need adjustment.
	 */

	execsigs(p);

	exec_close_files(p);

	p->p_stats->p_prof.pr_scale = 0;

	if (xp->x_flags & X_SET_GID && cr->cr_groups[0] != xp->x_gid) {
#ifdef KTRACE
		if (p->p_tracep && !groupmember(xp->x_gid, cr) &&
		    suser(cr, &p->p_acflag)) {
			p->p_tracep = NULL;
			p->p_traceflag = 0;
			vrele(tracep);
		}
#endif
		p->p_ucred = cr = crcopy(cr);
		cr->cr_groups[0] = xp->x_gid;
	}
	if (xp->x_flags & X_SET_UID && cr->cr_uid != xp->x_uid) {
#ifdef KTRACE
		if (p->p_tracep && suser(cr, &p->p_acflag)) {
			p->p_tracep = NULL;
			p->p_traceflag = 0;
			vrele(tracep);
		}
#endif
		p->p_ucred = cr = crcopy(cr);
		cr->cr_uid = xp->x_uid;
	}
	p->p_cred->p_svuid = cr->cr_uid;
	p->p_cred->p_svgid = cr->cr_groups[0];

	/*
	 * Set up initial register state for executing in the new image.
	 * This also handles any other OS-emulation-dependent stuff.
	 * This is a machine-dependent function.
	 */
	exec_set_state(xp);

	delete_text(xp);

	if (p->p_flag & SPPWAIT) {
		p->p_flag &= ~SPPWAIT;
		wakeup((caddr_t) p->p_pptr);
	}

	return (0);
}

/*
 * Look up the exec pathname and determine whether it's executable.
 * x_proc, x_path and x_flags are assumed to be set on entry.
 * x_vnode and x_vattr are set after a successful return,
 * and we map the first page at x_header.
 */
int
exec_lookup(xp)
	struct text *xp;
{
	struct nameidata nd;
	struct nameidata *ndp = &nd;
	struct proc *p = xp->x_proc;
	struct ucred *cr = p->p_ucred;
	vm_offset_t addr;
	int error = 0;

#ifdef DEBUG
	if (xp->x_vnode)
		panic("exec_lookup vnode exists");
#endif

	/*
	 * Look up the pathname.
	 * Return it locked; this blocks other execs of this file.
	 */
	ndp->ni_nameiop = LOOKUP | FOLLOW | LOCKLEAF;
	ndp->ni_segflg = xp->x_flags & X_PATH_SYSSPACE ?
	    UIO_SYSSPACE : UIO_USERSPACE;
	ndp->ni_dirp = xp->x_path;
	if (error = namei(ndp, p))
		return (error);

	/*
	 * Test the file and see if it's executable.
	 */
	if (ndp->ni_vp->v_type != VREG)
		/*
		 * XXX POSIX.1 says to return ENOENT for empty path,
		 * or empty file for execlp()/execvp(),
		 * but it's not clear how to do this right in the kernel.
		 */
		error = EACCES;
	if (!error)
		error = VOP_ACCESS(ndp->ni_vp, VEXEC, cr, p);
	if (!error)
		error = VOP_GETATTR(ndp->ni_vp, &xp->x_vattr, cr, p);
	if (!error && cr->cr_uid == 0 &&
	    (xp->x_vattr.va_mode & (VEXEC|VEXEC>>3|VEXEC>>6)) == 0)
		/*
		 * Root execute requires at least one exec bit in the mode,
		 * so saith POSIX.1 section 2.3.2.
		 */
		error = EACCES;
	if (!error && ndp->ni_vp->v_mount->mnt_flag & MNT_NOEXEC)
		error = EACCES;

	if (!error)
		xp->x_vnode = ndp->ni_vp;
	else
		vput(ndp->ni_vp);

	if (!error)
		error = vm_allocate(kernel_map, &addr, CLBYTES, 1);
	if (!error) {
		xp->x_header = (char *) addr;
		error = vm_mmap(kernel_map, (vm_offset_t *) &xp->x_header,
		    CLBYTES, VM_PROT_READ, VM_PROT_READ,
		    MAP_FILE|MAP_COPY|MAP_FIXED,
		    (caddr_t) xp->x_vnode, (off_t) 0);
	}

	return (error);
}

/*
 * Free a text structure and any associated data structures.
 */
void
delete_text(xp)
	struct text *xp;
{
	struct exec_arg *eap, *ea_last;
	struct exec_load *lp, *lp_last;

	if (xp->x_vnode)
		vput(xp->x_vnode);
	if (xp->x_strings &&
	    vm_deallocate(kernel_map, (vm_offset_t) xp->x_strings, ARG_MAX))
		panic("delete_text strings");		/* XXX */
	if (xp->x_header &&
	    vm_deallocate(kernel_map, (vm_offset_t) xp->x_header, CLBYTES))
		panic("delete_text header");		/* XXX */
	for (eap = xp->x_args; eap; eap = ea_last) {
		ea_last = eap->ea_next;
		FREE(eap, M_TEMP);
	}
	for (lp = xp->x_load_commands; lp; lp = lp_last) {
		if (lp->el_vnode)
			vrele(lp->el_vnode);
		lp_last = lp->el_next;
		FREE(lp, M_TEMP);
	}
	if (xp->x_path && xp->x_flags & X_PATH_SYSSPACE)
		FREE(xp->x_path, M_TEMP);
	FREE(xp, M_TEMP);
}

/*
 * Collect argument and environment strings and stash them temporarily
 * in the text structure for this executable.
 * This is convenient because it means that we can pass the strings to
 * the header format analysis routines, but if we cache texts,
 * this forces us to lock out other execs of the same file for a while.
 */
int
exec_gather_arguments(xp, argv, envp)
	struct text *xp;
	char **argv;
	char **envp;
{
	vm_offset_t string_base = 0;
	char *out;
	char *in, **inv = argv;
	struct exec_arg *eap, *ea_last;
	int space_left = ARG_MAX;
	int first_time = 1;
	u_int len = 0;
	int error;

	/*
	 * We copy strings twice: once to get them out of the dying
	 * address space and to compact them, and once to put them
	 * into the new address space and adjust them to the end
	 * of the stack.  The second copy is unnecessary on a
	 * machine where the stack grows up; we can just move the
	 * kernel's page into the new address space.  Wish there
	 * was an equally easy way to handle downward-growing stacks...
	 */

	if (error = vm_allocate(kernel_map, &string_base, ARG_MAX, 1))
		return (error);
	xp->x_strings = out = (char *) string_base;

	eap = ea_last = 0;
	while ((error = copyin(inv++, &in, sizeof in)) == 0 &&
	    (in == 0 || (error = copyinstr(in, out, space_left, &len)) == 0)) {
		/* XXX am I guaranteed not to lose here? */
		MALLOC(eap, struct exec_arg *, sizeof *eap, M_TEMP, M_WAITOK);
		eap->ea_string = in ? out : 0;
		eap->ea_next = 0;
		if (ea_last)
			ea_last->ea_next = eap;
		else
			xp->x_args = eap;
		++xp->x_arg_count;
		if ((space_left -= (int)len) <= 0) {
			error = E2BIG;
			break;
		}
		if (in == 0) {
			/* Switch over from argv to envp, or quit. */
			if (first_time == 0)
				break;
			first_time = 0;
			if (envp == 0)
				--inv;
			else
				inv = envp;
		}
		out += len;
		len = 0;
		ea_last = eap;
	}
	if (error == ENAMETOOLONG)	/* XXX from copyinstr() */
		error = E2BIG;

	xp->x_string_size = out - xp->x_strings;
	return (error);
}

/*
 * Copy saved arguments and environment out to the user stack.
 * ASSUMES THAT STACKS GROW DOWN (sorry, Bob).
 * (If stacks grow up, we just move the strings page into the user map!)
 * If we cache text structures, we need to free arg structs here.
 */
int
exec_install_arguments(xp)
	struct text *xp;
{
	struct exec_arg *eap;
	char **vector;
	char **v;
	int veclen = xp->x_arg_count * sizeof (char *);
	int size = roundup(xp->x_string_size, sizeof (char *));
	char *addr = xp->x_stack - size;
	int error;

	if (error = copyout(xp->x_strings, addr, size))
		return (error);

	MALLOC(vector, char **, veclen, M_TEMP, M_WAITOK);
	v = vector;

	for (eap = xp->x_args; eap; eap = eap->ea_next)
		if (eap->ea_string)
			*v++ = addr + (eap->ea_string - xp->x_strings);
		else
			*v++ = 0;

	error = copyout(vector, addr - veclen, veclen);

	xp->x_stack_top = addr - veclen;

	FREE(vector, M_TEMP);
	return (error);
}

/*
 * Change our name.
 */
void
exec_new_pcomm(p, path)
	struct proc *p;
	char *path;
{
	u_int len;
	char *cp;
	char pathtmp[MAXPATHLEN];

	if (copyinstr(path, pathtmp, MAXPATHLEN, &len) == 0) {
		for (cp = &pathtmp[len - 1];
		     cp > pathtmp && *cp != '/';
		     --cp)
			;
		if (*cp == '/')
			++cp;
		len -= cp - pathtmp;
		/* Note: p_comm has dimension MAXCOMLEN + 1 */
		if (len > MAXCOMLEN) {
			len = MAXCOMLEN;
			p->p_comm[MAXCOMLEN] = '\0';
		}
		bcopy(cp, p->p_comm, len);
	} else
		bcopy("???", p->p_comm, 4);
}

/*
 * Indicate that a vnode is a binary that we will execute from.
 * This sets the page cache policy and takes a reference to the vnode,
 * so that it won't be purged while there's a possibility that
 * its pager will be needed to service a page fault.
 * XXX VTEXT HAS TWO MEANINGS => page cache policy + may take page fault
 */
static void
vtext(vp)
	struct vnode *vp;
{

	if ((vp->v_flag & VTEXT) == 0) {
		vp->v_flag |= VTEXT;
#ifdef notdef		/* not currently correct */
		VREF(vp);
#endif
	}
}

/*
 * Load a zero-fill-on-demand segment.
 */
int
exec_zero_segment(p, lp)
	struct proc *p;
	struct exec_load *lp;
{
	vm_offset_t addr = lp->el_address;
	vm_map_t map = &p->p_vmspace->vm_map;
	size_t len = roundup(lp->el_length, CLBYTES);
	int error;

	if (error = vm_allocate(map, &addr, len, 0))
		return (error);

	if (error = vm_map_protect(map, addr, addr + len,
	    lp->el_prot, 0))
		if (vm_deallocate(map, addr, len))
			panic("exec_zero_segment deallocate");

	if (lp->el_attr & MAP_COPY)
		if (vm_map_inherit(map, addr, addr + len,
				   VM_INHERIT_COPY))
			panic("exec_zero_segment vm_inherit_copy");

	return (error);
}

/*
 * Clear a part of a page.
 * Used with exec formats which don't clear space for BSS
 * at the end of the last data page in the binary file.
 * We assume that the data segment has already been mapped COW.
 * In theory it could be faster to double-map the page and
 * clear the bits once, but there's always less than a page of zeroes
 * so the overhead may balance out.
 */
int
exec_clear_segment(p, lp)
	struct proc *p;
	struct exec_load *lp;
{
	char *zeroes;
	int error;

	MALLOC(zeroes, char *, lp->el_length, M_TEMP, M_WAITOK);
	bzero(zeroes, lp->el_length);
	error = copyout(zeroes, (void *)lp->el_address, lp->el_length);
	FREE(zeroes, M_TEMP);
	return (error);
}

/*
 * Load a segment by arranging to load on demand from a file.
 * The address must have been previously vm_allocate()d,
 * normally using exec_zero_segment().
 */
int
exec_map_segment(p, lp)
	struct proc *p;
	struct exec_load *lp;
{
	vm_offset_t addr = lp->el_address;
	vm_map_t map = &p->p_vmspace->vm_map;
	size_t len = roundup(lp->el_length, CLBYTES);
	int error;

	if (lp->el_vnode == 0)
		return (ENOEXEC);

	if (error = vm_mmap(map, &addr, len, lp->el_prot, VM_PROT_ALL,
	    MAP_FILE|MAP_FIXED|lp->el_attr, (caddr_t) lp->el_vnode,
	    lp->el_offset))
		if (vm_deallocate(map, addr, len))
			panic("exec_map_segment deallocate");
	if (!error && addr != lp->el_address)
		panic("exec_map_segment address");

	/*
	 * Indicate to the page cache that
	 * we may execute pages from this vnode.
	 * This acquires a reference to the vnode.
	 */
	vtext(lp->el_vnode);

	return (error);
}

/*
 * Load a segment by reading data into anonymous memory.
 * The address must have been previously vm_allocate()d.
 */
int
exec_read_segment(p, lp)
	struct proc *p;
	struct exec_load *lp;
{
	vm_offset_t addr = lp->el_address;
	vm_map_t map = &p->p_vmspace->vm_map;
	size_t len = roundup(lp->el_length, CLBYTES);
	int resid = 0;
	int error;

	if (lp->el_vnode == 0)
		return (ENOEXEC);

	error = vn_rdwr(UIO_READ, lp->el_vnode, (caddr_t) lp->el_address,
	    lp->el_length, lp->el_offset, UIO_USERSPACE,
	    (IO_UNIT|IO_NODELOCKED), p->p_ucred, &resid, p);

	if (!error && resid)
		error = ENOEXEC;

	if (!error)
		error = vm_map_protect(map, addr, addr + len, lp->el_prot, 0);

	if (!error && lp->el_attr & MAP_COPY)
		if (vm_map_inherit(map, addr, addr + len, VM_INHERIT_COPY))
			panic("exec_read_segment vm_inherit_copy");

	if (error)
		if (vm_deallocate(map, addr, len))
			panic("exec_read_segment deallocate");

	return (error);
}

/*
 * Implement close-on-exec feature.
 */
void
exec_close_files(p)
	struct proc *p;
{
	struct file *fp;
	struct filedesc *fdp = p->p_fd;
	char *offp = fdp->fd_ofileflags;
	int i;
	int lastfile = 0, freefile = -1;

	if (fdp->fd_nfiles == 0)
		return;

	for (i = 0; i < fdp->fd_nfiles; ++i)
		if (fp = fdp->fd_ofiles[i]) {
			if (offp[i] & UF_EXCLOSE) {
				if (offp[i] & UF_MAPPED)
					(void) munmapfd(p, i);
				offp[i] = 0;
				fdp->fd_ofiles[i] = 0;
				closef(fp, p);
				if (freefile < 0)
					freefile = i;
			} else
				lastfile = i;
		}
	if (lastfile < fdp->fd_lastfile)
		fdp->fd_lastfile = lastfile;
	if (freefile >= 0 && freefile < fdp->fd_freefile)
		fdp->fd_freefile = freefile;
}

/*
 * A machine-independent function for 'loading' interpreted files,
 * called from the machine-dependent header analysis code.
 * The header should indicate the name of the program to load.
 * We return EAGAIN to indicate that the analysis code should make another
 * pass, or errno if we failed to process the script.
 * We can't return 0 ('success') because a script never loads anything.
 */
int
exec_interpreter(xp)
	struct text *xp;
{
	struct exec_arg *eap, *ea_last;
	char *p = xp->x_header, *p2;
	char *s = xp->x_strings + xp->x_string_size;
	char *path = 0;
	/* XXX how much header should we read? */
	char *lastp = p + (xp->x_size >= CLBYTES ? CLBYTES : xp->x_size);
	int space_left = ARG_MAX - xp->x_string_size;
	u_int len;
	int error;

	/*
	 * Find the end of the (variable length) header.
	 */
	for (; p < lastp && *p != '\n'; ++p)
		;
	if (p >= lastp) {
		if (p >= xp->x_header + xp->x_size)
			return (ENOEXEC);
		return (E2BIG);
	}
	lastp = p;

	/*
	 * Stick the original exec pathname into the argument list.
	 * This becomes an argument to the interpreter.
	 */
	len = 0;
	if (xp->x_flags & X_PATH_SYSSPACE)
		error = copystr(xp->x_path, s, space_left, &len);
	else
		error = copyinstr(xp->x_path, s, space_left, &len);
	if ((space_left -= (int) (len + sizeof (char *))) < 0)
		error = E2BIG;
	if (error) {
		if (error == ENOENT)
			error = E2BIG;
		return (error);
	}
	/* If there is no argv[0], make one. */
	if (xp->x_args->ea_string == 0) {
		MALLOC(eap, struct exec_arg *, sizeof *eap, M_TEMP, M_WAITOK);
		eap->ea_next = xp->x_args;
		xp->x_args = eap;
		++xp->x_arg_count;
	}
	/* Stomp on the original argv[0] with the current filename. */
	xp->x_args->ea_string = s;
	s += len;
	*s++ = '\0';

	/*
	 * Scan the header looking for arguments.
	 * Prepend these arguments to the ones we've already seen.
	 */
	ea_last = 0;
	for (p = xp->x_header + 1; p < lastp; p = p2) {
		for (++p; p < lastp && isblank(*p); ++p)
			;
		if (p >= lastp)
			break;
		for (p2 = p + 1; p2 < lastp && !isblank(*p2); ++p2)
			;
		len = p2 - p;
		if ((space_left -= (int) (len + 1)) < 0) {
			error = E2BIG;
			break;
		}
		bcopy(p, s, len);

		MALLOC(eap, struct exec_arg *, sizeof *eap, M_TEMP, M_WAITOK);
		eap->ea_string = s;
		if (ea_last == 0) {
			eap->ea_next = xp->x_args;
			xp->x_args = eap;

			/* XXX Could we just pass a reference? */
			MALLOC(path, char *, len + 1, M_TEMP, M_WAITOK);
			bcopy(p, path, len);
			path[len] = '\0';
		} else {
			eap->ea_next = ea_last->ea_next;
			ea_last->ea_next = eap;
		}
		ea_last = eap;
		++xp->x_arg_count;

		s += len;
		*s++ = '\0';
	}
	xp->x_string_size = s - xp->x_strings;
	if (path == 0)
		return (ENOEXEC);
	if (error) {
		FREE(path, M_TEMP);
		return (error);
	}

	if (vm_deallocate(kernel_map, (vm_offset_t) xp->x_header, CLBYTES))
		panic("exec_interpreter deallocate header");
	xp->x_header = 0;
	vput(xp->x_vnode);
	xp->x_vnode = 0;
	if (xp->x_flags & X_PATH_SYSSPACE)
		FREE(xp->x_path, M_TEMP);

	xp->x_path = path;
	xp->x_flags |= X_PATH_SYSSPACE;

	if (error = exec_lookup(xp))
		return (error);

	return (EAGAIN);
}

/*
 * A machine-independent function for loading BSD demand-paged executables.
 * Every current BSD architecture supports this format;
 * the contents of the useful header fields differs only in byte order.
 * We assume host byte order (too bad for transexual MIPS programs).
 */
int
exec_demand_load_binary(xp)
	struct text *xp;
{
	struct exec *ep;
	struct exec_load *lp, *lp_last;

	/*
	 * Get a handle on the exec header, and check it for consistency.
	 * XXX Does the system zero-fill incomplete pages?
	 * XXX Assumes power-of-two cluster size.
	 */
	if (xp->x_size < CLBYTES)
		return (ENOEXEC);
	ep = (struct exec *) xp->x_header;
	if (ep->a_text == 0 ||
	    xp->x_size < roundup(ep->a_text, CLBYTES) + CLBYTES)
		return (ENOEXEC);
	if (ep->a_data &&
	    xp->x_size < ep->a_data + roundup(ep->a_text, CLBYTES) + CLBYTES)
		return (ENOEXEC);
	exec_set_entry(xp, ep->a_entry);

	/*
	 * Generate load commands for text, data, bss and stack.
	 * We start by reserving an area for text, data and bss.
	 */
	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	xp->x_load_commands = lp;
	lp->el_op = EXEC_ZERO;
	lp->el_vnode = 0;
	lp->el_offset = 0;
	lp->el_address = VM_MIN_ADDRESS;
	lp->el_length = roundup(ep->a_text, CLBYTES) +
	    roundup(ep->a_data + ep->a_bss, CLBYTES);
	lp->el_prot = VM_PROT_ALL;
	lp->el_attr = MAP_COPY;
	lp_last = lp;

	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	lp->el_op = EXEC_MAP;
	lp->el_vnode = xp->x_vnode;
	VREF(lp->el_vnode);
	lp->el_offset = CLBYTES;
	lp->el_address = VM_MIN_ADDRESS;
	xp->x_taddr = (caddr_t) lp->el_address;
	lp->el_length = ep->a_text;
	xp->x_tsize = howmany(lp->el_length, CLBYTES);
	lp->el_prot = VM_PROT_READ|VM_PROT_EXECUTE;
	lp->el_attr = MAP_COPY;
	lp_last->el_next = lp;
	lp_last = lp;

	if (ep->a_data > 0) {
		MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
		lp->el_op = EXEC_MAP;
		lp->el_vnode = xp->x_vnode;
		VREF(lp->el_vnode);
		lp->el_offset = CLBYTES + ep->a_text;
		lp->el_address = VM_MIN_ADDRESS + ep->a_text;
		xp->x_daddr = (caddr_t) lp->el_address;
		lp->el_length = ep->a_data;
		xp->x_dsize = howmany(lp->el_length + ep->a_bss, CLBYTES);
		lp->el_prot = VM_PROT_ALL;
		lp->el_attr = MAP_COPY;
		lp_last->el_next = lp;
		lp_last = lp;
	}

	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	lp->el_op = EXEC_ZERO;
	lp->el_vnode = 0;
	lp->el_offset = 0;
	lp->el_address = (vm_offset_t) USRSTACK - INITSSIZ;
	lp->el_length = INITSSIZ;
	lp->el_prot = VM_PROT_ALL;
	lp->el_attr = MAP_COPY;
	lp_last->el_next = lp;
	lp->el_next = 0;

	return (0);
}

/*
 * By popular demand, a new compact demand load binary format.
 * Two features: the first page is mapped out, and the header is in the text.
 */
int
exec_compact_demand_load_binary(xp)
	struct text *xp;
{
	struct exec *ep;
	struct exec_load *lp, *lp_last;

	/*
	 * Get a handle on the exec header, and check it for consistency.
	 * XXX Assumes power-of-two cluster size.
	 */
	if (xp->x_size < CLBYTES)
		return (ENOEXEC);
	ep = (struct exec *) xp->x_header;
	if (ep->a_text == 0 || xp->x_size < roundup(ep->a_text, CLBYTES))
		return (ENOEXEC);
	if (ep->a_data &&
	    xp->x_size < ep->a_data + roundup(ep->a_text, CLBYTES))
		return (ENOEXEC);
	exec_set_entry(xp, ep->a_entry);

	/*
	 * Generate load commands for text, data, bss and stack.
	 * We start by reserving an area for text, data and bss.
	 */
	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	xp->x_load_commands = lp;
	lp->el_op = EXEC_ZERO;
	lp->el_vnode = 0;
	lp->el_offset = 0;
	lp->el_address = VM_MIN_ADDRESS + CLBYTES;
	lp->el_length = roundup(ep->a_text, CLBYTES) +
	    roundup(ep->a_data + ep->a_bss, CLBYTES);
	lp->el_prot = VM_PROT_ALL;
	lp->el_attr = MAP_COPY;
	lp_last = lp;

	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	lp->el_op = EXEC_MAP;
	lp->el_vnode = xp->x_vnode;
	VREF(lp->el_vnode);
	lp->el_offset = 0;
	lp->el_address = VM_MIN_ADDRESS + CLBYTES;
	xp->x_taddr = (caddr_t) lp->el_address;
	lp->el_length = ep->a_text;
	xp->x_tsize = howmany(lp->el_length, CLBYTES);
	lp->el_prot = VM_PROT_READ|VM_PROT_EXECUTE;
	lp->el_attr = MAP_COPY;
	lp_last->el_next = lp;
	lp_last = lp;

	if (ep->a_data > 0) {
		MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
		lp->el_op = EXEC_MAP;
		lp->el_vnode = xp->x_vnode;
		VREF(lp->el_vnode);
		lp->el_offset = ep->a_text;
		lp->el_address = VM_MIN_ADDRESS + CLBYTES + ep->a_text;
		xp->x_daddr = (caddr_t) lp->el_address;
		lp->el_length = ep->a_data;
		xp->x_dsize = howmany(lp->el_length + ep->a_bss, CLBYTES);
		lp->el_prot = VM_PROT_ALL;
		lp->el_attr = MAP_COPY;
		lp_last->el_next = lp;
		lp_last = lp;
	}

	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	lp->el_op = EXEC_ZERO;
	lp->el_vnode = 0;
	lp->el_offset = 0;
	lp->el_address = (vm_offset_t) USRSTACK - INITSSIZ;
	lp->el_length = INITSSIZ;
	lp->el_prot = VM_PROT_ALL;
	lp->el_attr = MAP_COPY;
	lp_last->el_next = lp;
	lp->el_next = 0;

	return (0);
}

/*
 * Not demand paged -- we load data from the file into anonymous memory.
 */
int
exec_shared_binary(xp)
	struct text *xp;
{
	struct exec *ep;
	struct exec_load *lp, *lp_last;

	/*
	 * XXX We need some way of sharing the anonymous memory object that
	 * contains the text of the binary after loading.
	 * Maybe we can make a peculiar vnode for it?
	 */
	if (xp->x_size < sizeof *ep)
		return (ENOEXEC);
	ep = (struct exec *) xp->x_header;
	if (ep->a_text == 0 || xp->x_size < ep->a_text + sizeof *ep)
		return (ENOEXEC);
	if (ep->a_data &&
	    xp->x_size < ep->a_data + ep->a_text + sizeof *ep)
		return (ENOEXEC);
	exec_set_entry(xp, ep->a_entry);

	/*
	 * Arrange to read text and data into memory.
	 */
	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	xp->x_load_commands = lp;
	lp->el_op = EXEC_ZERO;
	lp->el_vnode = 0;
	lp->el_offset = 0;
	lp->el_address = VM_MIN_ADDRESS;
	lp->el_length = roundup(ep->a_text, CLBYTES) +
	    roundup(ep->a_data + ep->a_bss, CLBYTES);
	lp->el_prot = VM_PROT_ALL;
	lp->el_attr = MAP_COPY;
	lp_last = lp;

	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	lp->el_op = EXEC_READ;
	lp->el_vnode = xp->x_vnode;
	VREF(lp->el_vnode);
	lp->el_offset = sizeof *ep;
	lp->el_address = VM_MIN_ADDRESS;
	xp->x_taddr = (caddr_t) lp->el_address;
	lp->el_length = ep->a_text;
	xp->x_tsize = howmany(lp->el_length, CLBYTES);
	lp->el_prot = VM_PROT_READ|VM_PROT_EXECUTE;
	lp->el_attr = MAP_COPY;
	lp_last->el_next = lp;
	lp_last = lp;

	if (ep->a_data > 0) {
		MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
		lp->el_op = EXEC_READ;
		lp->el_vnode = xp->x_vnode;
		VREF(lp->el_vnode);
		lp->el_offset = ep->a_text + sizeof *ep;
		lp->el_address = VM_MIN_ADDRESS + roundup(ep->a_text, CLBYTES);
		xp->x_daddr = (caddr_t) lp->el_address;
		lp->el_length = ep->a_data;
		xp->x_dsize = howmany(lp->el_length + ep->a_bss, CLBYTES);
		lp->el_prot = VM_PROT_ALL;
		lp->el_attr = MAP_COPY;
		lp_last->el_next = lp;
		lp_last = lp;
	}

	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	lp->el_op = EXEC_ZERO;
	lp->el_vnode = 0;
	lp->el_offset = 0;
	lp->el_address = (vm_offset_t) USRSTACK - INITSSIZ;
	lp->el_length = INITSSIZ;
	lp->el_prot = VM_PROT_ALL;
	lp->el_attr = MAP_COPY;
	lp_last->el_next = lp;
	lp->el_next = 0;

	return (0);
}

/*
 * Not sharable -- text and data are loaded together and are writable.
 */
int
exec_unshared_binary(xp)
	struct text *xp;
{
	struct exec *ep;
	struct exec_load *lp, *lp_last;

	/*
	 * The very crudest type of binary...
	 */
	if (xp->x_size < sizeof *ep)
		return (ENOEXEC);
	ep = (struct exec *) xp->x_header;
	if (xp->x_size < ep->a_data + ep->a_text + sizeof *ep)
		return (ENOEXEC);
	exec_set_entry(xp, ep->a_entry);
	xp->x_tsize = 0;
	xp->x_taddr = VM_MIN_ADDRESS;

	/*
	 * Arrange for the single text+data segment to be read in.
	 */
	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	xp->x_load_commands = lp;
	lp->el_op = EXEC_ZERO;
	lp->el_vnode = 0;
	lp->el_offset = 0;
	lp->el_address = VM_MIN_ADDRESS;
	lp->el_length = ep->a_text + ep->a_data + ep->a_bss;
	lp->el_prot = VM_PROT_ALL;
	lp->el_attr = MAP_COPY;
	lp_last = lp;

	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	lp->el_op = EXEC_READ;
	lp->el_vnode = xp->x_vnode;
	VREF(lp->el_vnode);
	lp->el_offset = sizeof *ep;
	lp->el_address = VM_MIN_ADDRESS;
	xp->x_daddr = (caddr_t) lp->el_address;
	lp->el_length = ep->a_text + ep->a_data;
	xp->x_dsize = howmany(lp->el_length + ep->a_bss, CLBYTES);
	lp->el_prot = VM_PROT_ALL;
	lp->el_attr = MAP_COPY;
	lp_last->el_next = lp;
	lp_last = lp;

	MALLOC(lp, struct exec_load *, sizeof *lp, M_TEMP, M_WAITOK);
	lp->el_op = EXEC_ZERO;
	lp->el_vnode = 0;
	lp->el_offset = 0;
	lp->el_address = (vm_offset_t) USRSTACK - INITSSIZ;
	lp->el_length = INITSSIZ;
	lp->el_prot = VM_PROT_ALL;
	lp->el_attr = MAP_COPY;
	lp_last->el_next = lp;
	lp->el_next = 0;

	return (0);
}

/*
 * Set the entry address for the new image.
 * If we've already done this, ignore the new request.
 * Our obscure rule for setuid/setgid behavior: the first file which
 * sets the entry address is the one which determines setuid/setgid.
 * This prevents interpreted scripts (which don't set the entry point)
 * and shared libraries (which try to set it redundantly) from
 * affecting setuid behavior.
 */
void
exec_set_entry(xp, entry)
	struct text *xp;
	u_long entry;
{

	/*
	 * If we already have an entry point, don't change it.
	 */
	if (xp->x_flags & X_ENTRY)
		return;

	xp->x_entry = entry;
	xp->x_flags |= X_ENTRY;

	if (xp->x_proc->p_flag & STRC ||
	    xp->x_vnode->v_mount->mnt_flag & MNT_NOSUID)
		return;

	if (xp->x_vattr.va_mode & VSUID) {
		xp->x_uid = xp->x_vattr.va_uid;
		xp->x_flags |= X_SET_UID;
	}

	if (xp->x_vattr.va_mode & VSGID) {
		xp->x_gid = xp->x_vattr.va_gid;
		xp->x_flags |= X_SET_GID;
	}
}
