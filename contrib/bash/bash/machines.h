/* machines.h --
   Included file in the makefile that gets run through Cpp.  This file
   tells which machines have what features based on the unique machine
   identifier present in Cpp. */

/* **************************************************************** */
/*                                                                  */
/*                Global Assumptions (true for most systems).       */
/*                                                                  */
/* **************************************************************** */

/* We make some global assumptions here.  This can be #undef'ed in
   various machine specific entries. */

/* If this file is being processed with Gcc, then the user has Gcc. */
#if defined (__GNUC__) && !defined (NeXT)
#  if !defined (HAVE_GCC)
#    define HAVE_GCC
#  endif /* HAVE_GCC */
#endif /* __GNUC__ && !NeXT */

/* Assume that all machines have the getwd () system call.  We unset it
   for USG systems. */
#define HAVE_GETWD

/* Assume that all systems have a working getcwd () call.  We unset it for
   ISC systems. */
#define HAVE_GETCWD

/* Most (but not all) systems have a good, working version of dup2 ().
   For systems that don't have the call (HP/UX), and for systems
   that don't set the open-on-exec flag for the dup'ed file descriptors,
   (Sequents running Dynix, Ultrix), #undef HAVE_DUP2 in the machine
   description. */
#define HAVE_DUP2

/* Every machine that has Gcc has alloca as a builtin in Gcc.  If you are
   compiling Bash without Gcc, then you must have alloca in a library,
   in your C compiler, or be able to assemble or compile the alloca source
   that we ship with Bash. */
#define HAVE_ALLOCA

/* We like most machines to use the GNU Malloc routines supplied in the
   source code because they provide high quality error checking.  On
   some machines, our malloc () cannot be used (because of library
   conflicts, for example), and for those, you should specifically
   #undef USE_GNU_MALLOC in the machine description. */
#define USE_GNU_MALLOC

/* This causes the Gnu malloc library (from glibc) to be used. */
/* #define USE_GNU_MALLOC_LIBRARY */

/* Assume that every operating system supplies strchr () and strrchr ()
   in a standard library until proven otherwise. */
#define HAVE_STRCHR

/* **************************************************************** */
/*								    */
/*			Sun Microsystems Machines	      	    */
/*								    */
/* **************************************************************** */

#if defined (sun)
#  if defined (USGr4) || defined (__svr4__) || defined (__SVR4)
#    define Solaris
#  endif /* USGr4 || __svr4__ */

/* We aren't currently using GNU Malloc on Suns because of a bug in Sun's
   YP which bites us when Sun free ()'s an already free ()'ed address.
   When Sun fixes their YP, we can start using our winning malloc again. */
#  if !defined (Solaris)
/* #    undef USE_GNU_MALLOC */
#  endif /* !Solaris */

/* Most Sun systems have signal handler functions that are void. */
#  define VOID_SIGHANDLER

/* Most Sun systems have the following. */
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_VFPRINTF
#  define HAVE_GETGROUPS

#  if defined (mc68010)
#    define sun2
#    define M_MACHINE "sun2"
#  endif
#  if defined (mc68020)
#    define sun3
#    define M_MACHINE "sun3"
#  endif
#  if defined (sparc) || defined (__sparc__)
#    define sun4
#    define M_MACHINE "sparc"
#  endif
#  if defined (i386)
#    define Sun386i
#    define done386
#    define M_MACHINE "Sun386i"
#  endif /* i386 */

/* Check for SunOS4 or greater. */
#  if defined (HAVE_SHARED_LIBS)
#    if defined (Solaris)
#      define M_OS "SunOS5"
#      define SYSDEP_CFLAGS -DUSGr4 -DUSG -DSolaris -DOPENDIR_NOT_ROBUST
#      if !defined (HAVE_GCC)
#        define REQUIRED_LIBRARIES -ldl
#        define SYSDEP_LDFLAGS -Bdynamic
#      endif /* !HAVE_GCC */
       /* The following are only available thru the BSD compatability lib. */
#      undef HAVE_GETWD
#      undef HAVE_SETLINEBUF
#    else /* !Solaris */
#      define M_OS "SunOS4"
#      define SYSDEP_CFLAGS -DBSD_GETPGRP -DOPENDIR_NOT_ROBUST
#      define HAVE_DIRENT
#    endif /* !Solaris */
#  else /* !HAVE_SHARED_LIBS */
#    if !defined (sparc) && !defined (__sparc__)
#      undef VOID_SIGHANDLER
#    endif /* !sparc */
#    define M_OS "SunOS3"
#  endif /* !HAVE_SHARED_LIBS */
#endif /* sun */

/* **************************************************************** */
/*								    */
/*			DEC Machines (vax, decstations)   	    */
/*								    */
/* **************************************************************** */

/* ************************ */
/*                          */
/*     Alpha with OSF/1     */
/*                          */
/* ************************ */
#if defined (__alpha) || defined (alpha)
#  define M_MACHINE "alpha"
#  define M_OS "OSF1"
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_VFPRINTF
#  define HAVE_STRERROR
#  define HAVE_GETGROUPS
#  define VOID_SIGHANDLER
#  define USE_TERMCAP_EMULATION
#  if !defined (__GNUC__)
#    define SYSDEP_CFLAGS -DNLS -D_BSD
#  endif /* !__GNUC__ */
#  undef HAVE_ALLOCA
#  undef USE_GNU_MALLOC
#endif /* __alpha || alpha */

/* ************************ */
/*			    */
/*	    Ultrix	    */
/*			    */
/* ************************ */
#if defined (ultrix)
#  if defined (MIPSEL)
#    undef HAVE_ALLOCA_H
#    define M_MACHINE "MIPSEL"
#  else /* !MIPSEL */
#    define M_MACHINE "vax"
#  endif /* !MIPSEL */
#  define SYSDEP_CFLAGS -DBSD_GETPGRP -DTERMIOS_MISSING
#  define M_OS "Ultrix"
#  define HAVE_DIRENT
#  define VOID_SIGHANDLER
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_VFPRINTF
#  define HAVE_GETGROUPS
#  undef HAVE_DUP2
#endif /* ultrix */

/* ************************ */
/*			    */
/*	VAX 4.3 BSD	    */
/*			    */
/* ************************ */
#if defined (vax) && !defined (ultrix)
#  define M_MACHINE "vax"
#  define M_OS "Bsd"
#  define HAVE_SETLINEBUF
#  define HAVE_SYS_SIGLIST
#  define HAVE_GETGROUPS
#  define USE_VFPRINTF_EMULATION
#endif /* vax && !ultrix */

/* **************************************** */
/*					    */
/*		SGI Iris/IRIX	    	    */
/*					    */
/* **************************************** */
#if defined (sgi)
#  if defined (Irix3)
#    define M_OS "Irix3"
#    define MIPS_CFLAGS -real_frameptr -Wf,-XNl3072
#    undef HAVE_ALLOCA
#  endif /* Irix3 */
#  if defined (Irix4)
#    define M_OS "Irix4"
#    define MIPS_CFLAGS -Wf,-XNl3072
#  endif /* Irix4 */
#  if defined (Irix5)
#    define M_OS "Irix5"
#    define MIPS_CFLAGS -Wf,-XNl3072
#  endif /* Irix5 */
#  define M_MACHINE "sgi"
#  define HAVE_GETGROUPS
#  define VOID_SIGHANDLER
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_VFPRINTF
#  define REQUIRED_LIBRARIES -lsun
#  if defined (HAVE_GCC) || !defined (mips)
#    undef MIPS_CFLAGS
#    define MIPS_CFLAGS
#  endif /* HAVE_GCC || !mips */
#  define SGI_CFLAGS -DUSG -DPGRP_PIPE
#  define SYSDEP_CFLAGS SGI_CFLAGS MIPS_CFLAGS
#endif  /* sgi */

