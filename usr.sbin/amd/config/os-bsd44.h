/*
 * Copyright (c) 1990 Jan-Simon Pendry
 * Copyright (c) 1990 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
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
 *	@(#)os-bsd44.h	5.3 (Berkeley) 5/12/91
 *
 * $Id: os-bsd44.h,v 1.3 1993/12/17 03:15:25 sanders Exp $
 *
 * 4.4 BSD definitions for Amd (automounter)
 */

/*
 * Does the compiler grok void *
 */
#define	VOIDP

/*
 * Which version of the Sun RPC library we are using
 * This is the implementation release number, not
 * the protocol revision number.
 */
#define	RPC_4

/*
 * Which version of the NFS interface are we using.
 * This is the implementation release number, not
 * the protocol revision number.
 */
#define	NFS_44
#define HAS_TCP_NFS

/*
 * Does this OS have NDBM support?
 */
#define OS_HAS_NDBM

/*
 * 4.4 doesn't provide NIS.
 */
#undef HAS_NIS_MAPS

/*
 * The mount table is obtained from the kernel
 */
#undef	UPDATE_MTAB

/*
 * No mntent info on 4.4 BSD
 */
#undef	MNTENT_HDR

/*
 * Name of filesystem types
 */
#define	MOUNT_TYPE_NFS	MOUNT_NFS
#define	MOUNT_TYPE_UFS	MOUNT_UFS
#undef MTAB_TYPE_UFS
#define	MTAB_TYPE_UFS	"ufs"
#define	MTAB_TYPE_MFS	"mfs"

/*
 * How to unmount filesystems
 */
#undef UNMOUNT_TRAP
#undef	NEED_UMOUNT_FS
#define	NEED_UMOUNT_BSD

/*
 * How to copy an address into an NFS filehandle
 */
#undef NFS_SA_DREF
#define	NFS_SA_DREF(dst, src) { \
		(dst).addr = (struct sockaddr *) (src); \
		(dst).sotype = SOCK_DGRAM; \
		(dst).proto = 0; \
	}

/*
 * Byte ordering
 */
#ifndef BYTE_ORDER
#include <machine/endian.h>
#endif /* BYTE_ORDER */

#undef ARCH_ENDIAN
#if BYTE_ORDER == LITTLE_ENDIAN
#define ARCH_ENDIAN "little"
#else
#if BYTE_ORDER == BIG_ENDIAN
#define ARCH_ENDIAN "big"
#else
XXX - Probably no hope of running Amd on this machine!
#endif /* BIG */
#endif /* LITTLE */

/*
 * Miscellaneous 4.4 BSD bits
 */
#define	NEED_MNTOPT_PARSER
#define	SHORT_MOUNT_NAME

#define	MNTMAXSTR       128

#define	MNTTYPE_UFS	"ufs"		/* Un*x file system */
#define	MNTTYPE_NFS	"nfs"		/* network file system */
#define	MNTTYPE_MFS	"mfs"		/* memory file system */
#define	MNTTYPE_IGNORE	"ignore"	/* No type specified, ignore this entry */

#define	M_RDONLY	MNT_RDONLY
#define	M_SYNC		MNT_SYNCHRONOUS
#define	M_NOEXEC	MNT_NOEXEC
#define	M_NOSUID	MNT_NOSUID
#define	M_NODEV		MNT_NODEV

#define	MNTOPT_SOFT	"soft"		/* soft mount */
#define	MNTOPT_INTR	"intr"		/* interrupts allowed */
#define	MNTOPT_NOCONN	"noconn"	/* don't connect socket */

struct mntent {
	char	*mnt_fsname;	/* name of mounted file system */
	char	*mnt_dir;	/* file system path prefix */
	char	*mnt_type;	/* MNTTYPE_* */
	char	*mnt_opts;	/* MNTOPT* */
	int	mnt_freq;	/* dump frequency, in days */
	int	mnt_passno;	/* pass number on parallel fsck */
};

/*
 * Type of a file handle
 */
#undef NFS_FH_TYPE
#define	NFS_FH_TYPE	nfsv2fh_t *

/*
 * How to get a mount list
 */
#undef	READ_MTAB_FROM_FILE
#define	READ_MTAB_BSD_STYLE

/*
 * The data for the mount syscall needs the path in addition to the
 * host name since that is the only source of information about the
 * mounted filesystem.
 */
#define	NFS_ARGS_NEEDS_PATH

/*
 * 4.4 has RE support built in
 */
#undef RE_HDR
#define RE_HDR <regexp.h>
