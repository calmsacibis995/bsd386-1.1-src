/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Create work file in spool area.
**
**	Saves names in list for cleanup.
**
**	Returns file descriptor for open file.
**
**	RCSID $Id: CreateWf.c,v 1.1.1.1 1992/09/28 20:08:38 trent Exp $
**
**	$Log: CreateWf.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:38  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.2  1992/08/04  13:40:09  ziegast
 * To keep from permanent loop.
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	FILE_CONTROL

#include	"global.h"

/*
**	List of open files.
*/

typedef struct wf	Wf;
struct wf
{
	Wf *	next;
	char *	path;	/* Full pathname */
	char *	file;	/* Free later */
	int	fd;	/* SYSERROR unless created */
	bool	made;	/* True if `path' created */
};
static Wf *	Head;	/* Head of linked list */

int
CreateWf(path)
	register char *	path;
{
	register Wf *	wp;
	int		fd;

	Trace((2, "CreateWf(%s)", path));

	for ( wp = Head ; wp != (Wf *)0 ; wp = wp->next )
		if ( path == wp->path )
			break;

	if ( wp == (Wf *)0 )
		Fatal("CreateWf(%s): unregistered file", path);

	if ( (fd = wp->fd) == SYSERROR )
	{
		fd = Create(path);
		wp->made = true;
	}
	else
		wp->fd = SYSERROR;

	Trace((1, "CreateWf %s => [%d]", path, fd));

	return fd;
}

/*
**	Ensure unique path by creating.
*/

char *
UniqueWf(
	char **		file,	/* Last component of pathname */
	char		type,	/* TYPE */
	char		grade,	/* Grade */
	char *		name,	/* Node name in file */
	char *		node	/* Node name for path */
)
{
	register Wf *	wp;
	int		fd;
	int		count;
	char *		path;

	Trace((1, "UniqueWf(%#lx, %c, %c, %s, %s)", (Ulong)file, type, grade, name, node));

	for ( count = 0 ;; ) 
	{
		*file = WfName(type, grade, name);
		path = WfPath(*file, type, node);

		if ( (fd = CreateN(path, false)) != SYSERROR )
			break;

		if ( errno != EEXIST || ++count > 3 )
			SysError(CouldNot, CreateStr, path);

		ErrorLog("Sequence number wrap, retrying...");
		NewCount();	/* Someone else is using this set */
	}

	wp = Head;	/* From last WfPath() */
	wp->fd = fd;
	wp->made = true;

	return path;
}

/*
**	Unlink all workfiles created.
*/

void
UnlinkAllWf()
{
	register Wf *	wp;

	Trace((1, "UnlinkAllWf()"));

	for ( wp = Head ; wp != (Wf *)0 ; wp = wp->next )
	{
		if ( wp->fd != SYSERROR )
		{
			Trace((2, "\tclose %d", wp->fd));
			(void)close(wp->fd);
			wp->fd = SYSERROR;
		}

		if ( wp->made )
		{
			Trace((2, "\tunlink %s", wp->path));
			(void)unlink(wp->path);
			wp->made = false;
		}
	}
}

/*
**	Free up allocated memory
**	(and close open files).
*/

void
WfFree()
{
	register Wf *	wp;

	Trace((1, "WfFree()"));

	while ( (wp = Head) != (Wf *)0 )
	{
		Head = wp->next;

		if ( wp->fd != SYSERROR )
			(void)close(wp->fd);
		free(wp->file);
		free(wp->path);
		free((char *)wp);
	}
}

/*
**	Workfile pathname generation.
*/

char *
WfPath(
	char *		file,	/* Last component of UUCP filename */
	char		type,	/* Used for directory name if NEW */
	char *		node	/* Node name used for sub-directory */
)
{
	register Wf *	wp;
	register char *	cp;

#	if	DEBUG
	if ( file == NULLSTR || file[0] == '\0' )
		Fatal("NULL file for WfPath()");

	if ( node == NULLSTR || node[0] == '\0' )
		Fatal("NULL node name for WfPath()");
#	endif	/* DEBUG */

	Trace((2, "WfPath(%s, %c, %s)", file, type, node));

	cp = concat(SPOOLDIR, node, Slash, file, NULLSTR);

#	if	DEBUG
	if ( Traceflag )
	for ( wp = Head ; wp != (Wf *)0 ; wp = wp->next )
		if ( strcmp(cp, wp->path) == STREQUAL )
			Fatal("WfPath(%s, %s): duplicate file", file, node);
#	endif	/* DEBUG */

	wp = Talloc(Wf);
	wp->path = cp;
	wp->file = file;
	wp->next = Head;
	wp->made = false;
	wp->fd = SYSERROR;
	Head = wp;

	Trace((3, "\tWfPath => %s", cp));
	return cp;
}
