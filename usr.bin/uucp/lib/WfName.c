/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Make work file name.
**
**	Returns malloc'd string.
**
**	RCSID $Id: WfName.c,v 1.1.1.1 1992/09/28 20:08:42 trent Exp $
**
**	$Log: WfName.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:42  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILE_CONTROL

#include	"global.h"

#if	DEBUG
int		WorkFiles;
#endif	/* DEBUG */
int		MaxWorkFiles = MAXWORKFILES;

static Ulong	Count;
static Ulong	NextCount	= 1;



char *
WfName(
	char		type,
	char		grade,
	char *		node
)
{
	register int	i;
	register char *	name;

	name = Malloc(WORKNAMESIZE+1);

	/*
	**	First two chars are `type'<.>.
	*/

	name[0] = type;
	name[1] = '.';

	/*
	**	Next chars represent nodename.
	*/

	i = WORKNAMESIZE-(3+SEQLEN);

	(void)sprintf(&name[2], "%.*s", i, node);
	name[2+i] = '\0';
	i = strlen(name);

	/*
	**	Next char is `grade'.
	*/

	name[i++] = grade;

	/*
	**	Last SEQLEN chars increment every call.
	*/

	if ( ++Count == MaxSeq )
		Count = 0;
	if ( Count == NextCount )
		NewCount();

	(void)EncodeNum(&name[i], Count, SEQLEN);

	i += SEQLEN;
	name[i] = '\0';

	Trace((2, "WfName(%c, %c, %s) => %s [%#lx]", type, grade, node, name, (Ulong)name));

#	if	DEBUG
	WorkFiles++;
#	endif	/* DEBUG */
	return name;
}

/*
**	Obtain new sequence number from locked file.
*/

void
NewCount()
{
	int	fd;
	char	num[ULONG_SIZE];

	Trace((2, "NewCount read \"%s\"", SEQFILE));

	while ( (fd = open(SEQFILE, O_RDWR)) == SYSERROR )
		if ( !SysWarn(CouldNot, OpenStr, SEQFILE) )
		{
			fd = Create(SEQFILE);
			break;
		}

	while ( flock(fd, LOCK_EX) == SYSERROR )
		SysError(CouldNot, LockStr, SEQFILE);

	if ( read(fd, num, SEQLEN) != SEQLEN )
	{
		srandom((int)Time);
		Count = (Ulong)random() % MaxSeq;
		Count -= Count % VC_size;
	}
	else
		Count = DecodeNum(num, SEQLEN);

	if ( ORIG_UUCP )
	{
		/*
		**	Value in SEQF is start of *last* used!!
		**	And we use VC_size [62] at a time.
		*/

		if ( (Count += VC_size) >= MaxSeq )
			Count -= MaxSeq;

		if ( (NextCount = Count + VC_size) >= MaxSeq )
			NextCount -= MaxSeq;

		(void)EncodeNum(num, Count, SEQLEN);
	}
	else
	{
		/*
		**	Value in SEQF is start of what we use.
		**	And we use MaxWorkFiles at a time.
		*/

		if ( (NextCount = Count + MaxWorkFiles) >= MaxSeq )
			NextCount -= MaxSeq;

		(void)EncodeNum(num, NextCount, SEQLEN);
	}

	for ( ;; )
	{
		(void)lseek(fd, (long)0, 0);

		if ( write(fd, num, SEQLEN) == SEQLEN )
			break;

		SysError(CouldNot, WriteStr, SEQFILE);
	}

	(void)close(fd);

	Trace((1, "NewCount => %lu[%.*s]", Count, SEQLEN, num));
}

#if	0
/*
**	Make unique name without using locked sequence number.
*/

char *
WfName(
	int		type,
	int		grade,
	char *		node	/* Ignored */
)
{
	register int	i;
	register char *	name;

	if ( type == CMD_TYPE && FnVers == NEW_VERS )
		return newstr("commands");

	name = Malloc(WORKNAMESIZE+1);

	name[0] = grade;

	/*
	**	Count wraps every VC_size, so update time.
	*/

	if ( Count >= VC_size )
	{
		Timeusec++;
		Count = 0;
	}

	/*
	**	Next 5 chars reflect seconds.
	*/

	(void)EncodeNum(name+1, (Ulong)Time, 5);

	/*
	**	Next 4 chars reflect micro seconds.
	*/

	(void)EncodeNum(name+1+5, (Ulong)Timeusec, 4);

	/*
	**	Next 3 chars reflect process ID.
	*/

	(void)EncodeNum(name+1+5+4, (Ulong)Pid, 3);

	/*
	**	Last 1 chars increment every call.
	*/

	(void)EncodeNum(name+1+5+4+3, (Ulong)Count, 1);

	name[1+5+4+3+1] = '\0';
	Count++;

	return name;
}
#endif	/* 0 */
