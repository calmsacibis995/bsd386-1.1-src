/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: reboot.h,v 1.5 1993/02/05 21:52:21 karels Exp $
 */

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 *	@(#)reboot.h	7.6 (Berkeley) 6/28/90
 */

/*
 * Arguments to reboot system call.
 * These are passed to boot program in r11,
 * and on to init.
 */
#define	RB_AUTOBOOT	0	/* flags for system auto-booting itself */

#define	RB_ASKNAME	0x01	/* ask for file name to reboot from */
#define	RB_SINGLE	0x02	/* reboot to single user only */
#define	RB_NOSYNC	0x04	/* dont sync before reboot */
#define	RB_HALT		0x08	/* don't reboot, just halt */
#define	RB_INITNAME	0x10	/* name given for /sbin/init (unused) */
#define	RB_DFLTROOT	0x20	/* use compiled-in rootdev */
#define	RB_KDB		0x40	/* give control to kernel debugger */
#define	RB_RWROOT	0x80	/* mount root fs read-write */
#define	RB_DUMP		0x100	/* dump kernel memory before reboot */

/*
 * Constants for converting boot-style device number to type,
 * adaptor (uba, mba, etc), unit number and partition number.
 * Type (== major device number) is in the low byte
 * for backward compatibility.  Except for that of the "magic
 * number", each mask applies to the shifted value.
 * Format:
 *	 (4) (4) (4) (4)  (8)     (8)
 *	--------------------------------
 *	|MA | AD| CT| UN| PART  | TYPE |
 *	--------------------------------
 */
#define	B_ADAPTORSHIFT		24
#define	B_ADAPTORMASK		0x0f
#define	B_ADAPTOR(val)		(((val) >> B_ADAPTORSHIFT) & B_ADAPTORMASK)
#define B_CONTROLLERSHIFT	20
#define B_CONTROLLERMASK	0xf
#define	B_CONTROLLER(val)	(((val)>>B_CONTROLLERSHIFT) & B_CONTROLLERMASK)
#define B_UNITSHIFT		16
#define B_UNITMASK		0xf
#define	B_UNIT(val)		(((val) >> B_UNITSHIFT) & B_UNITMASK)
#define B_PARTITIONSHIFT	8
#define B_PARTITIONMASK		0xff
#define	B_PARTITION(val)	(((val) >> B_PARTITIONSHIFT) & B_PARTITIONMASK)
#define	B_TYPESHIFT		0
#define	B_TYPEMASK		0xff
#define	B_TYPE(val)		(((val) >> B_TYPESHIFT) & B_TYPEMASK)

#ifdef LOCORE
#define	B_MAGICMASK	0xf0000000
#define	B_DEVMAGIC	0xa0000000
#else
#define	B_MAGICMASK	((u_long)0xf0000000)
#define	B_DEVMAGIC	((u_long)0xa0000000)
#endif

#define MAKEBOOTDEV(type, adaptor, controller, unit, partition) \
	(((type) << B_TYPESHIFT) | ((adaptor) << B_ADAPTORSHIFT) | \
	((controller) << B_CONTROLLERSHIFT) | ((unit) << B_UNITSHIFT) | \
	((partition) << B_PARTITIONSHIFT) | B_DEVMAGIC)

/*
 * Definition of parameters passed to kernel by bootstrap.
 * We pass a variable number of variable sized parameters,
 * thus we use an array of self-describing elements.
 * The bootparamhdr structure is the header for the whole array,
 * and the bootparam structure heads each element of the array.
 */
#define	BOOT_MAGIC	0xB00DEF01

#ifndef LOCORE
struct bootparamhdr {
	u_long	b_magic;	/* BOOT_MAGIC */
	u_long	b_len;		/* total length, including header */
};
#else
/* XXX for bootstrap startup code */
#define	B_MAGIC	0
#if ULONG_MAX == 0xffffffff
#define	B_LEN	4
#endif
#endif /* LOCORE */

#ifndef LOCORE
struct bootparam {
	u_long	b_type;		/* parameter type */
	u_long	b_len;		/* element length, including header */
	/* then parameter value */
};

/* given pointer to struct bootparamhdr, return pointer to first element */
#define	B_FIRSTPARAM(bph)	((struct bootparam *)((bph) + 1))

/* given pointer to struct bootparam, return pointer to next bootparam */
#define	B_NEXTPARAM(bph, bp) \
	(((caddr_t) (bp) + (bp)->b_len + sizeof(struct bootparam) > \
	   (caddr_t) (bph) + (bph)->b_len) ? (struct bootparam *)NULL : \
	    (struct bootparam *) ((caddr_t) (bp) + ALIGN((bp)->b_len)))

/* given pointer to struct bootparam, return pointer to data */
#define	B_DATA(bp)		((u_char *)((bp) + 1))
#endif /* LOCORE */

/*
 * Boot parameter types, and structures used for value
 */

#define	B_ROOTTYPE	1	/* int: type of root to use */
#define	    ROOT_LOCAL	    0		/* local disk */
#define	    ROOT_NFS	    1		/* nfs server */
#define	    ROOT_MEMFS	    2		/* local memory file system */
#define	B_NFSPARAMS	2	/* nfs_diskless: remote file system specs */
#define	B_MEM		3	/* boot_mem: physical memory available */
#define	B_MEMFS		4	/* boot_mem: prefilled RAM disk */
#define	B_AUTODEBUG	5	/* int: autoconfiguration debug value */

#ifndef LOCORE
struct boot_mem {
	u_long	membase;	/* starting address */
	u_long	memlen;		/* length in bytes */
};
#endif

#define	B_MACHDEP_BASE	0x10000		/* start of machine-dependent */
#define	B_MACHDEP(i)	(B_MACHDEP_BASE + (i))

#include "machine/bootparam.h"
