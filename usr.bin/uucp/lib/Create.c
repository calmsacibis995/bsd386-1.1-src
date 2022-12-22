/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	File manipulation.
**
**	RCSID $Id: Create.c,v 1.1.1.1 1992/09/28 20:08:39 trent Exp $
**
**	$Log: Create.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:39  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILE_CONTROL

#include	"global.h"



/*
**	Create a file for writing.
*/

int
Create(
	char *	file
)
{
	int	fd;

	Trace((2, "Create(%s)", file));

#	ifdef	O_CREAT
	while ( (fd = open(file, O_CREAT|O_EXCL|O_WRONLY, FILE_MODE)) == SYSERROR )
#	else	/* O_CREAT */
	while ( (fd = creat(file, FILE_MODE)) == SYSERROR )
#	endif	/* O_CREAT */
		if ( !CheckDirs(file) )
			SysError(CouldNot, CreateStr, file);

	return fd;
}



/*
**	Create a file for appending.
*/

int
CreateA(
	char *	file
)
{
	int	fd;

	Trace((2, "CreateA(%s)", file));

#	ifdef	O_CREAT

	while ( (fd = open(file, O_CREAT|O_APPEND|O_WRONLY, FILE_MODE)) == SYSERROR )
		if ( !CheckDirs(file) )
			SysError(CouldNot, CreateStr, file);

#	else	/* O_CREAT */

	if ( (fd = open(file, O_WRONLY)) == SYSERROR )
	{
		while ( (fd = creat(file, FILE_MODE)) == SYSERROR )
			if ( !CheckDirs(file) )
				SysError(CouldNot, CreateStr, file);
	}
	else
		(void)lseek(fd, (long)0, 2);

#	if	FCNTL == 1 && O_APPEND != 0
	/*
	**	Real "append" mode.
	*/

	(void)fcntl
	(
		fd,
		F_SETFL,
		fcntl(fd, F_GETFL, 0) | O_APPEND
	);
#	endif
#	endif	/* O_CREAT */

	return fd;
}



/*
**	Create a file for writing, and ignore errors.
*/

int
CreateN(
	char *	file,
	bool	gripe
)
{
	int	fd;

	Trace((2, "CreateN(%s)", file));

#	ifdef	O_CREAT
	while ( (fd = open(file, O_CREAT|O_WRONLY, FILE_MODE)) == SYSERROR )
#	else	/* O_CREAT */
	while ( (fd = creat(file, FILE_MODE)) == SYSERROR )
#	endif	/* O_CREAT */
		if ( !CheckDirs(file) )
			if ( !gripe || !SysWarn(CouldNot, CreateStr, file) )
				break;

	return fd;
}



/*
**	Create a file for writing and reading.
*/

int
CreateR(
	char *	file
)
{
	int	fd;

	Trace((2, "CreateR(%s)", file));

#	ifdef	O_CREAT

	while ( (fd = open(file, O_CREAT|O_EXCL|O_RDWR, FILE_MODE)) == SYSERROR )
		if ( !CheckDirs(file) )
			SysError(CouldNot, CreateStr, file);

#	else	/* O_CREAT */

	while
	(
		close(creat(file)) == SYSERROR
		||
		(fd = open(file, O_RDWR)) == SYSERROR
	)
		SysError(CouldNot, CreateStr, file);

#	endif	/* O_CREAT */

	return fd;
}



/*
**	Link a file.
*/

void
Link(
	char *	name1,
	char *	name2
)
{
	Trace((2, "Link(%s, %s)", name1, name2));

	while ( link(name1, name2) == SYSERROR )
		if ( !CheckDirs(name2) )
			SysError("Can't link \"%s\" to \"%s\"", name1, name2);
}



/*
**	Move a file.
*/

void
Move(
	char *	name1,
	char *	name2
)
{
	Trace((2, "Move(%s, %s)", name1, name2));

#	if	RENAME_2 == 1

	while ( rename(name1, name2) == SYSERROR )
		if ( !CheckDirs(name2) )
			SysError(english("Can't rename \"%s\" to \"%s\""), name1, name2);

#	else	/* RENAME_2 == 1 */

	(void)unlink(name2);
	Link(name1, name2);
	(void)unlink(name1);

#	endif	/* RENAME_2 == 1 */
}



/*
**	Move a file, but don't panic on error.
*/

bool
MoveN(
	char *	name1,
	char *	name2
)
{
	Trace((2, "MoveN(%s, %s)", name1, name2));

#	if	RENAME_2 != 1
	(void)unlink(name2);
	while ( link(name1, name2) == SYSERROR )
#	else	/* RENAME_2 != 1 */
	while ( rename(name1, name2) == SYSERROR )
#	endif	/* RENAME_2 != 1 */
		if
		(
			!CheckDirs(name2)
			&&
			!SysWarn(english("Can't rename \"%s\" to \"%s\""), name1, name2)
		)
			return false;

#	if	RENAME_2 != 1
	(void)unlink(name1);
#	endif	/* RENAME_2 != 1 */

	return true;
}
