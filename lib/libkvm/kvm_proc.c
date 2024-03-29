/*	BSDI $Id: kvm_proc.c,v 1.4 1993/11/12 18:25:02 donn Exp $	*/

/*-
 * Copyright (c) 1989 The Regents of the University of California.
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
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)kvm_proc.c	5.24 (Berkeley) 5/18/92";
#endif /* LIBC_SCCS and not lint */

/*
 * Proc traversal interface for kvm.  ps and w are (probably) the exclusive
 * users of this code, so we've factored it out into a separate module.
 * Thus, we keep this grunge out of the other kvm applications (i.e.,
 * most other applications are interested only in open/close/read/nlist).
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <nlist.h>
#include <kvm.h>
#include <string.h>
#include <unistd.h>

#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/swap_pager.h>

#include <sys/kinfo.h>
#include <sys/kinfo_proc.h>

#include <limits.h>
#include <db.h>
#include <paths.h>

#include "kvm_private.h"

static char *
kvm_readswap(kd, p, va, cnt)
	kvm_t *kd;
	const struct proc *p;
	u_long va;
	u_long *cnt;
{
	register int ix;
	register u_long addr, head;
	register u_long offset, pagestart, sbstart, pgoff;
	register off_t seekpoint;
	struct vm_map_entry vme;
	struct vm_object vmo;
	struct pager_struct pager;
	struct swpager swap;
	struct swblock swb;
	static char page[NBPG];

	head = (u_long)&p->p_vmspace->vm_map.header;
	/*
	 * Look through the address map for the memory object
	 * that corresponds to the given virtual address.
	 * The header just has the entire valid range.
	 */
	addr = head;
	while (1) {
		if (kvm_read(kd, addr, (char *)&vme, sizeof(vme)) != 
		    sizeof(vme))
			return (0);

		if (va >= vme.start && va <= vme.end && 
		    vme.object.vm_object != 0)
			break;

		addr = (u_long)vme.next;
		if (addr == 0 || addr == head)
			return (0);
	}
	/*
	 * We found the right object -- follow shadow links.
	 */
	offset = va - vme.start + vme.offset;
	addr = (u_long)vme.object.vm_object;
	while (1) {
		if (kvm_read(kd, addr, (char *)&vmo, sizeof(vmo)) != 
		    sizeof(vmo))
			return (0);
		addr = (u_long)vmo.shadow;
		if (addr == 0)
			break;
		offset += vmo.shadow_offset;
	}
	if (vmo.pager == 0)
		return (0);

	offset += vmo.paging_offset;
	/*
	 * Read in the pager info and make sure it's a swap device.
	 */
	addr = (u_long)vmo.pager;
	if (kvm_read(kd, addr, (char *)&pager, sizeof(pager)) != sizeof(pager)
	    || pager.pg_type != PG_SWAP)
		return (0);

	/*
	 * Read in the swap_pager private data, and compute the
	 * swap offset.
	 */
	addr = (u_long)pager.pg_data;
	if (kvm_read(kd, addr, (char *)&swap, sizeof(swap)) != sizeof(swap))
		return (0);
	ix = offset / dbtob(swap.sw_bsize);
	if (swap.sw_blocks == 0 || ix >= swap.sw_nblocks)
		return (0);

	addr = (u_long)&swap.sw_blocks[ix];
	if (kvm_read(kd, addr, (char *)&swb, sizeof(swb)) != sizeof(swb))
		return (0);

	sbstart = (offset / dbtob(swap.sw_bsize)) * dbtob(swap.sw_bsize);
	sbstart /= NBPG;
	pagestart = offset / NBPG;
	pgoff = pagestart - sbstart;

	if (swb.swb_block == 0 || (swb.swb_mask & (1 << pgoff)) == 0)
		return (0);

	seekpoint = dbtob(swb.swb_block) + ctob(pgoff);
	errno = 0;
	if (lseek(kd->swfd, seekpoint, 0) == -1 && errno != 0)
		return (0);
	if (read(kd->swfd, page, sizeof(page)) != sizeof(page))
		return (0);

	offset %= NBPG;
	*cnt = NBPG - offset;
	return (&page[offset]);
}