/* ************************ */
/*			    */
/* 	  NEC EWS 4800	    */
/*			    */
/* ************************ */
#if defined (nec_ews)
#  if defined (SYSTYPE_SYSV) || defined (USGr4)
#    define M_MACHINE "ews4800"
#    define M_OS USG
#    define HAVE_VFPRINTF
#    define VOID_SIGHANDLER
#    define HAVE_STRERROR
#    define HAVE_DUP2
#    undef HAVE_GETWD
#    undef HAVE_RESOURCE /* ? */
     /* Alloca requires either Gcc or cc with -lucb. */
#    if !defined (HAVE_GCC)
#      define EXTRA_LIB_SEARCH_PATH /usr/ucblib
#      define REQUIRED_LIBRARIES -lc -lucb
#    endif /* !HAVE_GCC */
#    if defined (MIPSEB)
#      if !defined (HAVE_GCC)
#        define SYSDEP_CFLAGS -Wf,-XNl3072 -DUSGr4 -DUSGr3 -D_POSIX_JOB_CONTROL
#      else
#        define SYSDEP_CFLAGS -DUSGr4 -DUSGr3 -D_POSIX_JOB_CONTROL
#      endif /* HAVE_GCC */
#    else /* !MIPSEB */
#      define SYSDEP_CFLAGS -DUSGr4
#    endif /* MIPSEB */
#  else /* !SYSTYPE_SYSV && !USGr4 */
#    define M_OS Bsd
#  endif /* !SYSTYPE_SYSV && !USGr4 */
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#endif /* nec_ews */

/* ************************ */
/*			    */
/*	    Sony	    */
/*			    */
/* ************************ */
#if defined (sony)
#  if defined (MIPSEB)
#    define M_MACHINE "MIPSEB"
#  else /* !MIPSEB */
#    define M_MACHINE "sony"
#  endif /* !MIPSEB */

#  if defined (SYSTYPE_SYSV) || defined (USGr4)
#    define M_OS "USG"
#    undef HAVE_GETWD
#    define HAVE_DIRENT
#    define HAVE_STRERROR
#    define HAVE_VFPRINTF
#    define VOID_SIGHANDLER
     /* Alloca requires either Gcc or cc with -lucb. */
#    if !defined (HAVE_GCC)
#      define EXTRA_LIB_SEARCH_PATH /usr/ucblib
#      define REQUIRED_LIBRARIES -lc -lucb
#    endif /* !HAVE_GCC */
#    if defined (MIPSEB)
#      if !defined (HAVE_GCC)
#        define SYSDEP_CFLAGS -Wf,-XNl3072 -DUSGr4
#      else
#        define SYSDEP_CFLAGS -DUSGr4
#      endif /* HAVE_GCC */
#    else /* !MIPSEB */
#      define SYSDEP_CFLAGS -DUSGr4
#    endif /* !MIPSEB */
#  else /* !SYSTYPE_SYSV && !USGr4 */
#    define M_OS "Bsd"
#    define SYSDEP_CFLAGS -DHAVE_UID_T
#  endif /* !SYSTYPE_SYSV && !USGr4 */
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#endif /* sony */

/* ******************************** */
/*				    */
/*	   MIPS RISC/os		    */
/*				    */
/* ******************************** */

/* Notes on compiling with "make":

   * Place /bsd43/bin in your PATH before /bin.
   * Use `$(CC) -E' instead of `/lib/cpp' in Makefile.
*/
#if defined (mips) && !defined (M_MACHINE)

#  if defined (MIPSEB)
#    define M_MACHINE "MIPSEB"
#  else /* !MIPSEB */
#    if defined (MIPSEL)
#      define M_MACHINE "MIPSEL"
#    else /* !MIPSEL */
#      define M_MACHINE "mips"
#    endif /* !MIPSEL */
#  endif /* !MIPSEB */

#  define M_OS "Bsd"

   /* Special things for machines from MIPS Co. */
#  define MIPS_CFLAGS -DOPENDIR_NOT_ROBUST -DPGRP_PIPE

#  if defined (HAVE_GCC)
#    define SYSDEP_CFLAGS MIPS_CFLAGS
#  else /* !HAVE_GCC */
#    define SYSDEP_CFLAGS -Wf,-XNl3072 -systype bsd43 MIPS_CFLAGS
#  endif /* !HAVE_GCC */
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_VFPRINTF
#  define HAVE_GETGROUPS
   /* This is actually present but unavailable in the BSD universe? */
#  undef HAVE_UNISTD_H
#  if !defined (HAVE_RESOURCE)
#    define HAVE_RESOURCE
#  endif /* !HAVE_RESOURCE */
   /* /usr/include/sys/wait.h appears not to work correctly, so why use it? */
#  undef HAVE_WAIT_H
#endif /* mips */

/* ************************ */
/*			    */
/*	  Pyramid	    */
/*			    */
/* ************************ */
#if defined (pyr)
#  define M_MACHINE "Pyramid"
#  define M_OS "Bsd"
#  if !defined (HAVE_GCC)
#    undef HAVE_ALLOCA
#  endif /* HAVE_GCC */
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
   /* #define HAVE_VFPRINTF */
#  define HAVE_GETGROUPS
#endif /* pyr */

/* ************************ */
/*			    */
/*	    IBMRT	    */
/*			    */
/* ************************ */
#if defined (ibm032)
#  define M_MACHINE "IBMRT"
#  define M_OS "Bsd"
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define USE_VFPRINTF_EMULATION
   /* Alloca requires either gcc or hc or pcc with -ma in SYSDEP_CFLAGS. */
#  if !defined (HAVE_GCC)
#    define SYSDEP_CFLAGS -ma -U__STDC__
#  endif /* !HAVE_GCC */
#  define HAVE_GETGROUPS
/* #define USE_GNU_TERMCAP */
#endif /* ibm032 */

/* **************************************************************** */
/*								    */
/*	  All Intel 386 Processor Machines are Defined Here!	    */
/*								    */
/* **************************************************************** */

#if defined (i386)

/* Sequent Symmetry running Dynix/ptx 2.x */
#  if !defined (done386) && defined (_SEQUENT_)
#    define done386
#    define M_MACHINE "Symmetry"
#    define M_OS "Dynix"
#    define SYSDEP_CFLAGS -DUSG -DUSGr3 -DHAVE_GETDTABLESIZE -DHAVE_SETDTABLESIZE
#    define HAVE_DIRENT
#    define HAVE_VFPRINTF
#    define VOID_SIGHANDLER
#    define REQUIRED_LIBRARIES -lPW -lseq
#    undef HAVE_GETWD
#    undef HAVE_RESOURCE
#    undef HAVE_ALLOCA
#  endif /* _SEQUENT_ */

/* Sequent Symmetry running Dynix (4.2 BSD) */
#  if !defined (done386) && defined (sequent)
#    define done386
#    define M_MACHINE "Symmetry"
#    define M_OS "Bsd"
#    define SYSDEP_CFLAGS -DCPCC -DHAVE_SETDTABLESIZE
#    define HAVE_SETLINEBUF
#    define HAVE_SYS_SIGLIST
#    define HAVE_GETGROUPS
#    define LD_HAS_NO_DASH_L
#    undef HAVE_DUP2
#  endif /* Sequent 386 */

/* NeXT 3.x on i386 */
#  if !defined (done386) && defined (NeXT)
#    define done386
#    define M_MACHINE "NeXT"
#    define M_OS Bsd
#    define HAVE_VFPRINTF
#    define HAVE_SYS_SIGLIST
#    define HAVE_GETGROUPS
#    define HAVE_STRERROR
#    define VOID_SIGHANDLER
#    undef HAVE_GETWD
#    undef HAVE_GETCWD
#    undef USE_GNU_MALLOC
#    define SYSDEP_CFLAGS -DHAVE_RESOURCE -DMKFIFO_MISSING
#  endif

/* Generic 386 clone running Mach (4.3 BSD-compatible). */
#  if !defined (done386) && defined (MACH)
#    define done386
#    define M_MACHINE "i386"
#    define M_OS "Bsd"
#    define HAVE_SETLINEBUF
#    define HAVE_SYS_SIGLIST
#    define HAVE_GETGROUPS
#  endif /* i386 && MACH */

