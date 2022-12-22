/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Create all directories in ``path'' that don't already exist.
**
**	(Assumes last component of `path' is a file.)
**
**	RCSID $Id: MkDirs.c,v 1.1.1.1 1992/09/28 20:08:39 trent Exp $
**
**	$Log: MkDirs.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:39  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	EXECUTE
#define	FILE_CONTROL
#define	STDIO
#define	SYS_STAT

#include	"global.h"


#ifndef	MKDIR_2
static int	MDUid;
static int	MDGid;
void		SetDirsUid();
#endif	/* MKDIR_2 */



bool
MkDirs(path, uid, gid)
	char *		path;
	int		uid;
	int		gid;
{
	register char *	cp = path;
	register int	i;
#	ifndef	MKDIR_2
	register char *	errs;
	ExBuf		eb;
#	endif	/* MKDIR_2 */
	struct stat	statb;

	Trace((2, "MkDirs(%s, %d, %d)", path, uid, gid));
	DODEBUG(errno=0);

	i = SpoolDirLen;

	if ( strncmp(cp, SPOOLDIR, i) == STREQUAL )
		cp += i;
	else
		cp += 1;

#	ifndef	MKDIR_2
	FIRSTARG(&eb.ex_cmd) = MKDIR;
#	else	/* MKDIR_2 */
	i = 0;
#	endif	/* MKDIR_2 */

	while ( (cp = strchr(cp, '/')) != NULLSTR )
	{
		*cp = '\0';

		if ( stat(path, &statb) == SYSERROR && errno == ENOENT )
		{
			Trace((1, "MkDirs => mkdir %s", path));
#			if	MKDIR_2 == 1
			while ( mkdir(path, DIR_MODE) == SYSERROR )
			{
				if ( SysWarn(CouldNot, "mkdir", path) )
					continue;
				*cp++ = '/';
				return false;
			}
			(void)chown(path, uid, gid);
			(void)chmod(path, DIR_MODE);	/* umask always wrong */
			i++;
#			else	/* MKDIR_2 == 1 */
			NEXTARG(&eb.ex_cmd) = newstr(path);
#			endif	/* MKDIR_2 == 1 */
		}
		DODEBUG(else {Trace((2, "MkDirs @ \"%s\", errno=%s", path, ErrnoStr(errno))); errno=0;});

		*cp++ = '/';
	}

#	ifndef	MKDIR_2

	Trace((2, "MkDirs => %s %d dirs", MKDIR, NARGS(&eb.ex_cmd)-1));

	if ( NARGS(&eb.ex_cmd) == 1 )
		return false;

	cp = KeepErrFile;
	KeepErrFile = NULLSTR;

	MDUid = uid;
	MDGid = gid;

	if ( (errs = Execute(&eb, SetDirsUid, ex_exec, SYSERROR)) != NULLSTR )
	{
		Warn(StringFmt, errs);
		free(errs);
	}

	KeepErrFile = cp;

	for ( i = NARGS(&eb.ex_cmd) ; --i > 0 ; )
		free(ARG(&eb.ex_cmd, i));

	if ( errs != NULLSTR )
		return false;

#	else	/* MKDIR_2 */

	if ( i == 0 )
		return false;

#	endif	/* MKDIR_2 */

	return true;
}



#ifndef	MKDIR_2
/*
**	Set network uid, so that directories have correct owner.
*/

void
SetDirsUid()
{
	int	om;

	(void)umask((~DIR_MODE)&0777);

	while ( setgid(MDGid) == SYSERROR && geteuid() == 0 )
		if ( !SysWarn(CouldNot, "setgid", "group") )
			break;

	while ( setuid(MDUid) == SYSERROR && geteuid() == 0 )
		SysError(CouldNot, "setuid", "user");
}
#endif	/* MKDIR_2 */
