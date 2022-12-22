/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Move a file, copy if link fails.
**
**	RCSID $Id: MoveCp.c,v 1.1.1.1 1992/09/28 20:08:42 trent Exp $
**
**	$Log: MoveCp.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:42  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	FILE_CONTROL

#include	"global.h"



bool
MoveCp(name1, name2)
	char *	name1;
	char *	name2;
{
	int	count = 0;

	Trace((2, "MoveCp(%s, %s)", name1, name2));

#	if	RENAME_2 != 1
	(void)unlink(name2);
	while ( link(name1, name2) == SYSERROR )
#	else	/* RENAME_2 != 1 */
	while ( rename(name1, name2) == SYSERROR )
#	endif	/* RENAME_2 != 1 */
	{
		if ( errno == EXDEV )
		{
			if ( !CopyFileToFile(name1, name2) )
				return false;

			(void)unlink(name1);
			return true;
		}

		if
		(
			!CheckDirs(name2)
			&&
			!SysWarn(english("Can't rename \"%s\" to \"%s\""), name1, name2)
		)
			return false;
	}

#	if	RENAME_2 != 1
	(void)unlink(name1);
#	endif	/* RENAME_2 != 1 */

	return true;
}
