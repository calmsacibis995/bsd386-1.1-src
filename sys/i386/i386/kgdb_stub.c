/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
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
 *
 * from kgdb_stub.c,v 1.11 92/06/17 05:22:07 torek Exp  
 */

/*
 * "Stub" to allow remote cpu to debug over a serial line using gdb.
 */
#ifdef KGDB
#ifndef lint
/* from LBL version: */
static const char rcsid[] =
    " Header: kgdb_stub.c,v 1.11 92/06/17 05:22:07 torek Exp  ";
#endif

#include "param.h"
#include "systm.h"
#include "proc.h"
#include "vm/vm.h"
#include "../include/trap.h"
#include "../include/cpu.h"
#include "../include/psl.h"
#include "../include/reg.h"
#include "buf.h"
#include "cons.h"
#include "errno.h"

#include "kgdb_proto.h"
#include "machine/remote-sl.h"

#ifndef KGDBDEV
#define KGDBDEV -1
#endif
#ifndef KGDBRATE
#define KGDBRATE 9600
#endif

int kgdb_dev = KGDBDEV;		/* remote debugging device (-1 if none) */
int kgdb_rate = KGDBRATE;	/* remote debugging baud rate */
int kgdb_active = 0;            /* remote debugging active if != 0 */
int kgdb_debug_init = 0;	/* != 0 waits for remote at system init */
int kgdb_debug_panic = 1;	/* != 0 waits for remote on panic */
int kgdb_debug = 0;

static int (*kgdb_getc) __P((void *));
static void (*kgdb_putc) __P((void *, int));
static void *kgdb_ioarg;

#define GETC()	((*kgdb_getc)(kgdb_ioarg))
#define PUTC(c)	((*kgdb_putc)(kgdb_ioarg, c))
#define PUTESC(c) { \
	if (c == FRAME_END) { \
		PUTC(FRAME_ESCAPE); \
		c = TRANS_FRAME_END; \
	} else if (c == FRAME_ESCAPE) { \
		PUTC(FRAME_ESCAPE); \
		c = TRANS_FRAME_ESCAPE; \
	} else if (c == FRAME_START) { \
		PUTC(FRAME_ESCAPE); \
		c = TRANS_FRAME_START; \
	} \
	PUTC(c); \
}

kgdb_attach(getfn, putfn, ioarg)
	int (*getfn) __P((void *));
	void (*putfn) __P((void *, int));
	void *ioarg;
{

	kgdb_getc = getfn;
	kgdb_putc = putfn;
	kgdb_ioarg = ioarg;
}

/*
 * Send a message.  The host gets one chance to read it.
 */
static void
kgdb_send(type, bp, len)
	register u_char type;
	register u_char *bp;
	register int len;
{
	register u_char csum;
	register u_char *ep = bp + len;

	PUTC(FRAME_START);
	PUTESC(type);

	csum = type;
	while (bp < ep) {
		type = *bp++;
		csum += type;
		PUTESC(type);
	}
	csum = -csum;
	PUTESC(csum);
	PUTC(FRAME_END);
}

static int
kgdb_recv(bp, lenp)
	u_char *bp;
	int *lenp;
{
	register u_char c, csum;
	register int escape, len;
	register int type;

restart:
	csum = len = escape = 0;
	type = -1;
	while (1) {
		c = GETC();
		switch (c) {

		case FRAME_ESCAPE:
			escape = 1;
			continue;

		case TRANS_FRAME_ESCAPE:
			if (escape)
				c = FRAME_ESCAPE;
			break;

		case TRANS_FRAME_END:
			if (escape)
				c = FRAME_END;
			break;

		case TRANS_FRAME_START:
			if (escape)
				c = FRAME_START;
			break;
			
		case FRAME_START:
			goto restart;

		case FRAME_END:
			if (type < 0 || --len < 0) {
				csum = len = escape = 0;
				type = -1;
				continue;
			}
			if (csum != 0)
				return (0);
			*lenp = len;
			return (type);
		}
		csum += c;
		if (type < 0) {
			type = c;
			escape = 0;
			continue;
		}
		if (++len > SL_RPCSIZE) {
			while (GETC() != FRAME_END)
				continue;
			return (0);
		}
		*bp++ = c;
		escape = 0;
	}
}

