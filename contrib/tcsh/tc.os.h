/* $Header: /bsdi/MASTER/BSDI_OS/contrib/tcsh/tc.os.h,v 1.2 1994/01/13 23:31:48 polk Exp $ */
/*
 * tc.os.h: Shell os dependent defines
 */
/*-
 * Copyright (c) 1980, 1991 The Regents of the University of California.
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
#ifndef _h_tc_os
#define _h_tc_os

#define NEEDstrerror		/* Too hard to find which systems have it */


#ifdef notdef 
/*
 * for SVR4 and linux we used to fork pipelines backwards. 
 * This should not be needed any more.
 * more info in sh.sem.c
 */
# define BACKPIPE
#endif /* notdef */

#ifdef   _VMS_POSIX
# ifndef  NOFILE 
#  define  NOFILE 64
# endif /* NOFILE */
# define  nice(a)       setprio((getpid()),a)
# undef   NEEDstrerror    /* won't get sensible error messages otherwise */
# define  NEEDgethostname 
# include <sys/time.h>    /* for time stuff in tc.prompt.c */
# include <limits.h>
#endif /*atp vmsposix*/

#if defined(OPEN_MAX) && !defined(NOFILE)
# define NOFILE OPEN_MAX
#endif /* OPEN_MAX && !NOFILE */

#if defined(USR_NFDS) && !defined(NOFILE)
# define NOFILE USR_NFDS
#endif /* USR_NFDS && !NOFILE */

#ifndef NOFILE
# define NOFILE 256
#endif /* NOFILE */

#ifdef linux
# undef NEEDstrerror
#endif /* linux */

#ifdef OREO
# include <sys/time.h>
# ifdef notdef
  /* Don't include it, because it defines things we don't really have */
#  include <sys/resource.h>	
# endif /* notdef */
# ifdef POSIX
#  include <sys/tty.h>
#  include <termios.h>
# endif /* POSIX */
#endif /* OREO */

#ifndef NCARGS
# ifdef ARG_MAX
#  define NCARGS ARG_MAX
# else /* !ARG_MAX */
#  ifdef _MINIX
#   define NCARGS 80
#  else /* !_MINIX */
#   define NCARGS 1024
#  endif /* _MINIX */
# endif /* ARG_MAX */
#endif /* NCARGS */

#ifdef titan
extern int end;
#endif /* titan */

#ifdef hpux
# ifdef lint
/*
 * Hpux defines struct ucred, in <sys/user.h>, but if I include that
 * then I need to include the *world*
 * [all this to pass lint cleanly!!!]
 * so I define struct ucred here...
 */
struct ucred {
    int     foo;
};
# endif /* lint */

/*
 * hpux 7.0 does not define it
 */
# ifndef CSUSP
#  define CSUSP 032
# endif	/* CSUSP */

# ifndef hp9000s500
#  include <sys/bsdtty.h>
# endif /* hp9000s500 */

# ifndef POSIX
#  ifdef BSDJOBS
#   define getpgrp(a) getpgrp2(a)
#   define setpgrp(a, b) setpgrp2(a, b)
#  endif /* BSDJOBS */
# endif	/* POSIX */
#endif /* hpux */

/*
 * ISC does not define CSUSP
 */
#ifdef ISC
# ifndef CSUSP
#  define CSUSP 032
# endif	/* CSUSP */
# if defined(POSIX) && !defined(TIOCGWINSZ)
/*
 * ISC defines this only in termio.h. If we are using POSIX and include
 * termios.h, then we define it ourselves so that window resizing works.
 */
#  define TIOCGWINSZ      (('T'<<8)|104)
# endif /* POSIX && !TIOCGWINSZ */
#endif /* ISC */

#ifdef ISC202
# undef TIOCGWINSZ
#endif /* ISC202 */

/*
 * XXX: This will be changed soon to 
 * #if (SYSVREL > 0) && defined(TIOCGWINSZ)
 * If that breaks on your machine, let me know.
 *
 * It would break on linux, where all this is
 * defined in <termios.h>. Wrapper added.
 */
#if !defined(linux) && !defined(_VMS_POSIX)
# if defined(INTEL) || defined(u3b2) || defined (u3b5) || defined(ub15) || defined(u3b20d) || defined(ISC) || defined(SCO) || defined(tower32)
#  ifdef TIOCGWINSZ
/*
 * for struct winsiz
 */
