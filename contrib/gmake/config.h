/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define for DGUX with <sys/dg_sys_info.h>.  */
/* #undef DGUX */

/* Define if you have dirent.h.  */
#define DIRENT 1

/* Define to the type of elements in the array set by `getgroups'.
   Usually this is either `int' or `gid_t'.  */
#define GETGROUPS_T gid_t

/* Define if the `getloadavg' function needs to be run setuid or setgid.  */
/* #undef GETLOADAVG_PRIVILEGED */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if you have alloca.h and it should be used (not Ultrix).  */
/* #undef HAVE_ALLOCA_H */

/* Define if your system has its own `getloadavg' function.  */
#define HAVE_GETLOADAVG 1

/* Define if you have the strcoll function and it is properly defined.  */
#define HAVE_STRCOLL 1

/* Define if `union wait' is the type of the first arg to wait functions.  */
#define HAVE_UNION_WAIT 1

/* Define if you have vfork.h.  */
/* #undef HAVE_VFORK_H */

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define if you don't have dirent.h, but have ndir.h.  */
/* #undef NDIR */

/* Define if your struct nlist has an n_un member.  */
/* #undef NLIST_NAME_UNION */

/* Define if you have nlist.h.  */
/* #undef NLIST_STRUCT */

/* Define if your C compiler doesn't accept -c and -o together.  */
/* #undef NO_MINUS_C_MINUS_O */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if the setvbuf function takes the buffering type as its second
   argument and the buffer pointer as the third, as on System V
   before release 3.  */
/* #undef SETVBUF_REVERSED */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
/* #undef STAT_MACROS_BROKEN */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define on System V Release 4.  */
/* #undef SVR4 */

/* Define if you don't have dirent.h, but have sys/dir.h.  */
/* #undef SYSDIR */

/* Define if you don't have dirent.h, but have sys/ndir.h.  */
/* #undef SYSNDIR */

/* Define if `sys_siglist' is declared by <signal.h>.  */
#define SYS_SIGLIST_DECLARED 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define for Encore UMAX.  */
/* #undef UMAX */

/* Define for Encore UMAX 4.3 that has <inq_status/cpustats.h>
   instead of <sys/cpustats.h>.  */
/* #undef UMAX4_3 */

/* Define vfork as fork if vfork does not work.  */
/* #undef vfork */

/* Define if the closedir function returns void instead of int.  */
/* #undef VOID_CLOSEDIR */

/* Define to the name of the SCCS `get' command.  */
#define SCCS_GET "get"

/* Define this if the SCCS `get' command understands the `-G<file>' option.  */
/* #undef SCCS_GET_MINUS_G */

/* Define if you have _sys_siglist.  */
/* #undef HAVE__SYS_SIGLIST */

/* Define if you have dup2.  */
#define HAVE_DUP2 1

/* Define if you have getcwd.  */
#define HAVE_GETCWD 1

/* Define if you have getdtablesize.  */
#define HAVE_GETDTABLESIZE 1

/* Define if you have getgroups.  */
#define HAVE_GETGROUPS 1

/* Define if you have psignal.  */
#define HAVE_PSIGNAL 1

/* Define if you have setlinebuf.  */
#define HAVE_SETLINEBUF 1

/* Define if you have setregid.  */
#define HAVE_SETREGID 1

/* Define if you have setreuid.  */
#define HAVE_SETREUID 1

/* Define if you have sigsetmask.  */
#define HAVE_SIGSETMASK 1

/* Define if you have strerror.  */
#define HAVE_STRERROR 1

/* Define if you have sys_siglist.  */
#define HAVE_SYS_SIGLIST 1

/* Define if you have wait3.  */
#define HAVE_WAIT3 1

/* Define if you have waitpid.  */
#define HAVE_WAITPID 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <mach/mach.h> header file.  */
/* #undef HAVE_MACH_MACH_H */

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/param.h> header file.  */
#define HAVE_SYS_PARAM_H 1

/* Define if you have the <sys/timeb.h> header file.  */
#define HAVE_SYS_TIMEB_H 1

/* Define if you have the <sys/wait.h> header file.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the dgc library (-ldgc).  */
/* #undef HAVE_DGC */

/* Define if you have the elf library (-lelf).  */
/* #undef HAVE_ELF */

/* Define if you have the getloadavg library (-lgetloadavg).  */
#define HAVE_GETLOADAVG 1

/* Define if you have the kvm library (-lkvm).  */
/* #undef HAVE_KVM */

/* Define if you have the seq library (-lseq).  */
/* #undef HAVE_SEQ */

/* Define if you have the sun library (-lsun).  */
/* #undef HAVE_SUN */

/* Define if you have the util library (-lutil).  */
/* #undef HAVE_UTIL */