/* AIX PS/2 1.[23] for the [34]86. */
#  if !defined (done386) && defined (aixpc)
#    define done386
#    define M_MACHINE "aixpc"
#    define M_OS "AIX"
#    define HAVE_VFPRINTF
#    define VOID_SIGHANDLER
#    if defined (AIX_13)	/* AIX PS/2 1.3 */
#      define REQUIRED_LIBRARIES -lc_s
#    else
#      define SYSDEP_CFLAGS -D_BSD
#      define REQUIRED_LIBRARIES -lbsd -lc_s
#    endif /* !AIX_13 */
#    define HAVE_GETGROUPS
#    if !defined (HAVE_GCC)
#      undef HAVE_ALLOCA
#      undef HAVE_ALLOCA_H
#    endif /* !HAVE_GCC */
#    define USE_TERMCAP_EMULATION
#  endif /* AIXPC i386 */

/* System V Release 4 on the 386 */
#  if !defined (done386) && defined (USGr4)
#    define done386
#    define M_MACHINE "i386"
#    define M_OS "USG"
#    define HAVE_DIRENT
#    define HAVE_SYS_SIGLIST
#    define HAVE_VFPRINTF
#    define VOID_SIGHANDLER
     /* Alloca requires either Gcc or cc with -lucb. */
#    if !defined (HAVE_GCC)
#      define EXTRA_LIB_SEARCH_PATH /usr/ucblib
#      define REQUIRED_LIBRARIES -lc -lucb
#    endif /* !HAVE_GCC */
#    define HAVE_GETGROUPS
#    if defined (USGr4_2)
#      define SYSDEP_CFLAGS -DUSGr4 -DUSGr4_2
#    else
#      define SYSDEP_CFLAGS -DUSGr4
#    endif /* ! USGr4_2 */
#    undef HAVE_GETWD
#  endif /* System V Release 4 on i386 */

/* 386 box running Interactive Unix 2.2 or greater. */
#  if !defined (done386) && defined (isc386)
#    define done386
#    define M_MACHINE "isc386"
#    define M_OS "USG"
#    define HAVE_DIRENT
#    define HAVE_VFPRINTF
#    define VOID_SIGHANDLER
#    define HAVE_GETGROUPS
#    define USE_TERMCAP_EMULATION
#    if defined (HAVE_GCC)
#      define SYSDEP_LDFLAGS -posix
#    else
#      define REQUIRED_LIBRARIES -lPW
#      define SYSDEP_LDFLAGS -Xp
#      define ISC_POSIX -Xp
#    endif
#    define ISC_SYSDEPS -DUSGr3 -DPGRP_PIPE -DHAVE_GETPW_DECLS -D_POSIX_SOURCE -DOPENDIR_NOT_ROBUST
#    if defined (__STDC__)
#      if defined (HAVE_GCC)
#        define ISC_EXTRA -DO_NDELAY=O_NONBLOCK
#      else
#        define ISC_EXTRA -Dmode_t="unsigned short" -DO_NDELAY=O_NONBLOCK
#      endif /* HAVE_GCC */
#    else
#      define ISC_EXTRA
#    endif /* __STDC__ */
#    define SYSDEP_CFLAGS ISC_SYSDEPS ISC_POSIX ISC_EXTRA
#    undef HAVE_GETWD
#    undef HAVE_GETCWD
#  endif /* isc386 */

/* Xenix386 machine (with help from Ronald Khoo <ronald@robobar.co.uk>). */
#  if !defined (done386) && defined (Xenix386)
#    define done386
#    define M_MACHINE "i386"
#    define M_OS "Xenix"
#    define XENIX_CFLAGS -DUSG -DUSGr3

#    if defined (XENIX_22)
#      define XENIX_EXTRA -DREVERSED_SETVBUF_ARGS
#      define REQUIRED_LIBRARIES -lx
#    else /* !XENIX_22 */
#      define HAVE_DIRENT
#      if defined (XENIX_23)
#        define XENIX_EXTRA
#        define REQUIRED_LIBRARIES -ldir
#      else /* !XENIX_23 */
#        define XENIX_EXTRA -xenix
#        define SYSDEP_LDFLAGS -xenix
#        define REQUIRED_LIBRARIES -ldir -l2.3
#      endif /* !XENIX_23 */
#    endif /* !XENIX_22 */

#    define SYSDEP_CFLAGS XENIX_CFLAGS XENIX_EXTRA
#    define HAVE_VFPRINTF
#    define VOID_SIGHANDLER
#    define ALLOCA_ASM x386-alloca.s
#    define ALLOCA_OBJ x386-alloca.o
#    undef HAVE_ALLOCA
#    undef HAVE_GETWD
#  endif /* Xenix386 */

/* SCO UNIX 3.2 chip@count.tct.com (Chip Salzenberg) */
#  if !defined (done386) && defined (M_UNIX)
#    define done386
#    define M_MACHINE "i386"
#    define M_OS "SCO"
#    define SCO_CFLAGS -DUSG -DUSGr3 -DNO_DEV_TTY_JOB_CONTROL -DPGRP_PIPE \
		       -DOPENDIR_NOT_ROBUST
#    if defined (SCOv4)
#      define SYSDEP_CFLAGS SCO_CFLAGS
#    else /* !SCOv4 */
#      define SYSDEP_CFLAGS SCO_CFLAGS -DBROKEN_SIGSUSPEND
#    endif /* !SCOv4 */
#    define HAVE_VFPRINTF
#    define VOID_SIGHANDLER
#    define HAVE_GETGROUPS
#    undef HAVE_GETWD
#    undef HAVE_RESOURCE
/* advice from wbader@cess.lehigh.edu and Eduard.Vopicka@vse.cz */
#    if !defined (HAVE_GCC)
#      define REQUIRED_LIBRARIES -lc_s -lc -lPW
#    else
#      define REQUIRED_LIBRARIES -lc_s -lc
#    endif /* !HAVE_GCC */
#  endif /* SCO Unix on 386 boxes. */

/* BSDI BSD/386 running on a 386 or 486. */
#  if !defined (done386) && defined (bsdi)
#    define done386
#    define M_MACHINE "i386"
#    define M_OS "BSD386"
#    define HAVE_SYS_SIGLIST
#    define HAVE_SETLINEBUF
#    define HAVE_GETGROUPS
#    define HAVE_VFPRINTF
#    define HAVE_STRERROR
#    define VOID_SIGHANDLER
#    define HAVE_DIRENT
#  endif /* !done386 && bsdi */

/* Jolitz 386BSD running on a 386 or 486. */
#  if !defined (done386) && defined (__386BSD__)
#    define done386
#    define M_MACHINE "i386"
#    define M_OS "_386BSD"
#    define HAVE_SYS_SIGLIST
#    define HAVE_SETLINEBUF
#    define HAVE_GETGROUPS
#    define HAVE_VFPRINTF
#    define HAVE_STRERROR
#    define VOID_SIGHANDLER
#    define HAVE_DIRENT
#  endif /* !done386 && __386BSD__ */

#  if !defined (done386) && (defined (__linux__) || defined (linux))
#    define done386
#    define M_MACHINE "i386"
#    define M_OS Linux
#    define SYSDEP_CFLAGS -DUSG -DUSGr3 -DHAVE_GETDTABLESIZE -DHAVE_BCOPY
#    define REQUIRED_LIBRARIES
#    define HAVE_GETGROUPS
#    define HAVE_STRERROR
#    define VOID_SIGHANDLER
#    define HAVE_SYS_SIGLIST
#    define HAVE_VFPRINTF
#    define HAVE_VARARGS_H
#    define SEARCH_LIB_NEEDS_SPACE
#    if defined (__GNUC__)
#      define HAVE_FIXED_INCLUDES
#    endif /* __GNUC__ */
#    undef USE_GNU_MALLOC
#    undef HAVE_SETLINEBUF
#    undef HAVE_GETWD
#  endif  /* !done386 && __linux__ */

