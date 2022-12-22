/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Copy data between files.
**
**	RCSID $Id: CopyFdToFd.c,v 1.1.1.1 1992/09/28 20:08:40 trent Exp $
**
**	$Log: CopyFdToFd.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:40  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	FILE_CONTROL

#include	"global.h"



/*
**	Copy ``length'' bytes from ``ifd'' to ``ofd''.
*/

bool
CopyFdToFd(ifd, ifile, ofd, ofile, sizep)
	register int	ifd;
	char *		ifile;
	register int	ofd;
	char *		ofile;
	Ulong *		sizep;
{
	register int	r;
	register int	w;
	register char *	cp;
	long		posn;
	Ulong		length;
	bool		pipe = false;
	char		buf[FILEBUFFERSIZE];

	Trace((1, "CopyFdToFd(%d, %d, %lu)", ifd, ofd, *sizep));

	while ( (posn = lseek(ofd, (long)0, 1)) == SYSERROR )
	{
		if ( errno == ESPIPE )
		{
			pipe = true;
			posn = 0;
			break;
		}

		if ( !SysWarn(CouldNot, SeekStr, ofile) )
			return false;
	}

	for ( length = *sizep ; length != 0 ; )
	{
		if ( (r = FILEBUFFERSIZE) > length )
			r = length;

		while ( (r = read(ifd, buf, r)) == SYSERROR )
			if ( !SysWarn(CouldNot, ReadStr, ifile) )
				return false;

		if ( r == 0 )
			break;

		length -= r;

		cp = buf;

		while ( (w = write(ofd, cp, r)) != r )
		{
			if ( w == SYSERROR )
			{
				if ( !SysWarn(CouldNot, WriteStr, ofile) )
					return false;
				if ( !pipe )
					(void)lseek(ofd, posn, 0);
			}
			else
			{
				cp += w;
				r -= w;
				posn += w;
			}
		}

		posn += r;
	}

	*sizep = (Ulong)posn;
	return true;
}


/*
**	Copy contents from passed file descriptor to file.
*/

bool
CopyFdToFile(ifd, ifile, ofile, sizep)
	int	ifd;
	char *	ifile;
	char *	ofile;
	Ulong *	sizep;
{
	int	ofd;
	Ulong	val;

	Trace((1, "CopyFdToFile(%d, %s, %s)", ifd, ifile, ofile));

	if ( (ofd = CreateN(ofile, true)) == SYSERROR )
		return false;

	val = CopyFdToFd(ifd, ifile, ofd, ofile, sizep);

	(void)close(ofd);

	return val;
}


/*
**	Copy contents of file onto passed file descriptor.
*/

bool
CopyFileToFd(ifile, ofd, ofile, sizep)
	char *	ifile;
	int	ofd;
	char *	ofile;
	Ulong *	sizep;
{
	int	ifd;
	bool	val;

	Trace((1, "CopyFileToFd(%s, %d, %s)", ifile, ofd, ofile));

	while ( (ifd = open(ifile, O_RDONLY)) == SYSERROR )
		if ( !SysWarn(CouldNot, OpenStr, ifile) )
			return false;

	val = CopyFdToFd(ifd, ifile, ofd, ofile, sizep);

	(void)close(ifd);

	return val;
}


/*
**	Copy contents of file onto new file.
*/

bool
CopyFileToFile(ifile, ofile)
	char *	ifile;
	char *	ofile;
{
	int	ofd;
	bool	val;
	Ulong	size = MAX_ULONG;

	Trace((1, "CopyFileToFile(%s, %s)", ifile, ofile));

	if ( (ofd = CreateN(ofile, true)) == SYSERROR )
		return false;

	val = CopyFileToFd(ifile, ofd, ofile, &size);

	(void)close(ofd);

	return val;
}