/*
 * Translate a trap number into a unix compatible signal value.
 * (gdb only understands unix signal numbers).
 * ### should this be done at the other end?
 */
static int 
computeSignal(type)
	int type;
{
	int sigval;

	switch (type) {
	case T_RESADFLT:
	case T_PRIVINFLT:
	case T_RESOPFLT:
	case T_ALIGNFLT:
	case T_FPOPFLT:
		sigval = SIGILL;
		break;
	case T_BPTFLT:
	case T_TRCTRAP:
		sigval = SIGTRAP;
		break;
	case T_ARITHTRAP:
	case T_DIVIDE:
	case T_OFLOW:
	case T_BOUND:
	case T_DNA:
		sigval = SIGFPE;
		break;
	case T_SEGFLT:
	case T_PROTFLT:
	case T_SEGNPFLT:
		sigval = SIGBUS;
		break;
	case T_PAGEFLT:
		sigval = SIGSEGV;
		break;

	case T_ASTFLT:		/* ??? */
	case T_NMI:		/* ??? */
	case T_DOUBLEFLT:	/* ??? */
	case T_TSSFLT:		/* ??? */
	case T_STKFLT:		/* ??? */
	default:
		sigval = SIGEMT;
		break;
	}
	return (sigval);
}

/*
 * Trap into kgdb to wait for debugger to connect, 
 * noting on the console why nothing else is going on.
 */
kgdb_connect(verbose)
	int verbose;
{

	if (kgdb_dev < 0 || kgdb_getc == NULL)
		return;
	if (verbose)
		printf("kgdb waiting...");
	asm volatile ("int3");
	if (verbose)
		printf("connected.\n");
}

/*
 * Decide what to do on panic.
 */
kgdb_panic()
{

	if (kgdb_dev >= 0 && kgdb_getc != NULL &&
	    kgdb_active != 0 || kgdb_debug_panic) {
		printf("hit return to drop into kgdb, %s",
		    "any other character to dump core and reboot");
		if (getchar() == '\n')
			kgdb_connect(1);
	}
}

/*
 * Definitions exported from gdb.
 */
#define NUM_REGS 16
#define REGISTER_BYTES (NUM_REGS * 4)
#define REGISTER_BYTE(N)  ((N)*4)

#define GDB_SR 8
#define GDB_PC 9

static inline void
kgdb_copy(register char *src, register char *dst, register int nbytes)
{
	register char *ep = src + nbytes;

	while (src < ep)
		*dst++ = *src++;
}

#ifndef tISP
#define	tISP	(5)	/* XXX */
#endif

static const char trapmap[] = 		/* from gdb */
{
	tEAX, tECX, tEDX, tEBX,
	tISP, tEBP, tESI, tEDI,
	tEIP, tEFLAGS, tCS, tDS,	/* tSS doesn't exist for kernel trap */
	tDS, tES, tES, tES		/* lies: no fs or gs */
};

static inline void
regs_to_gdb(struct trapframe *fp, register u_long *gdb)
{
	register const char *tp;
	register u_long *regs = (u_long *)fp;

	for (tp = trapmap; tp < &trapmap[sizeof trapmap]; ++tp)
		*gdb++ = regs[*tp];
}

static inline void
gdb_to_regs(struct trapframe *fp, register u_long *gdb)
{
	register const char *tp;
	register u_long *regs = (u_long *)fp;

	for (tp = trapmap; tp < &trapmap[sizeof trapmap]; ++tp)
		regs[*tp] = *gdb++;
}

static u_long reg_cache[NUM_REGS];
static u_char inbuffer[SL_RPCSIZE];
static u_char outbuffer[SL_RPCSIZE];

/*
 * This function does all command procesing for interfacing to 
 * a remote gdb.
 */
