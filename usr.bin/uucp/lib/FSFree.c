/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Return true if enough space left on f/s for "file" to hold "space".
**
**	RCSID $Id: FSFree.c,v 1.1.1.1 1992/09/28 20:08:42 trent Exp $
**
**	$Log: FSFree.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:42  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.2  1992/09/10  14:33:35  ziegast
 * CPP modifications for 4.3+BSD
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	<sys/param.h>

#define	SYS_STAT

#include	"global.h"

#if	SYSV != 0 && SYS_VFS != 1
#include	<ustat.h>

#ifndef	FS_BSIZE
#define	FS_BSIZE	512
#endif	/* FS_BSIZE */

int	FreeBSize	= FS_BSIZE;
#endif	/* SYSV != 0 && SYS_VFS != 1 */

#if	BSD4 == 2 || SYS_VFS == 1
#if	SYSV != 0
#include	<sys/statvfs.h>
#define	statfs	statvfs
#else	/* SYSV != 0 */
#include	<sys/vfs.h>
#endif	/* SYSV != 0 */
#endif	/* BSD4 == 2 || SYS_VFS == 1 */

#if	BSD4 >= 3 && SYS_VFS != 1
#include	<sys/mount.h>

int	FreeBSize	= 1024;
#endif	/* BSD4 >= 3 && SYS_VFS != 1 */

long	FreeFSpace;



bool
FSFree(file, space)
	char *			file;	/* On file-system needed */
	Ulong			space;	/* Bytes needed */
{
#	if	SYSV != 0 && SYS_VFS != 1
	struct ustat		buf;
	struct stat		statb;

	while ( stat(file, &statb) == SYSERROR )
		SysError(CouldNot, StatStr, file);

	if ( ustat(statb.st_dev, &buf) == SYSERROR )	/* Happens on flakey NFS! */
	{
		if ( Traceflag )
			(void)SysWarn(CouldNot, "ustat", file);
		FreeFSpace = MAX_LONG;
		return true;
	}

	if ( (FreeFSpace = buf.f_tfree * FreeBSize - MINSPOOLFSFREE * 1024) < 0 )
		FreeFSpace = 0;

	Trace((2, 
		"FSFree(%s, %lu): dev=%lu, bsize=%ld, f_tfree=%ld, free %ld",
		file,
		space,
		(Ulong)statb.st_dev,
		FreeBSize,
		buf.f_tfree,
		FreeFSpace
	));

	if ( FreeFSpace <= space )
	{
		Trace((1, "FSFree(%s, %lu) LOW FREE SPACE %ld", file, space, FreeFSpace));
		return false;
	}
#	endif	/* SYSV != 0 && SYS_VFS != 1 */

#	if	BSD4 == 2 || BSD4 >= 4 || SYS_VFS == 1
	struct statfs	buf;

	if ( statfs(file, &buf) == SYSERROR )	/* Happens on flakey NFS! */
	{
		if ( Traceflag )
			(void)SysWarn(CouldNot, "statfs", file);
		FreeFSpace = MAX_LONG;
		return true;
	}

#	if	SYSV != 0
	if ( (FreeFSpace = buf.f_bavail * buf.f_frsize - MINSPOOLFSFREE * 1024) < 0 )
		FreeFSpace = 0;

	Trace((2, 
		"FSFree(%s, %lu): f_bsize=%ld, f_frsize=%ld, f_bavail=%ld, free %ld",
		file,
		space,
		buf.f_bsize,
		buf.f_frsize,
		buf.f_bavail,
		FreeFSpace
	));
#	else	/* SYSV != 0 */

	if ( (FreeFSpace = buf.f_bavail * buf.f_bsize - MINSPOOLFSFREE * 1024) < 0 )
		FreeFSpace = 0;

	Trace((2, 
		"FSFree(%s, %lu): f_bsize=%ld, f_bavail=%ld, free %ld",
		file,
		space,
		buf.f_bsize,
		buf.f_bavail,
		FreeFSpace
	));
#	endif	/* SYSV != 0 */

	if ( FreeFSpace <= space )
	{
		Trace((1, "FSFree(%s, %lu) LOW FREE SPACE %ld", file, space, FreeFSpace));
		return false;
	}
#	endif	/* BSD4 == 2 || BSD4 >= 4 || SYS_VFS == 1 */

#	if	BSD4 == 3 && SYS_VFS != 1
	struct fs_data	buf;

	if ( statfs(file, &buf) == SYSERROR )	/* Happens on flakey NFS! */
	{
		if ( Traceflag )
			(void)SysWarn(CouldNot, "statfs", file);
		FreeFSpace = MAX_LONG;
		return true;
	}

	/* Whatever else, `fd_bsize' is not the size of `fd_bfreen'! */

	if ( (FreeFSpace = buf.fd_bfreen * FreeBSize - MINSPOOLFSFREE * 1024) < 0 )
		FreeFSpace = 0;

	Trace((2, 
		"FSFree(%s, %lu): bsize=%ld, fd_bfreen=%ld, free %ld",
		file,
		space,
		FreeBSize,
		buf.fd_bfreen,
		FreeFSpace
	));

	if ( FreeFSpace <= space )
	{
		Trace((1, "FSFree(%s, %lu) LOW FREE SPACE %ld", file, space, FreeFSpace));
		return false;
	}
#	endif	/* BSD4 >= 3 && SYS_VFS != 1 */

	return true;
}
