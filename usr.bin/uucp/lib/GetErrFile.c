/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Read in an error file created by 'Execute', remove it,
**	and return a pointer to a string containing error message.
**
**	RCSID $Id: GetErrFile.c,v 1.1.1.1 1992/09/28 20:08:40 trent Exp $
**
**	$Log: GetErrFile.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:40  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	EXECUTE
#define	FILE_CONTROL
#define	STDIO
#define	SYSEXITS
#define	SYS_STAT

#include	"global.h"


#define	MAX_ERR_LEN	2000

extern char *	KeepErrFile;	/* Declared in MakeErrFile() */

static char	Str0[]		= english("Exit status ");
static char	Str1[]		= english("Bad exit status ");
static char	Str2[]		= english(" signal ");
static char	Str3[]		= english(" from:-\n\"");
static char	Str4[]		= english("\nstderr << EOF\n");
static char	Str5[]		= english("EOF >> stderr");
static char	Str6[]		= english("\"");
static char	StrCore[]	= english(" (core dumped)");
static char	StrUnex[]	= english("Unexplained error ");

static char *	status_string(char *, int);

#define	STATUS_LENGTH	15



char *
GetErrFile(
	VarArgs *	va,
	int		status,
	int		fd
)
{
	register int	size;
	register char *	cp;
	register char **cpp;
	register char *	np;
	char *		errs;
	char		number[4];
	struct stat	statb;

	size = sizeof Str1 + sizeof Str2 + sizeof Str3 + sizeof Str4
		+ sizeof Str5 + sizeof number * 2
		+ sizeof StrCore + STATUS_LENGTH + 2;

	if ( fd == SYSERROR || ErrorTty((int *)0) || fstat(fd, &statb) == SYSERROR )
		statb.st_size = 0;
	else
	{
		if ( statb.st_size == 0 && KeepErrFile != NULLSTR )
			(void)unlink(KeepErrFile);
		if ( statb.st_size > MAX_ERR_LEN )
		{
			(void)lseek(fd, statb.st_size-MAX_ERR_LEN, 0);
			statb.st_size = MAX_ERR_LEN;
		}
		else
			(void)lseek(fd, (long)0, 0);
		size += statb.st_size;
	}

	if ( statb.st_size == 0 )
	{
		if ( status == 0 )
		{
			if ( fd != SYSERROR && fd != fileno(ErrorFd) )
				(void)close(fd);
			return NULLSTR;
		}

		if ( fd != SYSERROR )
		{
			if ( (cp = ARG(va, 0)) != NULLSTR )
				size += strlen(cp);
			size += sizeof StrUnex + sizeof Str2 + sizeof Str3
				+ sizeof Str6 + sizeof number + sizeof StrCore
				+ STATUS_LENGTH + 2;
		}
	}

	for ( cpp = &ARG(va, 0) ; *cpp != NULLSTR ; )
		size += strlen(*cpp++) + 1;

	errs = cp = Malloc(size);

	if ( status )
		cp = strcpyend(cp, Str1);
	else
		cp = strcpyend(cp, Str0);
	size = ExStatus & 0xff;
	(void)sprintf(number, "%d", size);
	cp = strcpyend(cp, number);
	if ( size )
		cp = status_string(cp, size);
	if ( status & 0xff )
	{
		cp = strcpyend(cp, Str2);
		(void)sprintf(number, "%d", status&0x7f);
		cp = strcpyend(cp, number);
		if ( status & 0x80 )
			cp = strcpyend(cp, StrCore);
	}

	for ( np = cp, cpp = &ARG(va, 0), size = 0 ; *cpp != NULLSTR ; size++ )
	{
		if ( size == 0 )
			np = strcpyend(np, Str3);

		np = strcpyend(np, *cpp++);

		while ( ++cp < np )
			if ( *cp < ' ' && *cp != '\n' && *cp != '\t' )
				*cp = '?';
		*np++ = ' ';
	}

	if ( size != 0 )
		cp = strcpyend(cp, Str6);

	if ( (size = statb.st_size) > 0 )
	{
		cp = strcpyend(cp, Str4);

		(void)read(fd, cp, size);

		while ( --size >= 0 )
			if ( *cp++ < ' ' && cp[-1] != '\n' && cp[-1] != '\t' )
				if ( cp[-1] == '\0' )
					cp[-1] = ' ';
				else
					cp[-1] = '?';
		
		for ( size = statb.st_size ; *--cp == ' ' && --size > 0 ; );

		if ( *cp++ != '\n' )
			*cp++ = '\n';

		(void)strcpy(cp, Str5);
	}
	else
	if ( fd != SYSERROR )
	{
		cp = strcpyend(cp, Str4);

		cp = strcpyend(cp, StrUnex);
		size = ExStatus & 0xff;
		(void)sprintf(number, "%d", size);
		cp = strcpyend(cp, number);
		if ( size )
			cp = status_string(cp, size);
		if ( status & 0xff )
		{
			cp = strcpyend(cp, Str2);
			(void)sprintf(number, "%d", status&0x7f);
			cp = strcpyend(cp, number);
			if ( status & 0x80 )
				cp = strcpyend(cp, StrCore);
		}
		cp = strcpyend(cp, Str3);
		if ( (np = ARG(va, 0)) != NULLSTR )
			cp = strcpyend(cp, np);
		cp = strcpyend(cp, Str6);
		*cp++ = '\n';

		(void)strcpy(cp, Str5);
	}

	if ( fd != SYSERROR && fd != fileno(ErrorFd) )
		(void)close(fd);

	return errs;
}