#   include <sys/stream.h>
#   include <sys/ptem.h>
#  endif /* TIOCGWINSZ */
#  ifndef ODT
#   define NEEDgethostname
#  endif /* ODT */
# endif /* INTEL || u3b2 || u3b5 || ub15 || u3b20d || ISC || SCO || tower32 */
#endif /* !linux && !_VMS_POSIX */

#if defined(UNIXPC) || defined(COHERENT)
# define NEEDgethostname
#endif /* UNIXPC || COHERENT */

#ifdef IRIS4D
# include <sys/time.h>
# include <sys/resource.h>
# ifndef POSIX
/*
 * BSDsetpgrp() and BSDgetpgrp() are BSD versions of setpgrp, etc.
 */
#  define setpgrp BSDsetpgrp
#  define getpgrp BSDgetpgrp
# endif /* POSIX */
#endif /* IRIS4D */

/*
 * For some versions of system V software, specially ones that use the 
 * Wollongong Software TCP/IP, the FIOCLEX, FIONCLEX, FIONBIO calls
 * might not work correctly for file descriptors [they work only for
 * sockets]. So we try to use first the fcntl() and we only use the
 * ioctl() form, only if we don't have the fcntl() one.
 *
 * From: scott@craycos.com (Scott Bolte)
 */
#ifdef F_SETFD
# define close_on_exec(fd, v) fcntl((fd), F_SETFD, v)
#else /* !F_SETFD */
# ifdef FIOCLEX
# define close_on_exec(fd, v) ioctl((fd), ((v) ? FIOCLEX : FIONCLEX), NULL)
# else /* !FIOCLEX */
# define close_on_exec(fd, v)	/* Nothing */
# endif /* FIOCLEX */
#endif /* F_SETFD */

/*
 * Stat
 */
#ifdef ISC
/* these are not defined for _POSIX_SOURCE under ISC 2.2 */
# ifndef S_IFMT
#  define S_IFMT  0170000		/* type of file */
#  define S_IFDIR 0040000		/* directory */
#  define S_IFCHR 0020000		/* character special */
#  define S_IFBLK 0060000		/* block special */
#  define S_IFREG 0100000		/* regular */
#  define S_IFIFO 0010000		/* fifo */
#  define S_IFNAM 0050000		/* special named file */
# endif /* S_IFMT */
#endif /* ISC */

#if defined(uts) || defined(UTekV) || defined(sysV88)
/*
 * The uts 2.1.2 macros (Amdahl) are busted!
 * You should fix <sys/stat.h>, cause other programs will break too!
 *
 * From: creiman@ncsa.uiuc.edu (Charlie Reiman)
 */

/*
 * The same applies to Motorola MPC (System V/88 R32V2, UTekV 3.2e) 
 * workstations, the stat macros are broken.
 * Kaveh Ghazi (ghazi@caip.rutgers.edu)
 */
# undef S_ISDIR
# undef S_ISCHR
# undef S_ISBLK
# undef S_ISREG
# undef S_ISFIFO
# undef S_ISNAM
# undef S_ISLNK
# undef S_ISSOCK
#endif /* uts || UTekV || sysV88 */

#ifdef S_IFMT
# if !defined(S_ISDIR) && defined(S_IFDIR)
#  define S_ISDIR(a)	(((a) & S_IFMT) == S_IFDIR)
# endif	/* ! S_ISDIR && S_IFDIR */
# if !defined(S_ISCHR) && defined(S_IFCHR)
#  define S_ISCHR(a)	(((a) & S_IFMT) == S_IFCHR)
# endif /* ! S_ISCHR && S_IFCHR */
# if !defined(S_ISBLK) && defined(S_IFBLK)
#  define S_ISBLK(a)	(((a) & S_IFMT) == S_IFBLK)
# endif	/* ! S_ISBLK && S_IFBLK */
# if !defined(S_ISREG) && defined(S_IFREG)
#  define S_ISREG(a)	(((a) & S_IFMT) == S_IFREG)
# endif	/* ! S_ISREG && S_IFREG */
# if !defined(S_ISFIFO) && defined(S_IFIFO)
#  define S_ISFIFO(a)	(((a) & S_IFMT) == S_IFIFO)
# endif	/* ! S_ISFIFO && S_IFIFO */
# if !defined(S_ISNAM) && defined(S_IFNAM)
#  define S_ISNAM(a)	(((a) & S_IFMT) == S_IFNAM)
# endif	/* ! S_ISNAM && S_IFNAM */
# if !defined(S_ISLNK) && defined(S_IFLNK)
#  define S_ISLNK(a)	(((a) & S_IFMT) == S_IFLNK)
# endif	/* ! S_ISLNK && S_IFLNK */
# if !defined(S_ISSOCK) && defined(S_IFSOCK)
#  define S_ISSOCK(a)	(((a) & S_IFMT) == S_IFSOCK)
# endif	/* ! S_ISSOCK && S_IFSOCK */
#endif /* S_IFMT */

