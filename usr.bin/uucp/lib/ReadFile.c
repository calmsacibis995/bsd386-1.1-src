/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Read in a file, and pass back a pointer to data.
**
**	Remove any trailing '\n',
**	and guarantee at least 1 byte exists beyond '\0'.
**
**	Size of data is exported in "RdFileSize".
**	Modify time of data is exported in "RdFileTime".
**
**	RCSID $Id: ReadFile.c,v 1.1.1.1 1992/09/28 20:08:39 trent Exp $
**
**	$Log: ReadFile.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:39  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILE_CONTROL
#define	SYS_STAT
#define	ERRNO

#include	"global.h"


int		RdFileSize;
Time_t		RdFileTime;



char *
ReadFile(file)
	register char *	file;
{
	register int	fd;

	Trace((2, "ReadFile(%s)", file));

	RdFileSize = 0;
	errno = 0;

	if ( file == NULLSTR || (fd = open(file, O_RDONLY)) == SYSERROR )
	{
		RdFileTime = 0;
		return NULLSTR;
	}

	file = ReadFd(fd);

	(void)close(fd);

	return file;
}



/*
**	Read in a file, and pass back a pointer to data.
**
**	Remove any trailing '\n's,
**	and guarantee at least 1 byte exists beyond '\0'.
**
**	Size of data is exported in "RdFileSize".
**	Modify time of data is exported in "RdFileTime".
*/

char *
ReadFd(fd)
	register int	fd;
{
	register char *	data;
	struct stat	statb;

	Trace((2, "ReadFd(%d)", fd));

	statb.st_mtime = 0;
	RdFileSize = 0;
	errno = 0;

	if
	(
		fstat(fd, &statb) == SYSERROR
		||
		(RdFileSize = statb.st_size) == 0
	)
	{
		RdFileTime = statb.st_mtime;
		return NULLSTR;
	}

	RdFileTime = statb.st_mtime;

	data = Malloc((int)RdFileSize+2);	/* +2 so that callers may add 1 byte on end */

	if ( read(fd, data, (int)RdFileSize) != RdFileSize )
	{
		free(data);
		return NULLSTR;
	}

	if ( RdFileSize > 0 && data[RdFileSize-1] == '\n' )
		RdFileSize--;
	data[RdFileSize] = '\0';

	Trace((3, "ReadFd(%d) %d bytes", fd, RdFileSize));

	return data;
}