#define KREAD(kd, addr, obj) \
	(kvm_read(kd, addr, (char *)(obj), sizeof(*obj)) != sizeof(*obj))

/*
 * Read proc's from memory file into buffer bp, which has space to hold
 * at most maxcnt procs.
 */
static int
kvm_proclist(kd, what, arg, p, bp, maxcnt)
	kvm_t *kd;
	int what, arg;
	struct proc *p;
	struct kinfo_proc *bp;
	int maxcnt;
{
	register int cnt = 0;
	struct eproc eproc;
	struct pgrp pgrp;
	struct session sess;
	struct tty tty;
	struct proc proc;

	for (; cnt < maxcnt && p != 0; p = proc.p_nxt) {
		if (KREAD(kd, (u_long)p, &proc)) {
			_kvm_err(kd, kd->program, "can't read proc at %x", p);
			return (-1);
		}
		if (KREAD(kd, (u_long)proc.p_cred, &eproc.e_pcred) == 0)
			KREAD(kd, (u_long)eproc.e_pcred.pc_ucred,
			      &eproc.e_ucred);

		switch(ki_op(what)) {
			
		case KINFO_PROC_PID:
			if (proc.p_pid != (pid_t)arg)
				continue;
			break;

		case KINFO_PROC_UID:
			if (eproc.e_ucred.cr_uid != (uid_t)arg)
				continue;
			break;

		case KINFO_PROC_RUID:
			if (eproc.e_pcred.p_ruid != (uid_t)arg)
				continue;
			break;
		}
		/*
		 * We're going to add another proc to the set.  If this
		 * will overflow the buffer, assume the reason is because
		 * nprocs (or the proc list) is corrupt and declare an error.
		 */
		if (cnt >= maxcnt) {
			_kvm_err(kd, kd->program, "nprocs corrupt");
			return (-1);
		}
		/*
		 * gather eproc
		 */
		eproc.e_paddr = p;
		if (KREAD(kd, (u_long)proc.p_pgrp, &pgrp)) {
			_kvm_err(kd, kd->program, "can't read pgrp at %x",
				 proc.p_pgrp);
			return (-1);
		}
		eproc.e_sess = pgrp.pg_session;
		eproc.e_pgid = pgrp.pg_id;
		eproc.e_jobc = pgrp.pg_jobc;
		if (KREAD(kd, (u_long)pgrp.pg_session, &sess)) {
			_kvm_err(kd, kd->program, "can't read session at %x", 
				pgrp.pg_session);
			return (-1);
		}
		if ((proc.p_flag & SCTTY) && sess.s_ttyp != NULL) {
			if (KREAD(kd, (u_long)sess.s_ttyp, &tty)) {
				_kvm_err(kd, kd->program,
					 "can't read tty at %x", sess.s_ttyp);
				return (-1);
			}
			eproc.e_tdev = tty.t_dev;
			eproc.e_tsess = tty.t_session;
			if (tty.t_pgrp != NULL) {
				if (KREAD(kd, (u_long)tty.t_pgrp, &pgrp)) {
					_kvm_err(kd, kd->program,
						 "can't read tpgrp at &x", 
						tty.t_pgrp);
					return (-1);
				}
				eproc.e_tpgid = pgrp.pg_id;
			} else
				eproc.e_tpgid = -1;
		} else
			eproc.e_tdev = NODEV;
		eproc.e_flag = sess.s_ttyvp ? EPROC_CTTY : 0;
		if (sess.s_leader == p)
			eproc.e_flag |= EPROC_SLEADER;
		if (proc.p_wmesg)
			(void)kvm_read(kd, (u_long)proc.p_wmesg, 
			    eproc.e_wmesg, WMESGLEN);

#ifdef sparc
		(void)kvm_read(kd, (u_long)&proc.p_vmspace->vm_rssize,
		    (char *)&eproc.e_vm.vm_rssize,
		    sizeof(eproc.e_vm.vm_rssize));
		(void)kvm_read(kd, (u_long)&proc.p_vmspace->vm_tsize,
		    (char *)&eproc.e_vm.vm_tsize,
		    3 * sizeof(eproc.e_vm.vm_rssize));	/* XXX */
#else
		(void)kvm_read(kd, (u_long)proc.p_vmspace,
		    (char *)&eproc.e_vm, sizeof(eproc.e_vm));
#endif
		eproc.e_xsize = eproc.e_xrssize = 0;
		eproc.e_xccount = eproc.e_xswrss = 0;

		switch (ki_op(what)) {

		case KINFO_PROC_PGRP:
			if (eproc.e_pgid != (pid_t)arg)
				continue;
			break;

		case KINFO_PROC_TTY:
			if ((proc.p_flag&SCTTY) == 0 || 
			     eproc.e_tdev != (dev_t)arg)
				continue;
			break;
		}
		bcopy((char *)&proc, (char *)&bp->kp_proc, sizeof(proc));
		bcopy((char *)&eproc, (char *)&bp->kp_eproc, sizeof(eproc));
		++bp;
		++cnt;
	}
	return (cnt);
}

