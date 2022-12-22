/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Return next file from directory queue.
**
**	RCSID $Id: FindFile.c,v 1.1.1.1 1992/09/28 20:08:42 trent Exp $
**
**	$Log: FindFile.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:42  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILES
#define	NDIR
#define	SYS_STAT

#include	"global.h"

/*
**	File list built from queue directory search.
*/

#define	NAMESLOP	5	/* Some sites send long names */

typedef struct Flist
{
	struct Flist *	next;	/* Must be first (for listsort(3)) */
	char *		ncmp;	/* Start of sort string in `name' */
	char		name[WORKNAMESIZE+1+NAMESLOP];
}
	Flist;

static Flist *	FfreeHead;	/* Head of free list */
static Flist *	FlistHead;	/* Head of current list */
static int	FlistCount;	/* Items in queue */

static Time_t	CheckTime;	/* Last time check */
static DIR *	Dirp;		/* Current directory */

int		FQmax;		/* Maximum size of file queue */


static void	listbuild(DIR *, char *, char *, Find_t);
static int	listcmp(char *, char *);
static bool	newfiles(int);

extern char	MaxGrade;	/* Reject grades > this */



char *
FindFile(
	char *		dir,	/* Directory to be read */
	char *		prefix,	/* Every file selected must start with this */
	Find_t		action	/* Return filename, or check */
)
{
	register Flist *fp;

	static char *	lastdir;

	if ( dir == NULLSTR )
	{
		Trace((2, "FindFile(%s)", NullStr));
		FindFlush();
		return NULLSTR;
	}

	Trace((2, "FindFile(%s, %s, %s)", dir, prefix, (action==CHECKFILE)?"CHECK":"FIND"));

	if ( lastdir == NULLSTR || strcmp(dir, lastdir) != STREQUAL )
	{
		FindFlush();
		lastdir = newstr(dir);
	}

	if ( (action != ONEPASS && newfiles(DIRCHECKTIME)) || (fp = FlistHead) == (Flist *)0 )
	{
		if ( Dirp == (DIR *)0 )
		{
			while ( (Dirp = opendir(dir)) == NULL )
				if ( !SysWarn(CouldNot, ReadStr, dir) )
					return NULLSTR;
		}
		else
		if ( action != ONEPASS )
			rewinddir(Dirp);
		else
			goto nomore;

		listbuild(Dirp, prefix, dir, action);

		listsort((char **)&FlistHead, listcmp);

		if ( (fp = FlistHead) == (Flist *)0 )
		{
nomore:			Debug((2, "FindFile(%s) => NONE", prefix));
			closedir(Dirp);
			Dirp = (DIR *)0;
			return NULLSTR;
		}
	}

	/*
	**	If CHECK, filename will be discarded, and sorted in next call.
	**
	**	NB: this means that CHECK followed by ONEPASS won't work!
	*/

	FlistHead = fp->next;
	fp->next = FfreeHead;
	FfreeHead = fp;

	Debug((2, "FindFile(%s) => %s", prefix, fp->name));

	return fp->name;
}


/*
**	Close dir, flush files.
*/

void
FindFlush()
{
	register Flist *fp;

	Trace((1, "FindFlush() count=%d, MAX=%d", FlistCount, FQmax));

	if ( Dirp != (DIR *)0 )
	{
		closedir(Dirp);
		Dirp = (DIR *)0;
	}

	while ( (fp = FlistHead) != (Flist *)0 )
	{
		FlistHead = fp->next;
		fp->next = FfreeHead;
		FfreeHead = fp;
	}

	CheckTime = Time + DIRCHECKTIME;
}


/*
**	Build a list of selected file names from a queue directory.
*/

static void
listbuild(
	register DIR *		Dirp,
	char *			prefix,
	char *			dir,
	Find_t			action
)
{
	register Flist *	fp;
	register Flist **	nfp;
	register struct direct *dp;
	register int		plen = strlen(prefix);
	register int		qlen = SEQLEN+1;
#	ifdef	DEBUG
	static char		tmsg[] = english("Trace level %d");
#	endif	/* DEBUG */

	Trace((2, "listbuild(%s)", prefix));

	while ( (fp = FlistHead) != (Flist *)0 )
	{
		FlistHead = fp->next;
		fp->next = FfreeHead;
		FfreeHead = fp;
	}

	nfp = &FlistHead;
	FlistCount = 0;

	while ( (dp = readdir(Dirp)) != (struct direct *)0 )
	{
		static char	rjctn[]	= "reject name %s%s";

		if
		(
			dp->d_namlen > (WORKNAMESIZE+NAMESLOP)
			||
			(dp->d_namlen > WORKNAMESIZE && WARN_NAME_TOO_LONG)
		)
		{
			Warn("%s%s name too long", dir, dp->d_name);

			if ( dp->d_namlen > (WORKNAMESIZE+NAMESLOP) )
			{
				Debug((1, rjctn, dir, dp->d_name));
				continue;
			}
		}

		if ( plen == 0 )
			goto ok;

		if
		(
			dp->d_namlen < (plen+qlen)
			||
			strncmp(prefix, dp->d_name, plen) != STREQUAL
		)
		{
			Debug((9, rjctn, dir, dp->d_name));
			continue;
		}

		if ( qlen <= dp->d_namlen && dp->d_name[dp->d_namlen-qlen] > MaxGrade )
		{
			Debug((5, "reject %s%s, grade too low", dir, dp->d_name));
			continue;
		}
ok:
		FlistCount++;

		if ( (fp = FfreeHead) != (Flist *)0 )
			FfreeHead = fp->next;
		else
			fp = Talloc(Flist);

		(void)strcpy(fp->name, dp->d_name);

		if ( plen == 0 )
			fp->ncmp = fp->name;
		else
			fp->ncmp = &fp->name[dp->d_namlen-qlen];

		*nfp = fp;
		nfp = &fp->next;

		if ( action == CHECKFILE )
			break;
	}

	*nfp = (Flist *)0;

	if ( FlistCount > FQmax )
		FQmax = FlistCount;

	Debug((1, "listbuild found %d %s files", FlistCount, prefix));
}


/*
**	Compare two file names for listsort(3).
*/

static int
listcmp(
	char *		fp1,
	char *		fp2
)
{
	register char *	cp1 = ((Flist *)fp1)->ncmp;
	register char *	cp2 = ((Flist *)fp2)->ncmp;
	register int	c;

	Trace((9, "listcmp %s <> %s", cp1, cp2));

	/** Check grade **/

	if ( c = *cp1++ - *cp2++ )
		return c;

	/** Check for sequence wrap **/

	if ( c = *cp1++ - *cp2++ )
	{
		if ( c < -10 || c > 10 )	/* MAGIC ? */
			return -c;		/* Wrapped */

		return c;
	}

	/** Compare rest of sequence **/

	return strcmp(cp1, cp2);
}


/*
**	If SEQFILE changed, assume new work.
**
**	(This is far too frequent!)
*/

static bool
newfiles(
	int		incr
)
{
	bool		val;
	struct stat	statb;
	static Time_t	mod_time;	/* last mod time for time check file */

	Trace((3, "newfiles(%d)", incr));

	if ( !SetTimes() )
		return false;

	if ( Time < CheckTime )
		return false;

	Trace((2, "newfiles() time=%lu < check=%lu, stat %s", Time, CheckTime, SEQFILE));

	CheckTime = Time + incr;

	if ( stat(SEQFILE, &statb) == SYSERROR )
		return true;

	if ( statb.st_mtime > mod_time )
		val = true;
	else
		val = false;

	mod_time = statb.st_mtime;

	return val;
}
