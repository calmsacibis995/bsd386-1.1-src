/*	BSDI $Id: gcore.c,v 1.4 1993/02/08 06:07:08 polk Exp $	*/

/*-
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * Originally written by Eric Cooper in Fall 1981.
 * Inspired by a version 6 program by Len Levin, 1978.
 * Several pieces of code lifted from Bill Joy's 4BSD ps.
 * Most recently, hacked beyond recognition for 4.4BSD by Steven McCanne,
 * Lawrence Berkeley Laboratory.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *      This product includes software developed by the University of
 *      California, Lawrence Berkeley Laboratory and its contributors.
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

/*
 * gcore - get core images of running processes
 */
#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/kinfo.h>
#include <sys/kinfo_proc.h>
#include <machine/vmparam.h>
#include <a.out.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

int core __P((int, int, struct kinfo_proc *));
int md_core __P((kvm_t *, int, struct kinfo_proc *));

kvm_t *kd;

static unsigned data_offset;
static unsigned text_addr;

int
main(argc, argv)
	int argc;
	char **argv;
{
	register struct proc *p;
	int pid, uid, fd, cnt, op, efd;
	struct kinfo_proc *ki;
	char *corefile = 0;
	int err;
	int sflag = 0;
	struct exec exec;
	char errbuf[_POSIX2_LINE_MAX];
	char fname[64];

	setprog(argv[0]);

        opterr = 0;
        while ((op = getopt(argc, argv, "c:s")) != EOF) {
                switch (op) {

                case 'c':
			corefile = optarg;
                        break;

		case 's':
			++sflag;
			break;

		default:
			usage();
			break;
		}
	}
	argv += optind;
	argc -= optind;
	if (argc != 2)
		usage();

	kd = kvm_openfiles(0, 0, 0, O_RDONLY, errbuf);
	if (kd == 0)
		error("%s", errbuf);

	uid = getuid();
	pid = atoi(argv[1]);
			
	ki = kvm_getprocs(kd, KINFO_PROC_PID, pid, &cnt);
	if (ki == 0 || cnt != 1)
		error("%d: not found", pid);

	p = &ki->kp_proc;
	if (ki->kp_eproc.e_pcred.p_ruid != uid && uid != 0)
		error("%d: not owner", pid);

	if (p->p_stat == SZOMB)
		error("%d: zombie", pid);

	if (p->p_flag & SWEXIT)
		warning("process exiting");
	if (p->p_flag & SSYS)
		/* i.e. swapper or pagedaemon */
		error("%d: system process");

	if (corefile == 0) {
		sprintf(fname, "core.%d", pid);
		corefile = fname;
	}
	fd = open(corefile, O_RDWR|O_CREAT|O_TRUNC, 0666);
	if (fd < 0) {
		perror(corefile);
		exit(1);
	}
	efd = open(argv[0], O_RDONLY);
	if (efd < 0) {
		perror(argv[0]);
		exit(1);
	}
	if (read(efd, &exec, sizeof(exec)) != sizeof(exec))
		error("cannot read exec header of %s", argv[0]);

	data_offset = N_DATOFF(exec);
	text_addr = N_TXTADDR(exec);

	if (sflag && kill(pid, SIGSTOP) < 0) 
		warning("%d: could not deliver stop signal", pid);

	err = core(efd, fd, ki);
	if (err == 0)
		err = md_core(kd, fd, ki);

	if (sflag && kill(pid, SIGCONT) < 0)
		warning("%d: could not deliver continue signal", pid);
	close(fd);

	exit(0);
}

int
userdump(fd, p, addr, npage)
	register int fd;
	struct proc *p;
	register u_long addr;
	register int npage;
{
	register int cc;
	char buffer[NBPG];

	while (--npage >= 0) {
		cc = kvm_uread(kd, p, addr, buffer, NBPG);
		if (cc != NBPG)
			/* could be an untouched fill-with-zero page */
			bzero(buffer, NBPG);
		(void)write(fd, buffer, NBPG);
		addr += NBPG;
	}
	return (0);
}

int
datadump(efd, fd, p, addr, npage)
	register int efd;
	register int fd;
	struct proc *p;
	register u_long addr;
	register int npage;
{
	register int cc, delta;
	char buffer[NBPG];
	
	delta = data_offset - addr;
	while (--npage >= 0) {
		cc = kvm_uread(kd, p, addr, buffer, NBPG);
		if (cc != NBPG) {
			/*
			 * Try to read page from executable.
			 */
			if (lseek(efd, addr + delta, SEEK_SET) == -1)
				return (-1);
			if (read(efd, buffer, sizeof(buffer)) < 0)
				return (-1);
			warning("can't read page at %#x", addr);
		}
		(void)write(fd, buffer, NBPG);
		addr += NBPG;
	}
	return (0);
}

/*
 * Build the core file.
 */
int
core(efd, fd, ki)
	int efd;
	int fd;
	struct kinfo_proc *ki;
{
	union {
		struct user user;
		char ubytes[ctob(UPAGES)];
	} uarea;
	struct proc *p = &ki->kp_proc;
	int tsize = ki->kp_eproc.e_vm.vm_tsize;
	int dsize = ki->kp_eproc.e_vm.vm_dsize;
	int ssize = ki->kp_eproc.e_vm.vm_ssize;

	/* Read in user struct */
	if (kvm_read(kd, (u_long)p->p_addr, (void *)&uarea, sizeof(uarea)) 
	    != sizeof(uarea))
		error("could not read user structure");

	/*
	 * Fill in the eproc vm parameters, since these are garbage unless
	 * the kernel is dumping core or something.
	 */
	uarea.user.u_kproc = *ki;

	/* Dump user area */
	(void)write(fd, (char *)&uarea, sizeof(uarea));

	/* Dump data segment */
	if (datadump(efd, fd, p, text_addr + ctob(tsize), dsize) < 0)
		error("could not dump data segment");

	/* Dump stack segment */
	if (userdump(fd, p, USRSTACK - ctob(ssize), ssize) < 0)
		error("could not dump stack segment");

	return (0);
}
