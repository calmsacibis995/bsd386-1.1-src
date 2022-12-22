/*-
 * Copyright (c) 1992, 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: kern_resource.c,v 1.6 1994/02/06 20:11:01 karels Exp $
 */

/*-
 * Copyright (c) 1982, 1986, 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * (c) UNIX System Laboratories, Inc.  All or some portions of this file
 * are derived from material licensed to the University of California by
 * American Telephone and Telegraph Co. or UNIX System Laboratories, Inc.
 * and are reproduced herein with the permission of UNIX System Laboratories,
 * Inc.
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
 *	@(#)kern_resource.c	7.13 (Berkeley) 5/9/91
 */

#include "param.h"
#include "resourcevar.h"
#include "file.h"
#include "filedesc.h"
#include "malloc.h"
#include "proc.h"

#include "vm/vm.h"

/*
 * Resource controls and accounting.
 */

getpriority(curp, uap, retval)
	struct proc *curp;
	register struct args {
		int	which;
		int	who;
	} *uap;
	int *retval;
{
	register struct proc *p;
	register int low = PRIO_MAX + 1;

	switch (uap->which) {

	case PRIO_PROCESS:
		if (uap->who == 0)
			p = curp;
		else
			p = pfind(uap->who);
		if (p == 0)
			break;
		low = p->p_nice;
		break;

	case PRIO_PGRP: {
		register struct pgrp *pg;

		if (uap->who == 0)
			pg = curp->p_pgrp;
		else if ((pg = pgfind(uap->who)) == NULL)
			break;
		for (p = pg->pg_mem; p != NULL; p = p->p_pgrpnxt) {
			if (p->p_nice < low)
				low = p->p_nice;
		}
		break;
	}

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = curp->p_ucred->cr_uid;
		for (p = allproc; p != NULL; p = p->p_nxt) {
			if (p->p_ucred->cr_uid == uap->who &&
			    p->p_nice < low)
				low = p->p_nice;
		}
		break;

	default:
		return (EINVAL);
	}
	if (low == PRIO_MAX + 1)
		return (ESRCH);
	*retval = low;
	return (0);
}

/* ARGSUSED */
setpriority(curp, uap, retval)
	struct proc *curp;
	register struct args {
		int	which;
		int	who;
		int	prio;
	} *uap;
	int *retval;
{
	register struct proc *p;
	int found = 0, error = 0;

	switch (uap->which) {

	case PRIO_PROCESS:
		if (uap->who == 0)
			p = curp;
		else
			p = pfind(uap->who);
		if (p == 0)
			break;
		error = donice(curp, p, uap->prio);
		found++;
		break;

	case PRIO_PGRP: {
		register struct pgrp *pg;
		 
		if (uap->who == 0)
			pg = curp->p_pgrp;
		else if ((pg = pgfind(uap->who)) == NULL)
			break;
		for (p = pg->pg_mem; p != NULL; p = p->p_pgrpnxt) {
			error = donice(curp, p, uap->prio);
			found++;
		}
		break;
	}

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = curp->p_ucred->cr_uid;
		for (p = allproc; p != NULL; p = p->p_nxt)
			if (p->p_ucred->cr_uid == uap->who) {
				error = donice(curp, p, uap->prio);
				found++;
			}
		break;

	default:
		return (EINVAL);
	}
	if (found == 0)
		return (ESRCH);
	return (0);
}

donice(curp, chgp, n)
	register struct proc *curp, *chgp;
	register int n;
{
	register struct pcred *pcred = curp->p_cred;

	if (pcred->pc_ucred->cr_uid && pcred->p_ruid &&
	    pcred->pc_ucred->cr_uid != chgp->p_ucred->cr_uid &&
	    pcred->p_ruid != chgp->p_ucred->cr_uid)
		return (EPERM);
	if (n > PRIO_MAX)
		n = PRIO_MAX;
	if (n < PRIO_MIN)
		n = PRIO_MIN;
	if (n < chgp->p_nice && suser(pcred->pc_ucred, &curp->p_acflag))
		return (EACCES);
	chgp->p_nice = n;
	(void) setpri(chgp);
	return (0);
}