/* Assume a generic 386 running Sys V Release 3. */
#  if !defined (done386)
#    define done386
#    define M_MACHINE "i386"
#    define M_OS "USG"
#    define SYSDEP_CFLAGS -DUSGr3
#    define HAVE_VFPRINTF
#    define VOID_SIGHANDLER
     /* Alloca requires either Gcc or cc with libPW.a */
#    if !defined (HAVE_GCC)
#      define REQUIRED_LIBRARIES -lPW
#    endif /* !HAVE_GCC */
#    undef HAVE_GETWD
#  endif /* Generic i386 Box running Sys V release 3. */
#endif /* All i386 Machines with an `i386' define in cpp. */

/* **************************************************************** */
/*			                                            */
/*	                 Alliant FX/800                             */
/*			                                            */
/* **************************************************************** */
/* Original descs flushed.  FX/2800 machine desc 1.13 bfox@ai.mit.edu.
   Do NOT use -O with the stock compilers.  If you must optimize, use
   -uniproc with fxc, and avoid using scc. */
#if defined (alliant)
#  define M_MACHINE "alliant"
#  define M_OS "Concentrix"
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#  define HAVE_VFPRINTF
#  define HAVE_RESOURCE
#  define VOID_SIGHANDLER
#  define HAVE_STRERROR
#  define USE_GNU_MALLOC
#  define LD_HAS_NO_DASH_L
#  define SYSDEP_CFLAGS -DTERMIOS_MISSING -DMKFIFO_MISSING -DBSD_GETPGRP -w
   /* Actually, Alliant does have unistd.h, but it defines _POSIX_VERSION,
      and doesn't supply a fully compatible job control package.  We just
      pretend that it doesn't have it. */
#  undef HAVE_UNISTD_H
#  undef HAVE_ALLOCA
#endif /* alliant */

/* **************************************************************** */
/*                                                                  */
/*            Motorola Delta series running System V R3V6/7         */
/*                                                                  */
/* **************************************************************** */
/* Contributed by Robert L. McMillin (rlm@ms_aspen.hac.com).  */

#if defined (m68k) && defined (sysV68)
#  define M_MACHINE "Delta"
#  define M_OS "USG"
#  define SYSDEP_CFLAGS -DUSGr3
#  define VOID_SIGHANDLER
#  define HAVE_VFPRINTF
#  define REQUIRED_LIBRARIES -lm881
#  undef HAVE_GETWD
#  undef HAVE_RESOURCE
#  undef HAVE_DUP2
#  undef HAVE_ALLOCA
#endif /* Delta series */

/* **************************************************************** */
/*								    */
/*		      Gould 9000 - UTX/32 R2.1A			    */
/*								    */
/* **************************************************************** */
#if defined (gould)		/* Maybe should be GOULD_PN ? */
#  define M_MACHINE "gould"
#  define M_OS "Bsd"
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#endif /* gould */

/* ************************ */
/*			    */
/*	    NeXT	    */
/*			    */
/* ************************ */
#if defined (NeXT) && !defined (M_MACHINE)
#  define M_MACHINE "NeXT"
#  define M_OS "Bsd"
#  define HAVE_VFPRINTF
#  define HAVE_SYS_SIGLIST
#  define HAVE_GETGROUPS
#  define HAVE_STRERROR
#  define VOID_SIGHANDLER
#  undef HAVE_GETWD
#  undef HAVE_GETCWD
#  define SYSDEP_CFLAGS -DHAVE_RESOURCE -DMKFIFO_MISSING
#  undef USE_GNU_MALLOC
#endif /* NeXT */

/* ************************ */
/*			    */
/*	hp9000 4.4 BSD	    */
/*			    */
/* ************************ */
#if defined (hp9000) && defined (__BSD_4_4__)
#  define M_MACHINE "hp9000"
#  define M_OS "BSD_4_4"
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#  define HAVE_STRERROR
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
#  undef HAVE_ALLOCA
#endif /* hp9000 && __BSD_4_4__ */

/* ************************ */
/*			    */
/*	hp9000 4.3 BSD	    */
/*			    */
/* ************************ */
#if defined (hp9000) && !defined (hpux) && !defined (M_MACHINE)
#  define M_MACHINE "hp9000"
#  define M_OS "Bsd"
#  undef HAVE_ALLOCA
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#  define USE_VFPRINTF_EMULATION
#endif /* hp9000 && !hpux */

/* ************************ */
/*                          */
/*          hpux            */
/*                          */
/* ************************ */
#if defined (hpux)

/* HPUX comes in several different flavors, from pre-release 6.2 (basically
   straight USG), to Posix compliant 9.0. */

   /* HP machines come in several processor types.
      They are distinguished here. */
#  if defined (hp9000s200) && !defined (hp9000s300)
#    define M_MACHINE "hp9000s200"
#  endif /* hp9000s200 */
#  if defined (hp9000s300) && !defined (M_MACHINE)
#    define M_MACHINE "hp9000s300"
#  endif /* hp9000s300 */
#  if defined (hp9000s500) && !defined (M_MACHINE)
#    define M_MACHINE "hp9000s500"
#  endif /* hp9000s500 */
#  if defined (hp9000s700) && !defined (M_MACHINE)
#    define M_MACHINE "hp9000s700"
#  endif /* hp9000s700 */
#  if defined (hp9000s800) && !defined (M_MACHINE)
#    define M_MACHINE "hp9000s800"
#  endif /* hp9000s800 */
#  if defined (hppa) && !defined (M_MACHINE)
#    define M_MACHINE "hppa"
#  endif /* hppa */

/* Define the OS as the particular type that we are using. */
/* This is for HP-UX systems earlier than HP-UX 6.2 -- no job control. */
#  if defined (HPUX_USG)
#    define M_OS "USG"
#    define HPUX_SYSDEP_CFLAGS -Dhpux
#    define REQUIRED_LIBRARIES -lPW -lBSD
#    undef HAVE_WAIT_H
#    define HPUX_EXTRA
#  else /* !HPUX_USG */

/* All of the other operating systems need HPUX to be defined. */
#    define HPUX_EXTRA -DHPUX

     /* HPUX 6.2 .. 6.5 require -lBSD for getwd (), and -lPW for alloca (). */
#    if defined (HPUX_6)
#      define M_OS "hpux_6"
#      define REQUIRED_LIBRARIES -lPW -lBSD
#      undef HAVE_ALLOCA
#      undef HAVE_WAIT_H
#    endif /* HPUX_6 */

     /* On HP-UX 7.x, we do not link with -lBSD, so we don't have getwd (). */
#    if defined (HPUX_7)
#      define M_OS "hpux_7"
#      define REQUIRED_LIBRARIES -lPW
#      undef HAVE_GETWD
#      undef USE_GNU_MALLOC
#    endif /* HPUX_7 */

     /* HP-UX 8.x systems do not have a working alloca () on all platforms.
	This can cause us problems, especially when globbing.  HP has the
	same YP bug as Sun, so we #undef USE_GNU_MALLOC. */
#    if defined (HPUX_8)
#      define M_OS "hpux_8"
#      undef HAVE_ALLOCA
#      undef HAVE_GETWD
#      undef USE_GNU_MALLOC
#    endif /* HPUX_8 */

     /* HP-UX 9.0 reportedly fixes the alloca problems present in the 8.0
        release.  If so, -lPW is required to include it. */
#    if defined (HPUX_9)
#      define M_OS "hpux_9"
#      define REQUIRED_LIBRARIES -lPW
#      undef HAVE_RESOURCE
#      undef HAVE_GETWD
#      undef USE_GNU_MALLOC
#    endif /* HPUX_9 */

#  endif /* !HPUX_USG */

   /* All of the HPUX systems that we have tested have the following. */
#  define HAVE_DIRENT
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
#  define HAVE_GETGROUPS
#  define HAVE_STRERROR
#  define USE_TERMCAP_EMULATION
#  define SEARCH_LIB_NEEDS_SPACE