#ifdef tower32
/* The header files lie; we really don't have symlinks */
# undef S_ISLNK
# undef S_IFLNK
#endif /* tower32 */

#ifndef S_IREAD
# define S_IREAD 0000400
#endif /* S_IREAD */
#ifndef S_IROTH
# define S_IROTH (S_IREAD >> 6)
#endif /* S_IROTH */
#ifndef S_IRGRP
# define S_IRGRP (S_IREAD >> 3)
#endif /* S_IRGRP */
#ifndef S_IRUSR
# define S_IRUSR S_IREAD
#endif /* S_IRUSR */

#ifndef S_IWRITE
# define S_IWRITE 0000200
#endif /* S_IWRITE */
#ifndef S_IWOTH
# define S_IWOTH (S_IWRITE >> 6)
#endif /* S_IWOTH */
#ifndef S_IWGRP
# define S_IWGRP (S_IWRITE >> 3)
#endif /* S_IWGRP */
#ifndef S_IWUSR
# define S_IWUSR S_IWRITE
#endif /* S_IWUSR */

#ifndef S_IEXEC
# define S_IEXEC 0000100
#endif /* S_IEXEC */
#ifndef S_IXOTH
# define S_IXOTH (S_IEXEC >> 6)
#endif /* S_IXOTH */
#ifndef S_IXGRP
# define S_IXGRP (S_IEXEC >> 3)
#endif /* S_IXGRP */
#ifndef S_IXUSR
# define S_IXUSR S_IEXEC
#endif /* S_IXUSR */

#ifndef S_ISUID
# define S_ISUID 0004000 	/* setuid */
#endif /* S_ISUID */
#ifndef S_ISGID	
# define S_ISGID 0002000	/* setgid */
#endif /* S_ISGID */
#ifndef S_ISVTX
# define S_ISVTX 0001000	/* sticky */
#endif /* S_ISVTX */

/*
 * Access()
 */
#ifndef F_OK
# define F_OK 0
#endif /* F_OK */
#ifndef X_OK
# define X_OK 1
#endif /* X_OK */
#ifndef W_OK
# define W_OK 2
#endif /* W_OK */
#ifndef R_OK
# define R_OK 4
#endif /* R_OK */

/*
 * Open()
 */
#ifndef O_RDONLY
# define O_RDONLY	0
#endif /* O_RDONLY */
#ifndef O_WRONLY
# define O_WRONLY	1
#endif /* O_WRONLY */
#ifndef O_RDWR
# define O_RDWR		2
#endif /* O_RDWR */

/*
 * Lseek()
 */
#ifndef L_SET
# ifdef SEEK_SET
#  define L_SET		SEEK_SET
# else /* !SEEK_SET */
#  define L_SET		0
# endif	/* SEEK_SET */
#endif /* L_SET */
#ifndef L_INCR
# ifdef SEEK_CUR
#  define L_INCR	SEEK_CUR
# else /* !SEEK_CUR */
#  define L_INCR	1
# endif	/* SEEK_CUR */
#endif /* L_INCR */
#ifndef L_XTND
# ifdef SEEK_END
#  define L_XTND	SEEK_END
# else /* !SEEK_END */
#  define L_XTND	2
# endif /* SEEK_END */
#endif /* L_XTND */

#ifdef _SEQUENT_
# define NEEDgethostname
#endif /* _SEQUENT_ */

#if defined(BSD) && defined(POSIXJOBS) && !defined(__hp_osf)
# define setpgid(pid, pgrp)	setpgrp(pid, pgrp)
#endif /* BSD && POSIXJOBS && !__hp_osf */