int 
kgdb_trap(type, tf)
	int type;
	register struct trapframe *tf;
{
	register u_long len;
	caddr_t addr;
	register u_char *cp;
	register u_char out, in;
	register int outlen;
	int inlen;
	u_long gdb_regs[NUM_REGS];
	int saved_kgdb_active = kgdb_active;

	if (kgdb_debug > 1)
		printf("kgdb_trap: entering, type = %x, eip = %x\n",
		    type, tf->tf_eip);

	if (kgdb_dev < 0 || kgdb_getc == NULL) {
		/* not debugging */
		return (0);
	}
	if (kgdb_active == 0) {
		if (kgdb_debug > 2)
			printf("kgdb_trap: no active debugger session\n");

		if (type != T_BPTFLT) {
			/* No debugger active -- let trap handle this. */
			return (0);
		}

		/*
		 * If the packet that woke us up isn't an exec packet,
		 * ignore it since there is no active debugger.  Also,
		 * we check that it's not an ack to be sure that the 
		 * remote side doesn't send back a response after the
		 * local gdb has exited.  Otherwise, the local host
		 * could trap into gdb if it's running a gdb kernel too.
		 */
		in = GETC();
		/*
		 * If we came in asynchronously through the serial line,
		 * the framing character is eaten by the receive interrupt,
		 * but if we come in through a synchronous trap (i.e., via
		 * kgdb_connect()), we will see the extra character.
		 */
		if (in == FRAME_START)
			in = GETC();

		if (kgdb_debug > 2)
			printf("kgdb_trap: first command = %x\n", in);

		if (KGDB_CMD(in) != KGDB_EXEC || (in & KGDB_ACK) != 0)
			return (0);
		while (GETC() != FRAME_END)
			continue;
		/*
		 * Do the printf *before* we ack the message.  This way
		 * we won't drop any inbound characters while we're 
		 * doing the polling printf.
		 */
		printf("kgdb started from device %x\n", kgdb_dev);
		kgdb_send(in | KGDB_ACK, (u_char *)0, 0);
		kgdb_active = 1;
	}
	if (saved_kgdb_active == kgdb_active) {
		/*
		 * Only send an asynchronous SIGNAL message when we hit
		 * a breakpoint.  Otherwise, we will drop the incoming
		 * packet while we output this one (and on entry the other 
		 * side isn't interested in the SIGNAL type -- if it is,
		 * it will have used a signal packet.)
		 */
		outbuffer[0] = computeSignal(type);
		kgdb_send(KGDB_SIGNAL, outbuffer, 1);
	}
	/*
	 * Stick frame regs into our reg cache then tell remote host
	 * that an exception has occured.
	 */
	regs_to_gdb(tf, gdb_regs);

	for (;;) {
		in = kgdb_recv(inbuffer, &inlen);
		if (kgdb_debug > 2)
			printf("kgdb_trap: command %x\n", in);
		if (in == 0 || (in & KGDB_ACK))
			/* Ignore inbound acks and error conditions. */
			continue;

		out = in | KGDB_ACK;
		switch (KGDB_CMD(in)) {

		case KGDB_SIGNAL:
			/*
			 * if this command came from a running gdb,
			 * answer it -- the other guy has no way of
			 * knowing if we're in or out of this loop
			 * when he issues a "remote-signal".  (Note
			 * that without the length check, we could
			 * loop here forever if the output line is
			 * looped back or the remote host is echoing.)
			 */
			if (inlen == 0) {
				outbuffer[0] = computeSignal(type);
				kgdb_send(KGDB_SIGNAL, outbuffer, 1);
			}
			continue;

		case KGDB_REG_R:
		case KGDB_REG_R | KGDB_DELTA:
			cp = outbuffer;
			outlen = 0;
			for (len = inbuffer[0]; len < NUM_REGS; ++len) {
				if (reg_cache[len] != gdb_regs[len] ||
				    (in & KGDB_DELTA) == 0) {
					if (outlen + 5 > SL_MAXDATA) {
						out |= KGDB_MORE;
						break;
					}
					cp[outlen] = len;
					kgdb_copy((caddr_t)&gdb_regs[len],
					    (caddr_t)&cp[outlen + 1], 4);
					reg_cache[len] = gdb_regs[len];
					outlen += 5;
				}
			}
			break;
			
		case KGDB_REG_W:
		case KGDB_REG_W | KGDB_DELTA:
			cp = inbuffer;
			for (len = 0; len < inlen; len += 5) {
				register int j = cp[len];

				kgdb_copy((caddr_t)&cp[len + 1],
				    (caddr_t)&gdb_regs[j], 4);
				reg_cache[j] = gdb_regs[j];
			}
			gdb_to_regs(tf, gdb_regs);
			outlen = 0;
			break;
				
		case KGDB_MEM_R:
			len = inbuffer[0];
			kgdb_copy((caddr_t)&inbuffer[1], (caddr_t)&addr, 4);
			if (len > SL_MAXDATA) {
				outlen = 1;
				outbuffer[0] = E2BIG;
			} else if (!kgdb_acc(addr, len, B_READ, 1)) {
				outlen = 1;
				outbuffer[0] = EFAULT;
			} else {
				outlen = len + 1;
				outbuffer[0] = 0;
				kgdb_copy(addr, (caddr_t)&outbuffer[1], len);
			}
			break;

		case KGDB_MEM_W:
			len = inlen - 4;
			kgdb_copy((caddr_t)inbuffer, (caddr_t)&addr, 4);
			outlen = 1;
			if (!kgdb_acc(addr, len, B_READ, 0))
				outbuffer[0] = EFAULT;
			else {
				outbuffer[0] = 0;
				if (!kgdb_acc(addr, len, B_WRITE, 0))
					kdb_mkwrite(addr, len);
				kgdb_copy((caddr_t)&inbuffer[4], addr, len);
			}
			break;

		case KGDB_KILL:
			kgdb_active = 0;
			printf("kgdb detached\n");
			/* fall through */
		case KGDB_CONT:
			kgdb_send(out, 0, 0);
			tf->tf_eflags &=~ PSL_T;
			return (1);

		case KGDB_STEP:
			kgdb_send(out, 0, 0);
			tf->tf_eflags |= PSL_T;
			return (1);

		case KGDB_EXEC:
		default:
			/* Unknown command.  Ack with a null message. */
			outlen = 0;
			break;
		}

		if (kgdb_debug > 2)
			printf("kgdb_trap: reply %x\n", outbuffer[0]);
		/* Send the reply */
		kgdb_send(out, outbuffer, outlen);
	}
}

