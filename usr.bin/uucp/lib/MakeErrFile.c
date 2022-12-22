/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Create a file for error output (use "KeepErrFile" if set),
**	unless "stderr" is already assigned to a tty.
**
**	RCSID $Id: MakeErrFile.c,v 1.1.1.1 1992/09/28 20:08:39 trent Exp $
**
**	$Log: MakeErrFile.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:39  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	EXECUTE
#define	FILES
#define	FILE_CONTROL
#define	STDIO

#include	"global.h"

char *		KeepErrFile;



void
MakeErrFile(fdp)
	int *		fdp;
{
	register char *	errfile;

	Trace((1,
		"MakeErrFile(%#lx), KeepErrFile=%s",
		(Ulong)fdp,
		(KeepErrFile==NULLSTR)?NullStr:KeepErrFile
	));

	if ( ErrorTty(fdp) )
		return;

	if ( (errfile = KeepErrFile) == NULLSTR )
	{
		char *	cp;

		cp = WfName(DATA_TYPE, 'T', NODENAME);
		errfile = concat(TMPDIR, cp, NULLSTR);
		free(cp);
	}

	*fdp = CreateR(errfile);

	Trace((2, "MakeErrFile() => \"%s\" (%d)", errfile, *fdp));

	if ( KeepErrFile == NULLSTR )
	{
		(void)unlink(errfile);
		free(errfile);
	}
	else
		(void)chmod(errfile, (~WORK_FILE_MASK)&0777);
}
