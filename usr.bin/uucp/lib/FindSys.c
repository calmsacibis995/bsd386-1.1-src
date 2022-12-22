/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Return next node with messages to be sent.
**
**	RCSID $Id: FindSys.c,v 1.3 1994/01/31 01:25:53 donn Exp $
**
**	$Log: FindSys.c,v $
 * Revision 1.3  1994/01/31  01:25:53  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:57:32  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:41:56  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:28:42  pace
 * Add recent uunet changes
 *
 * Revision 1.1.1.1  1992/09/28  20:08:44  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILES
#define	NDIR

#include	"global.h"

/*
**	Lookup table for node names.
*/

#define	HASH_SIZE	67

typedef struct Entry	Entry;
struct Entry
{
	Entry *	great;
	Entry *	less;
	char	name[NODENAMEMAXSIZE+1];
	char	used;
};

static Entry *	NodeTable[HASH_SIZE];

static void	build_table(void);
static bool	enter(char *, bool);
static void	free_tree(Entry **);
static char *	walk(Entry **);


/*
**	Build hash table of available nodes with messages to send.
*/

static void
build_table()
{
	char *			dir;

	static char		scfmt[]	= "scanning for work: %s%s";

	Trace((2, "build_table()"));

	/** Free old table **/
	{
		register Entry **	p;

		for ( p = NodeTable ; p < &NodeTable[HASH_SIZE] ; p++ )
			free_tree(p);
	}

	/** Load SYSFILE **/

	LoadSys(NEEDSYS);		/* And accumulate system fields */

	/** Scan SPOOLALTDIRS for nodes **/

	for ( dir = NULLSTR ; (dir = ChNodeDir(NULLSTR, dir)) != NULLSTR ; )
	{
		register struct direct *dp2;
		register DIR *		dirp2;
		register struct direct *dp;
		register DIR *		dirp;

		if ( (dirp = opendir(".")) == (DIR *)0 )
		{
			(void)SysWarn(CouldNot, ReadStr, dir);
			continue;
		}

		setproctitle(scfmt, dir, EmptyStr);

		while ( (dp = readdir(dirp)) != (struct direct *)0 )
		{
			if ( dp->d_name[0] == '.' )
				continue;

			if ( strncmp(dp->d_name, LOCKPRE, strlen(LOCKPRE))
			     == STREQUAL
			    )
				continue;

			if ( !enter(dp->d_name, false) )
				continue;

			if ( (dirp2 = opendir(dp->d_name)) == NULL )
			{
				(void)SysWarn(CouldNot, ReadStr, dp->d_name);
				continue;
			}

			Debug((2, scfmt, dir, dp->d_name));

			while ( (dp2 = readdir(dirp2)) != (struct direct *)0 )
			{
				if ( dp2->d_name[0] != CMD_TYPE )
					continue;

				(void)enter(dp->d_name, true);

				break;
			}

			closedir(dirp2);
		}

		closedir(dirp);
	}
}

/*
**	Lookup name in table, enter if `add', return true if new.
*/

static bool
enter(
	char *			name,
	bool			add
)
{
	register char		c;
	register char *		cp1;
	register char *		cp2;
	register Entry **	epp;
	register Entry *	ep;
	int			l;

	if ( (l = strlen(name)) > NODENAMEMAXSIZE )
	{
		Debug((1, "name too long: %s", ExpandString(name, l)));
		return false;
	}

	for
	(
		epp = &NodeTable[HashName(name, HASH_SIZE)] ;
		(ep = *epp) != (Entry *)0 ;
	)
	{
		for ( cp1 = name, cp2 = ep->name ; c = *cp1++ ; )
			if ( c -= *cp2++ )
				break;

		if ( c == 0 && (c = *cp2) == 0 )
		{
			Trace((4, "enter(%s) exists", name));
			return false;
		}

		if ( c > 0 )
			epp = &ep->great;
		else
			epp = &ep->less;
	}

	if ( add )
	{
		*epp = ep = Talloc0(Entry);

		(void)strcpy(ep->name, name);

		Trace((3, "enter(%s)", name));
	}

	return true;
}

/*
**	Build table of nodes with messages to be sent, and return one by one.
*/

char *
FindSys()
{
	static bool		table_built;
	static Entry **		pointer;

	Trace((1, "FindSys()"));

	if ( !table_built )
	{
		build_table();
		pointer = NodeTable;
		table_built = true;
	}

	for ( ; pointer < &NodeTable[HASH_SIZE] ; pointer++ )
	{
		char *	node;

		while ( (node = walk(pointer)) != NULLSTR )
		{
			if ( !FindSysEntry(&node, NEEDSYS, NOALIAS) )
				continue;	/* Unknown system! */

			if ( !IgnoreTimeToCall && CheckSysTime(node, Master) == CF_TIME )
				continue;	/* Wrong time to call */

			if ( CallOK(node, SS_OK) != SS_OK )
				continue;

			setproctitle("%s: startup", node);

			return node;
		}
	}

	table_built = false;
	return NULLSTR;
}

/*
**	Clean tree walk.
*/

static void
free_tree(
	Entry **	epp
)
{
	register Entry *ep;

	if ( (ep = *epp) == (Entry *)0 )
		return;

	free_tree(&ep->great);
	free_tree(&ep->less);

	*epp = (Entry *)0;
	free((char *)ep);
}

/*
**	Destructive tree walk, returns next unused node.
*/

static char *
walk(
	Entry **	epp
)
{
	register Entry *ep;
	register char *	cp;

	if ( (ep = *epp) == (Entry *)0 )
		return NULLSTR;

	if ( ep->used != '\0' )
		return NULLSTR;

	if ( (cp = walk(&ep->great)) != NULLSTR )
		return cp;

	if ( (cp = walk(&ep->less)) != NULLSTR )
		return cp;

	ep->used = 'X';

	return ep->name;
}