/*
 * Build proc info array by reading in proc list from a crash dump.
 * Return number of procs read.  maxcnt is the max we will read.
 */
static int
kvm_deadprocs(kd, what, arg, a_allproc, a_zombproc, maxcnt)
	kvm_t *kd;
	int what, arg;
	u_long a_allproc;
	u_long a_zombproc;
	int maxcnt;
{
	register struct kinfo_proc *bp = kd->procbase;
	register int acnt, zcnt;
	struct proc *p;

	if (KREAD(kd, a_allproc, &p)) {
		_kvm_err(kd, kd->program, "cannot read allproc");
		return (-1);
	}
	acnt = kvm_proclist(kd, what, arg, p, bp, maxcnt);
	if (acnt < 0)
		return (acnt);

	if (KREAD(kd, a_zombproc, &p)) {
		_kvm_err(kd, kd->program, "cannot read zombproc");
		return (-1);
	}
	zcnt = kvm_proclist(kd, what, arg, p, bp + acnt, maxcnt - acnt);
	if (zcnt < 0)
		zcnt = 0;

	return (acnt + zcnt);
}

struct kinfo_proc *
kvm_getprocs(kd, op, arg, cnt)
	kvm_t *kd;
	int op, arg;
	int *cnt;
{
	int size, st, nprocs, i;

	if (kd->procbase != 0) {
		free((void *)kd->procbase);
		/* 
		 * Clear this pointer in case this call fails.  Otherwise,
		 * kvm_close() will free it again.
		 */
		kd->procbase = 0;
	}
	if (ISALIVE(kd)) {
		size = 0;
		st = getkerninfo(op, NULL, &size, arg);
		if (st < 0) {
			_kvm_syserr(kd, kd->program, "kvm_getprocs");
			return (0);
		}
		kd->procbase = (struct kinfo_proc *)_kvm_malloc(kd, st);
		if (kd->procbase == 0)
			return (0);
		size = st;
		st = getkerninfo(op, kd->procbase, &size, arg);
		if (st < 0) {
			_kvm_syserr(kd, kd->program, "kvm_getprocs");
			return (0);
		}
		if (size % sizeof(struct kinfo_proc) != 0) {
			_kvm_err(kd, kd->program,
				"proc size mismatch (%d total, %d chunks)",
				size, sizeof(struct kinfo_proc));
			return (0);
		}
		nprocs = size / sizeof(struct kinfo_proc);
	} else {
		struct nlist nl[4], *p;

		nl[0].n_name = "_nprocs";
		nl[1].n_name = "_allproc";
		nl[2].n_name = "_zombproc";
		nl[3].n_name = 0;

		if (kvm_nlist(kd, nl) != 0) {
			for (p = nl; p->n_type != 0; ++p)
				;
			_kvm_err(kd, kd->program,
				 "%s: no such symbol", p->n_name);
			return (0);
		}
		if (KREAD(kd, nl[0].n_value, &nprocs)) {
			_kvm_err(kd, kd->program, "can't read nprocs");
			return (0);
		}
		size = nprocs * sizeof(struct kinfo_proc);
		kd->procbase = (struct kinfo_proc *)_kvm_malloc(kd, size);
		if (kd->procbase == 0)
			return (0);

		nprocs = kvm_deadprocs(kd, op, arg, nl[1].n_value,
				      nl[2].n_value, nprocs);
#ifdef notdef
		size = nprocs * sizeof(struct kinfo_proc);
		(void)realloc(kd->procbase, size);
#endif
	}
#ifdef __i386__
	/* XXX: copy rss into place */
	for (i = 0; i < nprocs; i++)
		kd->procbase[i].kp_eproc.e_vm.vm_rssize =
		    kd->procbase[i].kp_eproc.e_vm.vm_pmap.pm_stats.resident_count;
#endif /* __i386__ */
	*cnt = nprocs;
	return (kd->procbase);
}