#if defined(BSDJOBS) && !(defined(POSIX) && defined(POSIXJOBS))
# if !defined(_AIX370) && !defined(_AIXPS2)
#  define setpgid(pid, pgrp)	setpgrp(pid, pgrp)
# endif /* !_AIX370 && !_AIXPS2 */
# define NEEDtcgetpgrp
#endif /* BSDJOBS && !(POSIX && POSIXJOBS) */

#ifdef RENO 
/*
 * RENO has this broken. It is fixed on 4.4BSD
 */
# define NEEDtcgetpgrp
#endif /* RENO */

#ifdef DGUX
# define setpgrp(a, b) setpgrp2(a, b)
# define getpgrp(a) getpgrp2(a)
#endif /* DGUX */

#ifdef SXA
# ifndef _BSDX_
/*
 * Only needed in the system V environment.
 */
#  define setrlimit 	bsd_setrlimit
#  define getrlimit	bsd_getrlimit
# endif	/* _BSDX_ */
#endif /* SXA */

#if defined(_MINIX) || defined(__EMX__)
# define NEEDgethostname
# define NEEDnice
# define HAVENOLIMIT
/*
 * Minix does not have these, so...
 */
# define getpgrp		getpid
#endif /* _MINIX || __EMX__ */

#ifdef __EMX__
/* XXX: How can we get the tty name in emx? */
# define ttyname(fd) (isatty(fd) ? "/dev/tty" : NULL)
#endif /* __EMX__ */

#ifndef POSIX
# define mygetpgrp()    getpgrp(0)
#else /* POSIX */
# if defined(BSD) || defined(SUNOS4) || defined(IRIS4D) || defined(DGUX)
#  define mygetpgrp()    getpgrp(0)
# else /* !(BSD || SUNOS4 || IRIS4D || DGUX) */
#  define mygetpgrp()    getpgrp()
# endif	/* BSD || SUNOS4 || IRISD || DGUX */
#endif /* POSIX */


#if SYSVREL > 0 && !defined(OREO) && !defined(sgi) && !defined(linux)
# define NEEDgetwd
#endif /* SYSVREL > 0 && !OREO && !sgi && !linux */

#ifndef S_IFLNK
# define lstat stat
#endif /* S_IFLNK */


#if defined(BSDTIMES) && !defined(_SEQUENT_)
typedef struct timeval timeval_t;
#endif /* BSDTIMES && ! _SEQUENT_ */

#ifdef NeXT
/*
 * From Tony_Mason@transarc.com, override NeXT's malloc stuff.
 */
# define malloc tcsh_malloc
# define calloc tcsh_calloc
# define realloc tcsh_realloc
# define free tcsh_free
#endif /* NeXT */


#if !defined(POSIX) || defined(SUNOS4) || defined(UTekV) || defined(sysV88)
extern time_t time();
extern char *getenv();
extern int atoi();
# ifndef __EMX__
extern char *ttyname();
# endif /* __EMX__ */

# if defined(SUNOS4)
#  ifndef toupper
extern int toupper __P((int));
#  endif /* toupper */
#  ifndef tolower
extern int tolower __P((int));
#  endif /* tolower */
extern caddr_t sbrk __P((int));
#  if SYSVREL == 0 && !defined(__lucid)
extern int qsort();
#  endif /* SYSVREL == 0 && !__lucid */
# else /* !SUNOS4 */
#  ifndef hpux
#   if __GNUC__ != 2
extern int abort();
#   endif /* __GNUC__ != 2 */
#   ifndef fps500
extern int qsort();
#   endif /* !fps500 */
#  else /* !hpux */
extern void abort();
extern void qsort();
#  endif /* hpux */
# endif	/* SUNOS4 */
extern void perror();

# ifndef NEEDgethostname
extern int gethostname();
# endif /* NEEDgethostname */

# ifdef BSDSIGS
#  if defined(_AIX370) || defined(MACH) || defined(NeXT) || defined(_AIXPS2) || defined(ardent) || defined(SUNOS4)
extern int sigvec();
extern int sigpause();
#  else	/* !(_AIX370 || MACH || NeXT || _AIXPS2 || ardent || SUNOS4) */
#   if (!defined(apollo) || !defined(__STDC__)) && !defined(__DGUX__) && !defined(fps500)
extern sigret_t sigvec();
extern void sigpause();
#   endif /* (!apollo || !__STDC__) && !__DGUX__ && !fps500 */
#  endif /* _AIX370 || MACH || NeXT || _AIXPS2 || ardent || SUNOS4 */
extern sigmask_t sigblock();
extern sigmask_t sigsetmask();
# endif	/* BSDSIGS */