#  if defined (HPUX_SYSDEP_CFLAGS)
#    define SYSDEP_CFLAGS HPUX_SYSDEP_CFLAGS HPUX_EXTRA
#  else /* !HPUX_SYSDEP_CFLAGS */
#    define SYSDEP_CFLAGS HPUX_EXTRA
#  endif /* !HPUX_SYSDEP_CFLAGS */

#endif /* hpux */

/* ************************ */
/*                          */
/*        HP OSF/1          */
/*                          */
/* ************************ */
#if defined (__hp_osf)
#  define M_MACHINE "HPOSF1"
#  define M_OS "OSF1"
#  define SYSDEP_CFLAGS -q no_sl_enable
#  define SYSDEP_LDFLAGS -q lang_level:classic
#  define REQUIRED_LIBRARIES -lPW
#  define HAVE_SETLINEBUF
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
#  define HAVE_GETGROUPS
#  define HAVE_STRERROR
#  undef HAVE_ALLOCA
#endif /* __hp_osf */

/* ************************ */
/*                          */
/*        KSR1 OSF/1        */
/*                          */
/* ************************ */
#if defined (__ksr1__)
#  define M_MACHINE "KSR1"
#  define M_OS "OSF1"
#  define HAVE_SETLINEBUF
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
#  define HAVE_GETGROUPS
#  define HAVE_STRERROR
#  define SYSDEP_CFLAGS -DHAVE_GETDTABLESIZE -DHAVE_BCOPY -DHAVE_UID_T
#  undef HAVE_ALLOCA
#  undef USE_GNU_MALLOC
#endif /* ksr1 */

/* ************************ */
/*                          */
/*   Intel Paragon - OSF/1  */
/*                          */
/* ************************ */
#if defined (__i860) && defined (__PARAGON__)
#  define M_MACHINE "Paragon"
#  define M_OS "OSF1"
#  define HAVE_GETGROUPS
#  define HAVE_SETLINEBUF
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
#  define HAVE_STRERROR
#  define HAVE_SYS_SIGLIST
#endif /* __i860 && __PARAGON__ */

/* ************************ */
/*			    */
/*	    Xenix286	    */
/*			    */
/* ************************ */
#if defined (Xenix286)
#  define M_MACHINE "i286"
#  define M_OS "Xenix"

#  define XENIX_CFLAGS -DUSG -DUSGr3

#  if defined (XENIX_22)
#    define XENIX_EXTRA -DREVERSED_SETVBUF_ARGS
#    define REQUIRED_LIBRARIES -lx
#  else /* !XENIX_22 */
#    define HAVE_DIRENT
#    if defined (XENIX_23)
#      define XENIX_EXTRA
#      define REQUIRED_LIBRARIES -ldir
#    else /* !XENIX_23 */
#      define XENIX_EXTRA -xenix
#      define SYSDEP_LDFLAGS -xenix
#      define REQUIRED_LIBRARIES -ldir -l2.3
#    endif /* !XENIX_23 */
#  endif /* !XENIX_22 */

#  define SYSDEP_CFLAGS XENIX_CFLAGS XENIX_EXTRA
#  undef HAVE_ALLOCA
#  undef HAVE_GETWD
#endif /* Xenix286 */

/* ************************ */
/*			    */
/*	    convex	    */
/*			    */
/* ************************ */
#if defined (convex)
#  define M_MACHINE "convex"
#  define M_OS "Bsd"
#  undef HAVE_ALLOCA
#  define HAVE_SETLINEBUF
#  define HAVE_SYS_SIGLIST
#  define HAVE_GETGROUPS
#endif /* convex */

/* ************************ */
/*                          */
/*          AIX/RT          */
/*                          */
/* ************************ */
#if defined (aix) && !defined (aixpc)
#  define M_MACHINE "AIXRT"
#  define M_OS "USG"
#  define HAVE_DIRENT
#  define HAVE_VFPRINTF
#  define HAVE_SYS_SIGLIST
#  define VOID_SIGHANDLER
#  define HAVE_GETGROUPS
#  define USE_TERMCAP_EMULATION
#  if !defined (HAVE_GCC)
#    define SYSDEP_CFLAGS -a -DNLS -DUSGr3 -DHAVE_BCOPY
#  else /* HAVE_GCC */
#    define SYSDEP_CFLAGS -DNLS -DUSGr3 -DHAVE_BCOPY
#  endif /* HAVE_GCC */
#  undef USE_GNU_MALLOC
#  undef HAVE_ALLOCA
#  undef HAVE_RESOURCE
#endif /* aix && !aixpc */

/* **************************************** */
/*					    */
/*		IBM RISC 6000		    */
/*					    */
/* **************************************** */
#if defined (RISC6000) || defined (_IBMR2)
#  define M_MACHINE "RISC6000"
#  define M_OS "AIX"
#  define HAVE_DIRENT
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
#  define USE_TERMCAP_EMULATION
#  define HAVE_GETGROUPS
#  define SYSDEP_CFLAGS -DNLS -DUSGr3 -DHAVE_BCOPY
#  undef HAVE_ALLOCA
#  undef HAVE_GETWD
#  undef USE_GNU_MALLOC
#endif /* RISC6000 */

/* **************************************** */
/*					    */
/*	u370 IBM AIX/370		    */
/*					    */
/* **************************************** */
#if defined (u370)
#  if defined (_AIX370)
#    define M_MACHINE "AIX370"
#    define M_OS "Bsd"
#    define REQUIRED_LIBRARIES -lbsd
#    define HAVE_SETLINEBUF
#    define HAVE_VFPRINTF
#    define SYSDEP_CFLAGS -D_BSD
#    define HAVE_GETGROUPS
#    define USE_TERMCAP_EMULATION
#    undef USE_GNU_MALLOC
#  endif /* _AIX370 */
#  if defined (USGr4) /* System V Release 4 on 370 series architecture. */
#    define M_MACHINE "uxp"
#    define M_OS "USG"
#    define HAVE_DIRENT
#    define HAVE_SYS_SIGLIST
#    define HAVE_VFPRINTF
#    define USE_GNU_MALLOC
#    define VOID_SIGHANDLER
#    if !defined (HAVE_GCC)
#      undef HAVE_ALLOCA
#      define EXTRA_LIB_SEARCH_PATH /usr/ucblib
#      define REQUIRED_LIBRARIES -lc -lucb
#    endif /* !HAVE_GCC */
#    define HAVE_GETGROUPS
#    define HAVE_RESOURCE
#    define SYSDEP_CFLAGS -DUSGr4
#    endif /* USGr4 */
#endif /* u370 */

/* ************************ */
/*			    */
/*	    ATT 3B	    */
/*			    */
/* ************************ */
#if defined (att3b) || defined (u3b2)
#  if defined (att3b)
#    define M_MACHINE "att3b"
#    define HAVE_SYS_SIGLIST
#  else /* !att3b */
#    define M_MACHINE "u3b2"
#  endif /* !att3b */
#  define M_OS "USG"
#  undef HAVE_GETWD
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
   /* For an AT&T Unix before V.3 take out the -DUSGr3 and the HAVE_DIRENT. */
#  define SYSDEP_CFLAGS -DUSGr3
#  define HAVE_DIRENT
   /* Alloca requires either Gcc or cc with libPW.a. */
#  if !defined (HAVE_GCC)
#    define REQUIRED_LIBRARIES -lPW
#  endif /* !HAVE_GCC */
#endif /* att3b */

/* ************************ */
/*			    */
/*	    ATT 386	    */
/*			    */
/* ************************ */
#if defined (att386)
#  define M_MACHINE "att386"
#  define M_OS "USG"
#  undef HAVE_GETWD
   /* Alloca requires either Gcc or cc with libPW.a. */
#  if !defined (HAVE_GCC)
#    define REQUIRED_LIBRARIES -lPW
#  endif /* HAVE_GCC */
#  define HAVE_SYS_SIGLIST
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
   /* For an AT&T Unix before V.3 take out the -DUSGr3 and the HAVE_DIRENT. */
#  define SYSDEP_CFLAGS -DUSGr3
#  define HAVE_DIRENT
#endif /* att386 */