/* ARGSUSED */
setrlimit(p, uap, retval)
	struct proc *p;
	register struct args {
		u_int	which;
		struct	rlimit *lim;
	} *uap;
	int *retval;
{
	struct rlimit alim;
	register struct rlimit *alimp;
	long max;
	int error;

	if (uap->which >= RLIM_NLIMITS)
		return (EINVAL);
	alimp = &p->p_rlimit[uap->which];
	if (error =
	    copyin((caddr_t)uap->lim, (caddr_t)&alim, sizeof (struct rlimit)))
		return (error);
	if (alim.rlim_cur > alimp->rlim_max || alim.rlim_max > alimp->rlim_max)
		if (error = suser(p->p_ucred, &p->p_acflag))
			return (error);

	/*
	 * Enforce system upper limits silently.
	 */
	switch (uap->which) {

	case RLIMIT_DATA:
		max = maxdsiz;
		break;

	case RLIMIT_STACK:
		max = maxssiz;
		break;

	case RLIMIT_NPROC:
		max = min(malloc_sizelimit(sizeof(struct proc), M_PROC),
			malloc_sizelimit(sizeof(struct vmspace), M_VMMAP)) / 2;
		break;
	case RLIMIT_OFILE:
		max = min(malloc_sizelimit(M_FILE, sizeof(struct file)),
			malloc_limit(M_FILEDESC) / OFILESIZE) / 2;
		break;

	default:
		max = 0;
		break;
	}

	if (max) {
		if (alim.rlim_cur > max)
			alim.rlim_cur = max;
		if (alim.rlim_max > max)
			alim.rlim_max = max;
	}

	/*
	 * The cpu limit code assumes that the hard limit
	 * is no less than the soft limit.
	 */
	if (alim.rlim_cur > alim.rlim_max)
		if (suser(p->p_ucred, &p->p_acflag) == 0)
			alim.rlim_max = alim.rlim_cur;
		else
			return (EINVAL);

	if (p->p_limit->p_refcnt > 1 &&
	    (p->p_limit->p_lflags & PL_SHAREMOD) == 0) {
		p->p_limit->p_refcnt--;
		p->p_limit = limcopy(p->p_limit);
	}

	if (uap->which == RLIMIT_STACK) {
		/*
		 * Stack is allocated to the max at exec time with only
		 * "rlim_cur" bytes accessible.  If stack limit is going
		 * up make more accessible, if going down make inaccessible.
		 */
		if (alim.rlim_cur != alimp->rlim_cur) {
			vm_offset_t addr;
			vm_size_t size;
			vm_prot_t prot;

			if (alim.rlim_cur > alimp->rlim_cur) {
				prot = VM_PROT_ALL;
				size = alim.rlim_cur - alimp->rlim_cur;
				addr = USRSTACK - alim.rlim_cur;
			} else {
				prot = VM_PROT_NONE;
				size = alimp->rlim_cur - alim.rlim_cur;
				addr = USRSTACK - alimp->rlim_cur;
			}
			addr = trunc_page(addr);
			size = round_page(size);
			(void) vm_map_protect(&p->p_vmspace->vm_map,
					      addr, addr+size, prot, FALSE);
		}
	}
	p->p_rlimit[uap->which] = alim;
	return (0);
}

/* ARGSUSED */
getrlimit(p, uap, retval)
	struct proc *p;
	register struct args {
		u_int	which;
		struct	rlimit *rlp;
	} *uap;
	int *retval;
{

	if (uap->which >= RLIM_NLIMITS)
		return (EINVAL);
	return (copyout((caddr_t)&p->p_rlimit[uap->which], (caddr_t)uap->rlp,
	    sizeof (struct rlimit)));
}

/* ARGSUSED */
getrusage(p, uap, retval)
	register struct proc *p;
	register struct args {
		int	who;
		struct	rusage *rusage;
	} *uap;
	int *retval;
{
	register struct rusage *rup;

	switch (uap->who) {

	case RUSAGE_SELF: {
		int s;

		rup = &p->p_stats->p_ru;
		s = splclock();
		rup->ru_stime = p->p_stime;
		rup->ru_utime = p->p_utime;
		splx(s);
		break;
	}

	case RUSAGE_CHILDREN:
		rup = &p->p_stats->p_cru;
		break;

	default:
		return (EINVAL);
	}
	return (copyout((caddr_t)rup, (caddr_t)uap->rusage,
	    sizeof (struct rusage)));
}

ruadd(ru, ru2)
	register struct rusage *ru, *ru2;
{
	register long *ip, *ip2;
	register int i;

	timevaladd(&ru->ru_utime, &ru2->ru_utime);
	timevaladd(&ru->ru_stime, &ru2->ru_stime);
	if (ru->ru_maxrss < ru2->ru_maxrss)
		ru->ru_maxrss = ru2->ru_maxrss;
	ip = &ru->ru_first; ip2 = &ru2->ru_first;
	for (i = &ru->ru_last - &ru->ru_first; i > 0; i--)
		*ip++ += *ip2++;
}

/*
 * Make a copy of the plimit structure.
 * We share these structures copy-on-write after fork,
 * and copy when a limit is changed.
 */
struct plimit *
limcopy(lim)
	struct plimit *lim;
{
	register struct plimit *copy;

	MALLOC(copy, struct plimit *, sizeof(struct plimit),
	    M_SUBPROC, M_WAITOK);
	bcopy(lim->pl_rlimit, copy->pl_rlimit,
	    sizeof(struct rlimit) * RLIM_NLIMITS);
	copy->p_lflags = 0;
	copy->p_refcnt = 1;
	return (copy);
}
