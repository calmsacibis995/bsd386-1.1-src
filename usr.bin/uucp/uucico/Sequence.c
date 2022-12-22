/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Routines to manipulate call sequence number.
**
**	NB: this should be redone to use a per-node sequence file. XXX
**
**	RCSID $Id: Sequence.c,v 1.1.1.1 1992/09/28 20:08:54 trent Exp $
**
**	$Log: Sequence.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:54  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILES
#define	TIME
#define	SETJMP
#define	STDIO
#define	SYS_TIME

#include	"global.h"
#include	"cico.h"


#define	MAX_NODESEQ	9999

static bool	Commit;

static void	putcount(FILE *, char *, int);


/*
**	Get next conversation sequence number.
*/

int
GetNxtSeq(
	char *		node
)
{
	register FILE *	fp0;
	register FILE *	fp1;
	int		count;
	int		ct;
	bool		found;
	char		name[NODENAMEMAXSIZE+1];
	char		buf[BUFSIZ];

	Commit = false;

	if ( NODESEQFILE == NULLSTR || access(NODESEQFILE, 0) == SYSERROR )
		return 0;	/* Sequence == 0 if no checks */

	if ( !mklock(NODESEQLOCK) )
	{
		static char	cl[] = "CAN'T LOCK %s";

		Log(cl, NODESEQLOCK);
		Debug((4, cl, NODESEQLOCK));
		return MAX_NODESEQ+1;
	}

	if ( (fp0 = fopen(NODESEQFILE, "r")) == NULL )
		return 0;

	if ( (fp1 = fopen(NODESEQTEMP, "w")) == NULL )
	{
		fclose(fp0);
		return MAX_NODESEQ+1;
	}

	(void)chmod(NODESEQTEMP, 0400);

	found = false;
	count = 0;

	while ( fgets(buf, BUFSIZ, fp0) != NULL )
	{
		if ( found )
		{
			fputs(buf, fp1);
			continue;
		}

		name[0] = '\0';
		if ( sscanf(buf, "%s %d", name, &ct) < 2 )
			ct = 0;
		name[NODENAMEMAXSIZE] = '\0';

		if ( strncmp(node, name, NODENAMEMAXSIZE) != STREQUAL )
		{
			fputs(buf, fp1);
			continue;
		}

		/** Found node **/

		found = true;

		if ( ct >= MAX_NODESEQ )
			ct = 0;
		count = ++ct;

		putcount(fp1, node, count);
	}

	if ( found )
		Commit = true;

	(void)fclose(fp0);

	if ( fclose(fp1) == EOF )
	{
		SysWarn(CouldNot, WriteStr, NODESEQTEMP);
		return MAX_NODESEQ+1;
	}

	return count;
}

/*
**	Commit sequence update.
*/

void
CommitSeq()
{
	if ( NODESEQFILE == NULLSTR )
		return;

	if ( Commit )
		Move(NODESEQTEMP, NODESEQFILE);
	else
		(void)unlink(NODESEQTEMP);

	rmlock(NODESEQLOCK);

	Commit = false;
}

/*
**	Write out a node+seq line.
*/

static void
putcount(
	FILE *			fd,
	char *			node,
	int			count
)
{
	register struct tm *	tp;

	(void)SetTimes();
	tp = localtime(&Time);

	(void)fprintf
	(
		fd,
		"%s %d %d/%d-%02d:%02d\n",
		node,
		count,
		tp->tm_mon + 1,
		tp->tm_mday,
		tp->tm_hour,
		tp->tm_min
	);
}

/*
**	Unlock sequence file.
*/

void
UnlockSeq()
{
	Commit = false;
	CommitSeq();
}