/* ************************ */
/*			    */
/*	 ATT UNIX PC	    */
/*			    */
/* ************************ */
#if defined (unixpc)
#  define M_MACHINE "unixpc"
#  define M_OS "USG"
#  define HAVE_VFPRINTF
#  define HAVE_DIRENT
#  if defined (HAVE_GCC)
#    define REQUIRED_LIBRARIES -ldirent -shlib
#  else /* !HAVE_GCC */
#    define REQUIRED_LIBRARIES -ldirent
#  endif /* !HAVE_GCC */
#  undef HAVE_GETWD
#  undef HAVE_DUP2
#  undef VOID_SIGHANDLER
#  undef HAVE_WAIT_H
#endif /* unixpc */

/* ************************ */
/*			    */
/*	    Encore	    */
/*			    */
/* ************************ */
#if defined (MULTIMAX)
#  if defined (n16)
#    define M_MACHINE "Multimax32k"
#  else
#    define M_MACHINE "Multimax"
#  endif /* n16 */
#  if defined (UMAXV)
#    define M_OS "USG"
#    define REQUIRED_LIBRARIES -lPW
#    define SYSDEP_CFLAGS -DUSGr3
#    define HAVE_DIRENT
#    define HAVE_VFPRINTF
#    define USE_TERMCAP_EMULATION
#    define VOID_SIGHANDLER
#  else
#    if defined (CMU)
#      define M_OS "Mach"
#    else
#      define M_OS "Bsd"
#    endif /* CMU */
#    define HAVE_SYS_SIGLIST
#    define HAVE_STRERROR
#    define HAVE_SETLINEBUF
#  endif /* UMAXV */
#  define HAVE_GETGROUPS
#endif  /* MULTIMAX */

/* ******************************************** */
/*						*/
/*   Encore Series 91 (88K BCS w Job Control)	*/
/*						*/
/* ******************************************** */
#if defined (__m88k) && defined (__UMAXV__)
#  define M_MACHINE "Gemini"
#  define M_OS "USG"
#  define REQUIRED_LIBRARIES -lPW
#  define USE_TERMCAP_EMULATION
#  define HAVE_DIRENT
#  define HAVE_GETGROUPS
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
#  define SYSDEP_CFLAGS -q ext=pcc -D_POSIX_JOB_CONTROL -D_POSIX_VERSION \
			-Dmalloc=_malloc -Dfree=_free -Drealloc=_realloc
#endif  /* m88k && __UMAXV__ */

/* ******************************************** */
/*						*/
/*    System V Release 4 on the ICL DRS6000     */
/*						*/
/* ******************************************** */
#if defined (drs6000)
#  define M_MACHINE "drs6000"
#  define M_OS "USG"
#  define SYSDEP_CFLAGS -Xa -DUSGr4
#  define SEARCH_LIB_NEEDS_SPACE
#  define HAVE_DIRENT
#  define HAVE_SYS_SIGLIST
#  define HAVE_VFPRINTF
#  define HAVE_GETGROUPS
#  define HAVE_STRERROR
#  define VOID_SIGHANDLER
#  define USE_GNU_TERMCAP
#  if !defined (__GNUC__)
#    undef HAVE_ALLOCA
#  endif
#  undef HAVE_ALLOCA_H
#  undef USE_GNU_MALLOC
#endif /* drs6000 */

/* ******************************************** */
/*						*/
/*  System V Release 4 on the Commodore Amiga   */
/*						*/
/* ******************************************** */
#if defined (amiga)
#  define M_MACHINE "amiga"
#  define M_OS "USG"
#  define SYSDEP_CFLAGS -DUSGr4
#  if !defined (HAVE_GCC)
#    define EXTRA_LIB_SEARCH_PATH /usr/ucblib
#    define REQUIRED_LIBRARIES -lc -lucb
#  endif /* !HAVE_GCC */
#  define HAVE_DIRENT
#  define HAVE_SYS_SIGLIST
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
#  define HAVE_GETGROUPS
#  define HAVE_STRERROR
#  undef HAVE_GETWD
#  undef USE_GNU_MALLOC
#endif /* System V Release 4 on amiga */

/* ************************ */
/*			    */
/*	    clipper	    */
/*			    */
/* ************************ */
/* This is for the Orion 1/05 (A BSD 4.2 box based on a Clipper processor) */
#if defined (clipper) && !defined (M_MACHINE)
#  define M_MACHINE "clipper"
#  define M_OS "Bsd"
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#endif  /* clipper */

/* ******************************** */
/*				    */
/*    Integrated Solutions 68020?   */
/*				    */
/* ******************************** */
#if defined (is68k)
#  define M_MACHINE "is68k"
#  define M_OS "Bsd"
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#  define USE_VFPRINTF_EMULATION
#  undef HAVE_ALLOCA
#endif /* is68k */

/* ******************************** */
/*				    */
/*	   Omron Luna/Mach 2.5	    */
/*				    */
/* ******************************** */
#if defined (luna88k)
#  define M_MACHINE "Luna88k"
#  define M_OS "Bsd"
#  define HAVE_SYS_SIGLIST
#  define USE_GNU_MALLOC
#  define HAVE_SETLINEBUF
#  define HAVE_VFPRINTF
#  define HAVE_GETGROUPS
#  define HAVE_VFPRINTF
#endif /* luna88k */

/* ************************ */
/*			    */
/*   BBN Butterfly GP1000   */
/*   Mach 1000 v2.5	    */
/*			    */
/* ************************ */
#if defined (butterfly) && defined (BFLY1)
#define M_MACHINE "BBN Butterfly"
#define M_OS "Mach 1000"
#define HAVE_SETLINEBUF
#define HAVE_SYS_SIGLIST
#define HAVE_GETGROUPS
#define HAVE_VFPRINTF
#  ifdef BUILDING_MAKEFILE
MAKE = make
#  endif /* BUILDING_MAKEFILE */
#endif /* butterfly */

/* **************************************** */
/*					    */
/*	    Apollo/SR10.2/BSD4.3	    */
/*					    */
/* **************************************** */
/* This is for the Apollo DN3500 running SR10.2 BSD4.3 */
#if defined (apollo)
#  define M_MACHINE "apollo"
#  define M_OS "Bsd"
#  define SYSDEP_CFLAGS -D_POSIX_VERSION -D_INCLUDE_BSD_SOURCE \
			-D_INCLUDE_POSIX_SOURCE -DTERMIOS_MISSING \
			-DBSD_GETPGRP -Dpid_t=int
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#endif /* apollo */

/* ************************ */
/*			    */
/*	DG AViiON	    */
/*			    */
/* ************************ */
/* This is for the DG AViiON box (runs DG/UX with both AT&T & BSD features.) */
#if defined (__DGUX__) || defined (DGUX)
#  define M_OS "USG"
#  if !defined (_M88KBCS_TARGET)
#    define M_MACHINE "AViiON"
#    define SYSDEP_CFLAGS -D_DGUX_SOURCE -DPGRP_PIPE
#    define REQUIRED_LIBRARIES -ldgc
#  else /* _M88KBCS_TARGET */
#    define M_MACHINE "m88kBCS_AV"
#    define SYSDEP_CFLAGS -D_DGUX_SOURCE -D_M88K_SOURCE -DPGRP_PIPE
#    undef HAVE_RESOURCE
#  endif /* _M88KBCS_TARGET */
   /* DG/UX comes standard with Gcc. */
#  define HAVE_GCC
#  define HAVE_FIXED_INCLUDES
#  define HAVE_STRERROR
#  define HAVE_GETGROUPS
#  define VOID_SIGHANDLER
#  undef HAVE_GETWD
#  undef USE_GNU_MALLOC

/* If you want to build bash for M88K BCS compliance on a DG/UX 5.4
   or above system, do the following:
     - If you have built in this directory before run "make clean" to
       endure the Bash directory is clean.
     - Run "eval `sde-target m88kbcs`" to set the software development
       environment to build BCS objects.
     - Run "make".
     - Do "eval `sde-target default`" to reset the SDE. */
#endif /* __DGUX__ */

