/*-
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
 *	@(#)compat.h	5.5 (Berkeley) 1/10/93
 */

#ifdef NO_U_UNDERSCORES
typedef u_long	unsigned long
typedef u_short	unsigned short
typedef u_char	unsigned char
#endif

/* 32-bit machine */
#ifndef USHRT_MAX
#define	USHRT_MAX		0xFFFF
#define	ULONG_MAX		0xFFFFFFFF
#endif

#ifdef NO_SIZE_T
typedef size_t	unsigned int
#endif

#ifndef O_ACCMODE			/* POSIX 1003.1 access mode mask. */
#define	O_ACCMODE	(O_RDONLY|O_WRONLY|O_RDWR)
#endif

#ifndef O_EXLOCK			/* No locking! */
#define	O_EXLOCK	0
#endif

#ifndef O_SHLOCK
#define	O_SHLOCK	0
#endif

#ifndef SIZE_T_MAX			/* Size_t is probably a u_long. */
#define	SIZE_T_MAX	ULONG_MAX
#endif

#ifndef EFTYPE
#define EFTYPE  EINVAL			/* POSIX 1003.1 format errno. */
#endif

#ifndef BYTE_ORDER
#define LITTLE_ENDIAN   1234		/* LSB first: i386, vax */
#define BIG_ENDIAN      4321		/* MSB first: 68000, ibm, net */
#define BYTE_ORDER      BIG_ENDIAN	/* Set for your system. */
#endif

#ifndef STDERR_FILENO
#define  STDIN_FILENO   0		/* ANSI C #defines */
#define STDOUT_FILENO   1
#define STDERR_FILENO   2
#endif

#ifndef SEEK_END
#define	SEEK_SET	0		/* POSIX 1003.1 seek values */
#define	SEEK_CUR	1
#define	SEEK_END	2
#endif

#ifndef NULL				/* ANSI C #defines NULL everywhere. */
#define NULL 0
#endif

#ifndef	MAX
#define	MAX(_a,_b)	((_a)<(_b)?(_b):(_a))
#endif
#ifndef	MIN
#define	MIN(_a,_b)	((_a)<(_b)?(_a):(_b))
#endif

#if defined(SYSV) || defined(SYSTEM5)
#define	index(a, b)	strchr(a, b)
#define	rindex(a, b)	strrchr(a, b)
#define	bzero(a, b)	memset(a, 0, b)
#define	bcmp(a, b, n)	memcmp(a, b, n)
#define	bcopy(a, b, n)	memmove(b, a, n)
#endif

#ifndef SIG_BLOCK
/*
 * If you don't have POSIX 1003.1 signals, the signal code surrounding the 
 * temporary file creation is intended to block all of the possible signals
 * long enough to create the file and unlink it.
 */

typedef unsigned int sigset_t;

#define sigemptyset(set)        (*(set) = 0)
#define sigfillset(set)         (*(set) = ~(sigset_t)0, 0)
#define sigaddset(set,signo)    (*(set) |= sigmask(signo), 0)
#define sigdelset(set,signo)    (*(set) &= ~sigmask(signo), 0)
#define sigismember(set,signo)  ((*(set) & sigmask(signo)) != 0)

#define SIG_BLOCK       1
#define SIG_UNBLOCK     2
#define SIG_SETMASK     3

static int __sigtemp;            /* For the use of sigprocmask */

#define sigprocmask(how,set,oset) \
        ((__sigtemp = (((how) == SIG_BLOCK) ? \
        sigblock(0) | *(set) : (((how) == SIG_UNBLOCK) ? \
        sigblock(0) & ~(*(set)) : ((how) == SIG_SETMASK ? \
        *(set) : sigblock(0))))), ((oset) ? \
        (*(oset) = sigsetmask(__sigtemp)) : sigsetmask(__sigtemp)), 0)
#endif