void
_kvm_freeprocs(kd)
	kvm_t *kd;
{
	if (kd->procbase) {
		free(kd->procbase);
		kd->procbase = 0;
	}
}

void *
_kvm_realloc(kd, p, n)
	kvm_t *kd;
	void *p;
	size_t n;
{
	void *np = (void *)realloc(p, n);

	if (np == 0)
		_kvm_err(kd, kd->program, "out of memory");
	return (np);
}

#define	CHUNK_SIZE	128

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/*
 * Read in an argument vector from the user address space of process p.
 * addr if the user-space base address of narg null-terminated contiguous 
 * strings.  This is used to read in both the command arguments and
 * environment strings.  Read at most maxcnt characters of strings.
 */
/* static */ char **
kvm_argv(kd, p, addr, narg, maxcnt)
	kvm_t *kd;
	const struct proc *p;
	register u_long addr;
	register int narg;
	register int maxcnt;
{
	register char *cp;
	register int len;
	register char **argv;
	char **xargv, **rargv, **lastargv;
	char *xp;
	unsigned int n;
	int rlen;
	int off;

	/*
	 * Check that there aren't an unreasonable number of arguments,
	 * and that the address is in user space.
	 */
	if (narg > 512 || addr < VM_MIN_ADDRESS || addr >= VM_MAXUSER_ADDRESS)
		return (0);

	if (kd->argv == 0) {
		/*
		 * Try to avoid reallocs.
		 */
		kd->argc = MAX(narg + 1, 32);
		kd->argv = (char **)_kvm_malloc(kd, kd->argc * 
						sizeof(*kd->argv));
		if (kd->argv == 0)
			return (0);
	} else if (narg + 1 > kd->argc) {
		kd->argc = MAX(2 * kd->argc, narg + 1);
		kd->argv = (char **)_kvm_realloc(kd, kd->argv, kd->argc * 
						sizeof(*kd->argv));
		if (kd->argv == 0)
			return (0);
	}
	if (kd->argspc == 0) {
		kd->argspc = (char *)_kvm_malloc(kd, NBPG);
		if (kd->argspc == 0)
			return (0);
		kd->arglen = NBPG;
	}
	cp = kd->argspc;
	lastargv = &kd->argv[narg];
	rlen = 0;
	len = 0;

	if (kvm_uread(kd, p, addr, (char *)kd->argv, narg * sizeof *argv) !=
	    narg * sizeof *argv)
		/* XXX */
		return (0);
	*lastargv = 0;

	/*
	 * Work through argv picking up strings.
	 * We suck in the original argv and convert it incrementally.
	 * We try to bundle adjacent reads for efficiency.
	 * 'xargv' is the 'index' or current argv.
	 * 'rargv' is the argv where we will start the next read.
	 * 'rlen' accumulates the count of bytes in the pending read.
	 * Note that 'len' counts the number of bytes of argument strings,
	 * not the total number of bytes in the buffer.
	 */
	for (rargv = xargv = kd->argv; xargv < lastargv; ++xargv) {

		/* should we bundle this read? */
		if (xargv < lastargv &&
		    (n = *(xargv + 1) - *xargv) < CHUNK_SIZE &&
		    (maxcnt == 0 || len + rlen + n < maxcnt * 2)) {
			rlen += n;
			continue;
		}

		rlen += CHUNK_SIZE;
		if ((vm_offset_t)(*rargv + rlen) >= VM_MAXUSER_ADDRESS)
			rlen = VM_MAXUSER_ADDRESS - (vm_offset_t)*rargv;

		if (cp + rlen >= kd->argspc + kd->arglen) {
			char **pp;
			char *op = kd->argspc;

			kd->arglen *= 2;
			kd->argspc = (char *)_kvm_realloc(kd, kd->argspc,
			    kd->arglen);
			if (kd->argspc == 0) 
				return (0);
			cp = &kd->argspc[len];
			off = kd->argspc - op;
			/* adjust argv pointers */
			for (pp = kd->argv; pp < rargv; ++pp)
				*pp += off;
		}

		addr = (u_long)*rargv;
		if (kvm_uread(kd, p, addr, cp, rlen) != rlen)
			/* XXX */
			return (0);

		/* convert argv pointers and check string lengths */
		off = cp - (char *)addr;
		for (; rargv < xargv; ++rargv) {
			if (maxcnt) {
				/* how much text must be scanned for a nul? */
				n = *(rargv + 1) - *rargv;
				if (len + n + 1 >= maxcnt)
					n = maxcnt - len;
			}
			*rargv += off;
			if (maxcnt) {
				/* scan for the nul & terminate if necessary */
				if (xp = memchr(*rargv, '\0', n))
					len += xp - *rargv + 1;
				else
					len += n;
				if (len >= maxcnt) {
					(*rargv)[n - 1] = '\0';
					*++rargv = 0;
					return (kd->argv);
				}
			}
		}

		/*
		 * We have a final string of unknown length,
		 * with the first CHUNK_SIZE bytes at the end of the buffer.
		 * Adjust its argv, then try to find its end,
		 * reading more chunks of data if necessary.
		 */
		*xargv += off;
		xp = *xargv;
		n = cp + rlen - *xargv;
		++rargv;

		for (;;) {
			cp += rlen;
			addr += rlen;
			if (maxcnt)
				len += n;
			if (xp = memchr(xp, '\0', n))
				/* found it */
				break;
			if (maxcnt && len + 1 >= maxcnt)
				break;

			rlen = CHUNK_SIZE;
			if (addr + rlen >= VM_MAXUSER_ADDRESS)
				rlen = VM_MAXUSER_ADDRESS - addr;
			if (rlen <= 0)
				break;

			if (kvm_uread(kd, p, addr, cp, rlen) != rlen)
				/* XXX */
				return (0);

			xp = cp;
			n = rlen;
		}

		/*
		 * We're here because:
		 * - we hit the end of the address space
		 * - we're checking string length and we've run over
		 * - we found the end of the string (xp points to it)
		 */
		if (rlen <= 0)
			/* strings are probably garbled */
			return (0);
		if (!xp) {
			/* didn't find the end, but went past maxcnt */
			cp[maxcnt - len - 1] = '\0';
			*++xargv = 0;
			return (kd->argv);
		}
		++xp;
		if (maxcnt) {
			len -= cp - xp;		/* fix overestimate */
			if (len + 1 >= maxcnt) {
				/* found the end and it's past the limit */
				xp[maxcnt - len - 1] = '\0';
				*++xargv = 0;
				return (kd->argv);
			}
		}

		cp = xp;
		rlen = 0;
	}

	*xargv = 0;
	return (kd->argv);
}