/*
 * XXX do kernacc and useracc calls if safe, otherwise use PTE protections.
 */
kgdb_acc(addr, len, rw, usertoo)
	caddr_t addr;
	int len, rw, usertoo;
{
	extern char proc0paddr[], kstack[];		/* XXX! */
	extern char *kernel_map;		/* XXX! */

	if (kstack <= addr && addr < kstack + UPAGES * NBPG)
		/*
		 * Horrid hack -- the kernel stack is mapped
		 * by the user page table, but we guarantee (?) its presence.
		 */
		return (1);
	if (kernel_map != NULL) {
		if (kernacc(addr, len, rw))
			return (1);
		if (usertoo && curproc && useracc(addr, len, rw))
			return (1);
	}

#if 0
	/* Fails for physical address zero: too bad! */
	if (rw != B_WRITE && curproc && curproc->p_vmspace &&
	    pmap_extract(&curproc->p_vmspace->vm_pmap, addr) &&
	    pmap_extract(&curproc->p_vmspace->vm_pmap, addr + len - 1))
		return (1);
	if (addr < proc0paddr + UPAGES * NBPG  ||
	    kstack <= addr && addr < kstack + UPAGES * NBPG)
		return (1);
#endif
	return (0);
}

kdb_mkwrite(addr, len)
	register caddr_t addr;
	register int len;
{

#if 0
	if (kernel_map != NULL) {
		chgkprot(addr, len, B_WRITE);
		return;
	}
#endif

	/* XXX currently a no-op: if KGDB is defined, we can write anything */
}
#endif