/* ************************ */
/*			    */
/*    Harris Night Hawk	    */
/*			    */
/* ************************ */
/* This is for the Harris Night Hawk family. */
#if defined (_CX_UX)
#  if defined (_M88K)
#    define M_MACHINE "nh4000"
#  else /* !_M88K */
#    if defined (hcx)
#      define M_MACHINE "nh2000"
#    else /* !hcx */
#      if defined (gcx)
#        define M_MACHINE "nh3000"
#      endif /* gcx */
#    endif /* !hcx */
#  endif /* !_M88K */
#  define M_OS "USG"
#  define SYSDEP_CFLAGS -g -Xa -v -Dgetwd=bash_getwd -D_POSIX_SOURCE \
			-D_POSIX_JOB_CONTROL
#  define USE_TERMCAP_EMULATION
#  define HAVE_VFPRINTF
#  define HAVE_GETGROUPS
#  define VOID_SIGHANDLER
#  undef USE_GNU_MALLOC
#  undef HAVE_GETWD
#endif /* _CX_UX */

/* **************************************** */
/*					    */
/*	    	Tektronix	    	    */
/*					    */
/* **************************************** */
/* These are unproven as yet. */
#if defined (Tek4132)
#  define M_MACHINE "Tek4132"
#  define M_OS "Bsd"
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#endif /* Tek4132 */

#if defined (Tek4300)
#  define M_MACHINE "Tek4300"
#  define M_OS "Bsd"
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#endif /* Tek4300 */

/* ************************ */
/*                          */
/*     Tektronix XD88       */
/*                          */
/* ************************ */
#if defined (m88k) && defined (XD88)
#  define M_MACHINE "XD88"
#  define M_OS "USG"
#  define HAVE_DIRENT
#  define HAVE_VFPRINTF
#  define HAVE_GETCWD
#  define VOID_SIGHANDLER
#  define HAVE_GETGROUPS
#  undef HAVE_GETWD
#  undef HAVE_ALLOCA
#endif /* m88k && XD88 */

/* ************************ */
/*                          */
/*     Motorola M88100      */
/*                          */
/* ************************ */
#if defined (m88k) && (defined (M88100) || defined (USGr4))
#  define M_MACHINE "M88100"
#  define M_OS "USG"
#  if defined (USGr4)
#    define SYSDEP_CFLAGS -DUSGr4 -D_POSIX_JOB_CONTROL
#  else
#    define SYSDEP_CFLAGS -D_POSIX_JOB_CONTROL
#  endif
#  define HAVE_DIRENT
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
#  define HAVE_GETGROUPS
#  undef HAVE_GETWD
#  undef HAVE_GETCWD
#  undef HAVE_ALLOCA
#endif /* m88k && M88100 */

/* ************************ */
/*			    */
/*     Sequent Balances     */
/*       (Dynix 3.x)	    */
/* ************************ */
#if defined (sequent) && !defined (M_MACHINE)
#  define M_MACHINE "Sequent"
#  define M_OS "Bsd"
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#  define LD_HAS_NO_DASH_L
#  undef HAVE_DUP2
#endif /* sequent */

/* ****************************************** */
/*					      */
/*    NCR Tower 32, System V Release 3	      */
/*					      */
/* ****************************************** */
#if defined (tower32)
#  define M_MACHINE "tower32"
#  define M_OS "USG"
#  if !defined (HAVE_GCC)
#    define REQUIRED_LIBRARIES -lPW
     /* Disable stack/frame-pointer optimization, incompatible with alloca */
#    define SYSDEP_CFLAGS -DUSGr3 -W2,-aat
#  else /* HAVE_GCC */
#    define SYSDEP_CFLAGS -DUSGr3
#  endif /* HAVE_GCC */
#  define HAVE_VFPRINTF
#  define USE_TERMCAP_EMULATION
#  define VOID_SIGHANDLER
#  undef HAVE_GETWD
#endif /* tower32 */

/* ************************ */
/*			    */
/*    Ardent Titan OS v2.2  */
/*			    */
/* ************************ */
#if defined (ardent)
#  define M_MACHINE "Ardent Titan"
#  define M_OS "Bsd"
#  if !defined (titan)
#    define HAVE_GETGROUPS
#  endif /* !titan */
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define SYSDEP_CFLAGS -43 -w
#  define SYSDEP_LDFLAGS -43
#  undef HAVE_ALLOCA
#  undef USE_GNU_MALLOC
#  undef HAVE_VFPRINTF
#endif /* ardent */

/* ************************ */
/*			    */
/*	  Stardent	    */
/*			    */
/* ************************ */
#if defined (stardent) && !defined (M_MACHINE)
#  define M_MACHINE "Stardent"
#  define M_OS "USG"
#  define HAVE_SYS_SIGLIST
#  define USE_TERMCAP_EMULATION
#  define VOID_SIGHANDLER
#  undef HAVE_GETWD
#  undef HAVE_ALLOCA
#endif /* stardent */

/* ************************ */
/*			    */
/*	Concurrent	    */
/*			    */
/* ************************ */
#if defined (concurrent)
   /* Use the BSD universe (`universe ucb') */
#  define M_MACHINE "Concurrent"
#  define M_OS "Bsd"
#  define HAVE_SYS_SIGLIST
#  define HAVE_SETLINEBUF
#  define HAVE_GETGROUPS
#endif /* concurrent */

/* **************************************************************** */
/*                                                                  */
/*             Honeywell Bull X20 (lele@idea.sublink.org)	    */
/*                                                                  */
/* **************************************************************** */
#if defined (hbullx20)
#  define M_MACHINE "Honeywell"
#  define M_OS "USG"
#  define SYSDEP_CFLAGS -DUSG
   /* Bull x20 needs -lposix for struct dirent. */
#  define REQUIRED_LIBRARIES -lPW -lposix
#  define HAVE_DIRENT
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
#  define USE_TERMCAP_EMULATION
#  undef HAVE_GETWD
#endif  /* hbullx20 */

/* ************************ */
/*			    */
/*	    CRAY	    */
/*			    */
/* ************************ */
#if defined (cray)
#  if defined (Cray1) || defined (Cray2)
#    define M_MACHINE "Cray"
#    define CRAY_STACK
#  endif
#  if defined (CrayXMP) && !defined (M_MACHINE)
#    define M_MACHINE "CrayXMP"
#    define CRAY_STACK -DCRAY_STACKSEG_END=getb67
#  endif
#  if defined (CrayYMP) && !defined (M_MACHINE)
#    define M_MACHINE "CrayYMP"
#    define CRAY_STACK -DCRAY_STACKSEG_END=getb67
#  endif
#  if !defined (M_MACHINE)
#    define M_MACHINE "Cray"
#    define CRAY_STACK
#  endif
#  define M_OS "Unicos"
#  define SYSDEP_CFLAGS -DUSG -DPGRP_PIPE -DOPENDIR_NOT_ROBUST CRAY_STACK
#  define HAVE_VFPRINTF
#  define HAVE_MULTIPLE_GROUPS
#  define VOID_SIGHANDLER
#  define USE_TERMCAP_EMULATION
#  undef HAVE_ALLOCA
#  undef HAVE_RESOURCE
#  undef USE_GNU_MALLOC
#endif /* cray */

/* ************************ */
/*			    */
/*	MagicStation	    */
/*			    */
/* ************************ */
#if defined (MagicStation)
#  define M_MACHINE "MagicStation"
#  define M_OS "USG"
#  define SYSDEP_CFLAGS -DUSGr4
#  define HAVE_DIRENT
#  define HAVE_GETGROUPS
#  define HAVE_STRERROR
#  define VOID_SIGHANDLER
#  undef HAVE_ALLOCA
#  undef HAVE_GETWD
#endif /* MagicStation */

/* ************************ */
/*			    */
/*	   Plexus	    */
/*			    */
/* ************************ */
#if defined (plexus)
#  define M_MACHINE "plexus"
#  define M_OS "USG"
#  define REQUIRED_LIBRARIES -lndir
#  define USE_TERMCAP_EMULATION
#  undef HAVE_DUP2
#  undef HAVE_GETWD
#  define HAVE_VFPRINTF
#  undef HAVE_ALLOCA		/* -lPW doesn't work w/bash-cc? */
#endif /* plexus */

