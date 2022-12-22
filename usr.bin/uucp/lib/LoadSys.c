/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Routines to access SYSFILE/ALIASFILE.
**
**	RCSID $Id: LoadSys.c,v 1.1.1.1 1992/09/28 20:08:44 trent Exp $
**
**	$Log: LoadSys.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:44  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.3  1992/08/04  13:38:05  ziegast
 * Got rid of "register" to make unprotoizing work.  (debugging)
 *
 * Revision 1.2  1992/04/17  21:43:47  piers
 * duplicate alias warning more explicit
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILES

#include "global.h"


/*
**	Variables.
*/

Sysl		SysHead;	/* Head of skip-list */
static int	Level;		/* During skip-list insertion */

extern char	SS_Space[];
#ifdef	DEBUG
extern char	SS_GotStr[];
extern char	SS_Need[];
extern char	SS_No[];

static void	TraceSys(char *, char *, Syst);
#endif	/* DEBUG */


/*
**	Add new element to skip-list.
*/

static Sysl *
insert(
	char *		name
)
{
	register int	i;
	register int	val;
	register Sysl *	sp1;
	register Sysl *	sp2;
	Sysl *		next[MAXLEVEL];

	/** Search **/

	for ( sp1 = &SysHead, i = Level ; i >= 0 ; i-- )
	{
		while ( (sp2 = sp1->next[i]) != (Sysl *)0 )
		{
			if ( (val = strcmp(name, sp2->name)) == STREQUAL )
				return sp2;		/* match */
			if ( val < 0 )
				break;
			sp1 = sp2;
		}

		next[i] = sp1;
	}

	/** Choose new level **/

	for ( i = 0 ; (random() & 0x6000) == 0 ; )	/* `p' = 1/4 */
		if ( ++i > Level )
		{
			Trace((1, "New level %d", i));

			if ( i < MAXLEVEL )
			{
				Level = i;
				next[i] = &SysHead;
				if ( i >= MINLEVEL )
					break;		/* fix dice */
				continue;
			}

			i = Level;
			break;
		}

	/** Insert **/

	TraceT(8, (8, "\tinsert level %d \"%s\" after \"%s\"", i, name, (sp1==&SysHead)?"HEAD":sp1->name));

	/*
	**	Don't allocate unused pointers
	**	O{n} => O{log4(n)} memory use.
	*/

	sp2 = (Sysl *)Malloc(sizeof(Sysl)-(MAXLEVEL-1-i)*sizeof(Sysl *));

	do
	{
		sp2->next[i] = next[i]->next[i];
		next[i]->next[i] = sp2;
	}
	while
		( --i >= 0 );

	sp2->name = name;
	sp2->type = unk_t;
	sp2->val = NULLSTR;
	sp2->free = false;
	return sp2;
}

/*
**	Build skip-list of nodes and aliases.
*/

void
LoadSys(
	Nst		needsys
)
{
	register Sysl *	sp;
	register char *	cp;
	register char *	p;
	register char *	q;
	char *		data;
	char *		val;
	char *		bufp;

	if ( SysHead.next[0] != (Sysl *)0 )
		return;	/* Already loaded */

	Trace((1, "LoadSys(%ssys)", needsys==NEEDSYS?SS_Need:SS_No));

	srandom((int)Time);

	/*
	**	Incorporate SYSFILE.
	**
	**	Format is: <node>[<space><param>]+
	*/

	while ( (cp = ReadFile(SYSFILE)) == NULLSTR )
		SysError(CouldNot, ReadStr, SYSFILE);

	bufp = data = cp;

	while ( (cp = GetSysNode(&val, &bufp)) != NULLSTR )
	{
		Trace((8, SS_GotStr, cp, (val==NULLSTR)?EmptyStr:val));

		sp = insert(cp);

		if ( sp->type != unk_t )
		{
			/** Nodes can have multiple entries **/

			Trace((5, "%s: multiple entry", cp));

			if ( needsys == NEEDSYS && val != NULLSTR )
				if ( (cp = sp->val) != NULLSTR )
				{
					sp->val = concat(cp, "\n", val, NULLSTR);
					if ( sp->free )
						free(cp);
					else
						sp->free = true;
				}
				else
					sp->val = val;
		}
		else
		{
			sp->val = val;
			sp->type = sys_t;
		}
	}

	/*
	**	Incorporate ALIASFILE.
	**
	**	Format is: <node>[<space><alias>]+
	*/

	if ( (cp = ReadFile(ALIASFILE)) != NULLSTR )
	{
		bufp = data = cp;

		while ( (cp = GetSysNode(&val, &bufp)) != NULLSTR )
		{
			TraceT(8, (8, SS_GotStr, cp, (val==NULLSTR)?EmptyStr:val));

			/*
			**	Could lookup node here, but less CPU if
			**	we do it in verify.
			*/

			if ( (p = val) != NULLSTR )
			for ( ;; )
			{
				if ( (q = strpbrk(p, SS_Space)) != NULLSTR )
					*q++ = '\0';

				sp = insert(p);

				if ( sp->type != unk_t )
					Warn
					(
		"Alias \"%s\" for node  \"%s\" in \"%s\" duplicates node in \"%s\"",
						p,
						cp,
						ALIASFILE,
						SYSFILE
					);
				else
				{
					sp->val = cp;
					sp->type = alias_t;
				}

				if ( (p = q) == NULLSTR )
					break;

				p += strspn(p, SS_Space);
			}
		}
	}

#	ifdef	DEBUG
	if ( Traceflag >= 9 )
		WalkSys(TraceSys);
#	endif	/* DEBUG */
}

/*
**	Search for `name' in skip-list.
*/

Sysl *
LookupSys(
	char *		name
)
{
	register int	i;
	register int	val;
	register Sysl *	sp1;
	register Sysl *	sp2;

	Trace((2, "LookupSys(%s)", name));

	/** Search **/

	for ( sp1 = &SysHead, i = Level ; i >= 0 ; i-- )
		while ( (sp2 = sp1->next[i]) != (Sysl *)0 )
		{
			TraceT(7, (7, "cmp %s <> %s", name, sp2->name));
			if ( (val = strcmp(name, sp2->name)) == STREQUAL )
				return sp2;		/* match */
			if ( val < 0 )
				break;
			sp1 = sp2;
		}

	return (Sysl *)0;
}

#ifdef	DEBUG
/*
**	Trace table build.
**
**	Called from WalkSys() once for each node.
*/

static void
TraceSys(
	char *		name,
	char *		val,
	Syst		type
)
{
	static int	count;

	Trace((9, "\t%2d: %s", count++, name));
}
#endif	/* DEBUG */

/*
**	Walk through cached entries.
*/

void
WalkSys(
	void	(*funcp)(char *, char *, Syst)
)
{
	register Sysl *	sp;

	for ( sp = SysHead.next[0] ; sp != (Sysl *)0 ; sp = sp->next[0] )
		(*funcp)(sp->name, sp->val, sp->type);
}
