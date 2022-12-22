/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: sysinfo.h,v 1.1 1992/08/20 18:33:06 karels Exp $
 */


#ifndef _SYSINFO_H_
#define _SYSINFO_H_

/* 
 * Information about the current system.
 * This structure includes only fairly static information
 * (set no later than boot time.)  Returned by getkerninfo().
 * The strings are copied out immediately after this structure.
 * The pointers are offsets from the start of the buffer
 * when returned by the kernel.
 */
struct sysinfo {
	/* machine info */
	char	*sys_machine;		/* generic machine type */
	char	*sys_model;		/* specific machine type/model */
	long	sys_ncpu;		/* number of cpus */
	long	sys_cpuspeed;		/* ~mips, per cpu */
	long	sys_hwflags;		/* additional info, see below */
	u_long	sys_physmem;		/* amount of physical memory, KB */
	u_long	sys_usermem;		/* amount of memory for procs, KB */
	u_long	sys_pagesize;		/* software page size */
	
	/* system info */
	char	*sys_ostype;		/* generic OS type: "BSD" */
	char	*sys_osrelease;		/* general OS release */
	long	sys_os_revision;	/* OS interface version number */
	long	sys_posix1_version;	/* POSIX.1 version number */
	char	*sys_version;		/* binary version info */

	/* system parameters */
	long	sys_hz;			/* time-of-day clock frequency */
	long	sys_phz;		/* profiling/stats clock frequency */
	int	sys_ngroup_max;		/* max number of groups */
	long	sys_argmax;		/* max arg list size */
	long	sys_openmax;		/* approx. max number of open files */
	long	sys_childmax;		/* approx. max number of procs */
	
	/* local info */
	struct	timeval sys_boottime;	/* time of last system boot */
	char	*sys_hostname;		/* specific host name */
	
	/* string values follow */
};

/*
 * values for sys_hwflags
 * Bits above 0xff are reserved for hardware-dependent features.
 */
#define	SYS_FPA		0x01		/* has floating-point accelerator */

#endif /* _SYSINFO_H_ */