/* ************************ */
/*			    */   
/*     Siemens MX500        */
/*      (SINIX 5.2x)	    */
/* ************************ */
#ifdef sinix
#define M_MACHINE "Siemens MX500"
#define M_OS "SINIX V5.2x"
#define USG

#define HAVE_GETCWD
#define VOID_SIGHANDLER
#define HAVE_STRERROR
#define HAVE_GETGROUPS
#define HAVE_VFPRINTF
#define HAVE_POSIX_SIGNALS
#define HAVE_RESOURCE
#define USE_GNU_MALLOC
#define SYSDEP_CFLAGS -DUSGr3 -DUSG
#define REQUIRED_LIBRARIES syscalls.o
#undef HAVE_ALLOCA
#undef HAVE_GETWD
#endif /* sinix */

/* ************************ */
/*			    */
/*  Symmetric 375 (4.2 BSD) */
/*			    */
/* ************************ */
#if defined (scs) && !defined (M_MACHINE)
#  define M_MACHINE "Symmetric_375"
#  define M_OS "Bsd"
#  define HAVE_SYS_SIGLIST
#  define HAVE_GETGROUPS
#  define HAVE_SETLINEBUF
#  define USE_VFPRINTF_EMULATION
#  define USE_GNU_MALLOC
#  undef HAVE_STRCHR
#endif /* scs */

/* ************************ */
/*			    */
/*    PCS Cadmus System	    */
/*			    */
/* ************************ */
#if defined (cadmus) && !defined (M_MACHINE)
#  define M_MACHINE "cadmus"
#  define M_OS "BrainDeath"
#  define SYSDEP_CFLAGS -DUSG
#  define HAVE_DIRENT
#  define HAVE_VFPRINTF
#  define VOID_SIGHANDLER
#  define USE_TERMCAP_EMULATION
#  undef HAVE_GETWD
#  undef HAVE_ALLOCA
#  undef HAVE_WAIT_H
#endif /* cadmus */

/* **************************************************************** */
/*								    */
/*			Generic Entry   			    */
/*								    */
/* **************************************************************** */

/* Use this entry for your machine if it isn't represented here.  It
   is loosely based on a Vax running Bsd. */

#if !defined (M_MACHINE)
#  define UNKNOWN_MACHINE
#endif

#if defined (UNKNOWN_MACHINE)
#  define M_MACHINE "UNKNOWN_MACHINE"
#  define M_OS "UNKNOWN_OS"

/* Required libraries for building on this system. */
#  define REQUIRED_LIBRARIES

/* Define HAVE_SYS_SIGLIST if your system has sys_siglist[]. */
#  define HAVE_SYS_SIGLIST

/* Undef HAVE_ALLOCA if you are not using Gcc, and neither your library
   nor compiler has a version of alloca ().  In that case, we will use
   our version of alloca () in alloca.c */
/* #undef HAVE_ALLOCA */

/* Undef USE_GNU_MALLOC if there appear to be library conflicts, or if you
   especially desire to use your OS's version of malloc () and friends.  We
   reccommend against this because GNU Malloc has debugging code built in. */
#  define USE_GNU_MALLOC

/* Define USE_GNU_TERMCAP if you want to use the GNU termcap library
   instead of your system termcap library. */
/* #define USE_GNU_TERMCAP */

/* Define HAVE_SETLINEBUF if your machine has the setlinebuf ()
   stream library call.  Otherwise, setvbuf () will be used.  If
   neither of them work, you can edit in your own buffer control
   based upon your machines capabilities. */
#  define HAVE_SETLINEBUF

/* Define HAVE_VFPRINTF if your machines has the vfprintf () library
   call.  Otherwise, printf will be used.  */
#  define HAVE_VFPRINTF

/* Define USE_VFPRINTF_EMULATION if you want to use the BSD-compatible
   vfprintf() emulation in vprint.c. */
/* #  define USE_VFPRINTF_EMULATION */

/* Define HAVE_GETGROUPS if your OS allows you to be in multiple
   groups simultaneously by supporting the `getgroups' system call. */
/* #define HAVE_GETGROUPS */

/* Define SYSDEP_CFLAGS to be the flags to cc that make your compiler
   work.  For example, `-ma' on the RT makes alloca () work. */
/* This is a summary of the semi-machine-independent definitions that
   can go into SYSDEP_CFLAGS:

	USGr4	-	The machine is running SVR4
	USGr4_2 -	The machine is running SVR4.2
	USG	-	The machine is running some sort of System V Unix
	AFS	-	The Andrew File System is being used
	BSD_GETPGRP -	getpgrp(2) takes a pid argument, a la 4.3 BSD
	TERMIOS_MISSING - the termios(3) functions are not present or don't
			  work, even though _POSIX_VERSION is defined
	PGRP_PIPE -	Requires parent-child synchronization via pipes to
			make job control work right
	HAVE_UID_T -	Definitions for uid_t and gid_t are in <sys/types.h>
	HAVE_GETDTABLESIZE - getdtablesize(2) exists and works correctly
	HAVE_SETDTABLESIZE - setdtablesize(2) exists and works correctly
	HAVE_RESOURCE -	<sys/resource.h> and [gs]rlimit exist and work
	MKFIFO_MISSING - named pipes do not work or mkfifo(3) is missing
	REVERSED_SETVBUF_ARGS - brain-damaged implementation of setvbuf that
				has args 2 and 3 reversed from the SVID and
				ANSI standard
	NO_DEV_TTY_JOB_CONTROL - system can't do job control on /dev/tty
	BROKEN_SIGSUSPEND - sigsuspend(2) does not work to wake up processes
			    on SIGCHLD
	HAVE_BCOPY -	bcopy(3) exists and works as in BSD
	HAVE_GETPW_DECLS - USG machines with the getpw* functions defined in
			   <pwd.h> that cannot handle redefinitions in the
			   bash source
	OPENDIR_NOT_ROBUST - opendir(3) allows you to open non-directory files
*/
#  define SYSDEP_CFLAGS

/* Define HAVE_STRERROR if your system supplies a definition for strerror ()
   in the C library, or a macro in a header file. */
/* #define HAVE_STRERROR */

/* Define HAVE_DIRENT if you have the dirent library and a definition of
   struct dirent.  If not, the BSD directory reading library and struct
   direct are assumed. */
/* #define HAVE_DIRENT */

/* If your system does not supply /usr/lib/libtermcap.a, but includes
   the termcap routines as a part of the curses library, then define
   this.  This is the case on some System V machines. */
/* #define USE_TERMCAP_EMULATION */

/* Define VOID_SIGHANDLER if your system's signal () returns a pointer to
   a function returning void. */
/* #define VOID_SIGHANDLER */

/* Define EXTRA_LIB_SEARCH_PATH if your required libraries (or standard)
   ones for that matter) are not normally in the ld search path.  For
   example, some machines require /usr/ucblib in the ld search path so
   that they can use -lucb. */
/* #define EXTRA_LIB_SEARCH_PATH /usr/ucblib */

/* Define SEARCH_LIB_NEEDS_SPACE if your native ld requires a space after
   the -L argument, which gives the name of an alternate directory to search
   for libraries specified with -llib.  For example, the HPUX ld requires
   this:
   	-L lib/readline -lreadline
   instead of:
   	-Llib/readline -lreadline
 */
/* #define SEARCH_LIB_NEEDS_SPACE */

/* Define LD_HAS_NO_DASH_L if your ld can't grok the -L flag in any way, or
   if it cannot grok the -l<lib> flag, or both. */
/* #define LD_HAS_NO_DASH_L */
#  if defined (LD_HAS_NO_DASH_L)
#   undef SEARCH_LIB_NEEDS_SPACE
#  endif /* LD_HAS_NO_DASH_L */

#endif  /* UNKNOWN_MACHINE */