static void
ps_str_a(p, addr, n)
	struct ps_strings *p;
	u_long *addr;
	int *n;
{
	*addr = (u_long)p->ps_argv;
	*n = p->ps_argc;
}

static void
ps_str_e(p, addr, n)
	struct ps_strings *p;
	u_long *addr;
	int *n;
{
	*addr = (u_long)p->ps_envp;
	*n = p->ps_nenv;
}

/*
 * Determine if the proc indicated by p is still active.
 * This test is not 100% foolproof in theory, but chances of
 * being wrong are very low.
 */
static int
proc_verify(kd, kernp, p)
	kvm_t *kd;
	u_long kernp;
	const struct proc *p;
{
	struct proc kernproc;

	/*
	 * Just read in the whole proc.  It's not that big relative
	 * to the cost of the read system call.
	 */
	if (kvm_read(kd, kernp, (char *)&kernproc, sizeof(kernproc)) != 
	    sizeof(kernproc))
		return (0);
	return (p->p_pid == kernproc.p_pid &&
		(kernproc.p_stat != SZOMB || p->p_stat == SZOMB));
}

static char **
kvm_doargv(kd, kp, nchr, info)
	kvm_t *kd;
	const struct kinfo_proc *kp;
	int nchr;
	int (*info)(struct ps_strings*, u_long *, int *);
{
	register const struct proc *p = &kp->kp_proc;
	register char **ap;
	u_long addr;
	int cnt;
	struct ps_strings arginfo;

	/*
	 * Pointers are stored at the top of the user stack.
	 */
	if (p->p_stat == SZOMB || 
	    kvm_uread(kd, p, USRSTACK - sizeof(arginfo), (char *)&arginfo,
		      sizeof(arginfo)) != sizeof(arginfo))
		return (0);

	(*info)(&arginfo, &addr, &cnt);
	ap = kvm_argv(kd, p, addr, cnt, nchr);
	/*
	 * For live kernels, make sure this process didn't go away.
	 */
	if (ap != 0 && ISALIVE(kd) &&
	    !proc_verify(kd, (u_long)kp->kp_eproc.e_paddr, p))
		ap = 0;
	return (ap);
}