/*
**	Convert status to string.
*/

static char *
status_string(
	register char *	cp,
	int		status
)
{
	register char *	sp = NULLSTR;

	switch ( status )
	{
	case EX_USAGE:		sp = "USAGE";		break;
	case EX_DATAERR:	sp = "DATAERR";		break;
	case EX_NOINPUT:	sp = "NOINPUT";		break;
	case EX_NOUSER:		sp = "NOUSER";		break;
	case EX_NOHOST:		sp = "NOHOST";		break;
	case EX_UNAVAILABLE:	sp = "UNAVAILABLE";	break;
	case EX_SOFTWARE:	sp = "SOFTWARE";	break;
	case EX_OSERR:		sp = "OSERR";		break;
	case EX_OSFILE:		sp = "OSFILE";		break;
	case EX_CANTCREAT:	sp = "CANTCREAT";	break;
	case EX_IOERR:		sp = "IOERR";		break;
	case EX_TEMPFAIL:	sp = "TEMPFAIL";	break;
	case EX_PROTOCOL:	sp = "PROTOCOL";	break;
	case EX_NOPERM:		sp = "NOPERM";		break;
	case EX_FDERR:		sp = "FD0ERR";		break;
	case EX_FDERR+1:	sp = "FD1ERR";		break;
	case EX_FDERR+2:	sp = "FD2ERR";		break;
	}

	if ( sp != NULLSTR )
	{
		*cp++ = ' ';
		*cp++ = '[';
		cp = strcpyend(cp, sp);
		*cp++ = ']';
		*cp = '\0';
	}

	return cp;
}



/*
**	Strip debugging info. from error string.
*/

char *
StripErrString(
	char *		errs
)
{
	register char *	cp1;
	register char *	cp2;

	if ( (cp2 = strstr(errs, Str4)) == NULLSTR )
		return newstr(errs);

	do
		cp1 = cp2 + (sizeof Str4 - 1);
	while
		( (cp2 = strstr(cp1, Str4)) != NULLSTR );

	if ( (cp2 = strstr(cp1, Str5)) == NULLSTR )
		return newstr(cp1);

	return newnstr(cp1, cp2-cp1);
}



/*
**	Return status info. from error string.
*/

char *
StripStatusString(
	char *		errs
)
{
	register char *	cp1;
	register char *	cp2;

	if ( (cp2 = strstr(errs, Str3)) == NULLSTR )
		return newstr(errs);

	if ( (cp1 = strstr(errs, Str0)) != NULLSTR )
		cp1 += sizeof Str0;
	else
	if ( (cp1 = strstr(errs, Str1)) != NULLSTR )
		cp1 += sizeof Str1;
	else
		return newnstr(errs, cp2-errs);

	return newnstr(cp1, cp2-cp1);
}