# ifndef killpg
extern int killpg();
# endif	/* killpg */

# ifndef lstat
extern int lstat();
# endif	/* lstat */

# ifdef BSD
extern uid_t getuid(), geteuid();
extern gid_t getgid(), getegid();
# endif /* BSD */

# ifdef SYSMALLOC
extern memalign_t malloc();
extern memalign_t realloc();
extern memalign_t calloc();
extern void free();
# endif	/* SYSMALLOC */

# ifdef BSDTIMES
extern int getrlimit();
extern int setrlimit();
extern int getrusage();
extern int gettimeofday();
# endif	/* BSDTIMES */

# if defined(NLS) && !defined(NOSTRCOLL) && !defined(NeXT)
extern int strcoll();
# endif /* NLS && !NOSTRCOLL && !NeXT */

# ifdef BSDJOBS
#  ifdef BSDTIMES
extern int wait3();
#  else	/* !BSDTIMES */
#   if !defined(POSIXJOBS) && !defined(_SEQUENT_)
extern int wait3();
#   else /* POSIXJOBS || _SEQUENT_ */
extern int waitpid();
#   endif /* POSIXJOBS || _SEQUENT_ */
#  endif /* BSDTIMES */
# else /* !BSDJOBS */
#  if SYSVREL < 3
extern int ourwait();
#  else	/* SYSVREL >= 3 */
extern int wait();
#  endif /* SYSVREL < 3 */
# endif	/* BSDJOBS */

# ifdef BSDNICE
extern int setpriority();
# else /* !BSDNICE */
extern int nice();
# endif	/* BSDNICE */

# if (!defined(fps500) && !defined(apollo) && !defined(__lucid))
extern void setpwent();
extern void endpwent();
# endif /* !fps500 && !apollo && !__lucid */

# ifndef __STDC__
extern struct passwd *getpwuid(), *getpwnam(), *getpwent();
#  ifdef PW_SHADOW
extern struct spwd *getspnam(), *getspent();
#  endif /* PW_SHADOW */
#  ifdef PW_AUTH
extern struct authorization *getauthuid();
#  endif /* PW_AUTH */
# endif /* __STDC__ */

# ifndef getwd
extern char *getwd();
# endif	/* getwd */

#else /* POSIX || !SUNOS4 || !UTekV || !sysV88 */

# if (defined(SUNOS4) && !defined(__GNUC__)) || defined(_IBMR2) || defined(_IBMESA)
extern char *getwd();
# endif	/* (SUNOS4 && ! __GNUC__) || _IBMR2 || _IBMESA */

# ifdef SCO
extern char *ttyname();   
# endif /* SCO */

#endif /* !POSIX || SUNOS4 || UTekV || sysV88 */

#if defined(SUNOS4) && __GNUC__ == 2
/*
 * Somehow these are missing
 */
extern int ioctl __P((int, int, ...));
extern int readlink __P((const char *, char *, size_t));
#endif /* SUNOS4 && __GNUC__ == 2 */

#if (defined(BSD) && !defined(__386BSD__)) || defined(SUNOS4) 
extern void bcopy	__P((const void *, void *, size_t));
# define memmove(a, b, c) (bcopy((char *) (b), (char *) (a), (int) (c)), a)
#endif /* (BSD && !__386BSD__) || SUNOS4 */

#if !defined(hpux) && !defined(COHERENT) && ((SYSVREL < 4) || defined(_SEQUENT_)) && !defined(__386BSD__) && !defined(memmove)
# define NEEDmemmove
#endif /* !hpux && !COHERENT && (SYSVREL < 4 || _SEQUENT_) && !__36BSD__ && !memmove */

#if SYSVREL == 4
extern int gethostname();
extern int getrlimit();
extern int setrlimit();
extern int getrusage();
# ifndef IRIS4D
extern int gettimeofday();
extern int wait3();
# endif
#endif /* SYSVREL == 4 */

#if defined(__osf__) && defined(__alpha)
extern int gethostname __P((char *, int));
extern void *sbrk __P((ssize_t));
extern int ioctl __P((int, unsigned long, char *));
extern pid_t vfork __P((void));
extern int killpg __P((pid_t, int));
extern char *getwd __P((char *));
#endif /* __osf__ && __alpha */

#endif /* _h_tc_os */