/*
 * Get the command args.  This code is now machine independent.
 */
char **
kvm_getargv(kd, kp, nchr)
	kvm_t *kd;
	const struct kinfo_proc *kp;
	int nchr;
{
	return (kvm_doargv(kd, kp, nchr, ps_str_a));
}

char **
kvm_getenvv(kd, kp, nchr)
	kvm_t *kd;
	const struct kinfo_proc *kp;
	int nchr;
{
	return (kvm_doargv(kd, kp, nchr, ps_str_e));
}

/*
 * Read from user space.  The user context is given by p.
 */
ssize_t
kvm_uread(kd, p, uva, buf, len)
	kvm_t *kd;
	register const struct proc *p;
	register u_long uva;
	register char *buf;
	register size_t len;
{
	register char *cp;

	cp = buf;
	while (len > 0) {
		u_long pa;
		register int cc;
		
		cc = _kvm_uvatop(kd, p, uva, &pa);
		if (cc > 0) {
			if (cc > len)
				cc = len;
			errno = 0;
			if (lseek(kd->pmfd, (off_t)pa, 0) == -1 && errno != 0) {
				_kvm_err(kd, 0, "invalid address (%x)", uva);
				break;
			}
			cc = read(kd->pmfd, cp, cc);
			if (cc < 0) {
				_kvm_syserr(kd, 0, _PATH_MEM);
				break;
			} else if (cc < len) {
				_kvm_err(kd, kd->program, "short read");
				break;
			}
		} else if (ISALIVE(kd)) {
			/* try swap */
			register char *dp;
			int cnt;

			dp = kvm_readswap(kd, p, uva, &cnt);
			if (dp == 0) {
				_kvm_err(kd, 0, "invalid address (%x)", uva);
				return (0);
			}
			cc = MIN(cnt, len);
			bcopy(dp, cp, cc);
		} else
			break;
		cp += cc;
		uva += cc;
		len -= cc;
	}
	return (ssize_t)(cp - buf);
}
