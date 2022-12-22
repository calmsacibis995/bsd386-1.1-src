/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Command list processing.
**
**	RCSID $Id: QueueCmd.c,v 1.2 1993/02/28 15:28:47 pace Exp $
**
**	$Log: QueueCmd.c,v $
 * Revision 1.2  1993/02/28  15:28:47  pace
 * Add recent uunet changes
 *
 * Revision 1.1.1.1  1992/09/28  20:08:37  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	NO_VA_FUNC_DECLS
#define	FILE_CONTROL

#include	"global.h"

typedef struct
{
	char *	cp;
	char *	buf;
	int	len;
}
	Buf;

#define	INITIAL_BUF_SIZE	256	/* Big enough for most command files */

static char *	buffer(va_list, Buf *);

typedef struct Lst	Lst;
struct Lst
{
	Lst *	next;
	char *	name;
	Buf	buf;
};

static Lst *	Head;

/*
**	Queue a command string for a command file.
*/

void
QueueCmd(va_alist)
	va_dcl		/* file, fmt, arg, ... */
{
	register va_list vp;
	register Lst *	lp;
	char *		file;
	char *		fmt;

	va_start(vp);
	file = va_arg(vp, char *);

#	if	DEBUG
	if ( file == NULLSTR || file[0] == '\0' )
		Fatal("No filename for QueueCmd()");
	if ( strncmp(file, SPOOLDIR, SpoolDirLen) != STREQUAL )
		Fatal("QueueCmd(%s) - name must start with %s", file, SPOOLDIR);
#	endif	/* DEBUG */

	Trace((2, "QueueCmd(%s)", file));

	for ( lp = Head ; lp != (Lst *)0 ; lp = lp->next )
	{
		if ( lp->name == file )
		{
			(void)buffer(vp, &lp->buf);
			va_end(vp);
			return;
		}
	}

	Trace((1, "QueueCmd(%s) create list element", file));

	lp = Talloc0(Lst);
	lp->next = Head;
	Head = lp;
	lp->name = file;

	(void)buffer(vp, &lp->buf);
	va_end(vp);
}

/*
**	Write out accumulated commands to respective files.
*/

int
WriteCmds()
{
	register Lst *	lp;
	register Lst *	np;
	register int	len;
	register int	fd;
	int		count;

	Trace((1, "WriteCmds()"));

	for ( count = 0, lp = Head ; lp != (Lst *)0 ; )
	{
		if ( (len = lp->buf.cp - lp->buf.buf) == 0 )
			continue;

		fd = CreateWf(lp->name);

		while ( write(fd, lp->buf.buf, len) != len )
		{
			SysError(lp->name);
			(void)lseek(fd, (long)0, 0);
		}

		while ( close(fd) == SYSERROR )
			SysError(lp->name);

		Trace((2, "WriteCmds %d bytes => %s", len, lp->name));

		np = lp->next;
		free(lp->buf.buf);
		free((char *)lp);
		lp = np;

		count++;
	}

	Head = (Lst *)0;

	Trace((1, "WriteCmds %d files", count));

	return count;
}

/*
**	Store string in expandable memory buffer.
*/

static char *
buffer(vp, bufp)
	register va_list vp;
	register Buf *	bufp;
{
	register char *	cp;
	register int	len;
	register int	slen;
	register int	newlen;
#	if	NO_VFPRINTF
	char *		a[16];
#	endif	/* NO_VFPRINTF */
	char		buf[10240];

	if ( (cp = va_arg(vp, char *)) == NULLSTR )
		Fatal("No string for buffer()");

#	if	NO_VFPRINTF

	for ( len = 0 ; len < 16 ; len++ )
		a[len] = va_arg(vp, char *);

	(void)sprintf(buf, cp, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
			a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15]);

#	else	/* NO_VFPRINTF */

	(void)vsprintf(buf, cp, vp);

#	endif	/* NO_VFPRINTF */

	slen = strlen(buf);

	Debug((4, "cmd: %.*s", slen-1, buf));	/* Avoid trailing '\n' */

	if ( (cp = bufp->buf) == NULLSTR )
	{
		bufp->len = INITIAL_BUF_SIZE;
		bufp->buf = bufp->cp = cp = Malloc(bufp->len);
	}

	len = bufp->cp - cp;

	if ( (slen += len) > (newlen = bufp->len) )
	{
		if ( newlen > 8192 )
			newlen += 8192;
		else
			newlen *= 2;
		if ( newlen < slen )
			newlen = (slen|8191)+1;

		bufp->buf = Malloc(newlen);
		bufp->cp = strncpyend(bufp->buf, cp, len);
		bufp->len = newlen;

		free(cp);
		cp = bufp->buf;
	}

	bufp->cp = strcpyend(bufp->cp, buf);

	return cp;
}
