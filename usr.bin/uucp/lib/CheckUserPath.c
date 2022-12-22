/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	USERFILE processing routines.
**
**	RCSID $Id: CheckUserPath.c,v 1.2 1994/01/31 01:25:51 donn Exp $
**
**	$Log: CheckUserPath.c,v $
 * Revision 1.2  1994/01/31  01:25:51  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:57:32  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:41:56  vixie
 * Initial revision
 *
 * Revision 1.1.1.1  1992/09/28  20:08:42  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILES
#define	SYSEXITS

#include	"global.h"

/*
**	User structure.
*/

typedef struct User	User;

struct User
{
	User *	next;
	char *	user;	/* Login name */
	char *	node;	/* Node name */
	char **	paths;	/* NULLSTR terminated list */
	int *	lens;	/* Lengths of respective paths */
	bool	callb;	/* Callback flag */
};

static User *	Head;
static User *	UserDefault;
static User *	NodeDefault;

static bool	SetupUsers(void);


/*
**	Check if node has `callb' flag set.
*/

bool
CallBack(
	char *		user	/* XXX Why not `node'?? */
)
{
	register User *	up;

	while ( (up = Head) == (User *)0 )
		if ( !SetupUsers() )
			return false;

	for ( ; up != (User *)0 ; up = up->next )
		if ( strcmp(user, up->user) == STREQUAL )
			return up->callb;

	return false;
}

/*
**	Check User list to validate given pathname.
*/

bool
CheckUserPath(
	register char *	user,	/* Match username */
	register char *	node,	/* Match nodename */
	char *		path	/* To be validated */
)
{
	register char **cpp;
	register char *	cp;
	register User *	up;
	register int *	ip;

	Trace((2, "CheckUserPath(%s, %s, %s)",
	       user==NULLSTR?NullStr:user,
	       node==NULLSTR?NullStr:node,
	       path));

	if ( path[0] != '/' || strstr(path, "/../") != NULLSTR )
		return false;

	while ( (up = Head) == (User *)0 )
		if ( !SetupUsers() )
			return false;

	for ( ; up != (User *)0 ; up = up->next )
	{
		DODEBUG(static char	cmpstr[]	= "\tcmp: %s <> %s");

		if ( user != NULLSTR && user[0] != '\0' )
		{
			Trace((5, cmpstr, user, up->user));
			if ( strcmp(user, up->user) == STREQUAL )
				break;
		}

		if ( node != NULLSTR && node[0] != '\0' )
		{
			Trace((5, cmpstr, node, up->node));
			if ( strcmp(node, up->node) == STREQUAL )
				break;
		}
	}

	if ( up == (User *)0 )
	{
		if ( user != NULLSTR )
			up = NodeDefault;
		else
			up = UserDefault;

		if ( up == (User *)0 )
			return false;
	}
	Trace((2, "\tUSERFILE match: \"%s,%s\"\n", up->user, up->node));

	for ( ip = up->lens, cpp = up->paths ; (cp = *cpp++) != NULLSTR ; )
	{
		Trace((2, "\tCheckUserPath %s[%d] <> %s", cp, *ip, path));

		if ( strncmp(cp, path, *ip++) == STREQUAL )
			return true;
	}

	Trace((1, "\tCheckUserPath %s FAILED", path));
	return false;
}

/*
**	Read USERFILE and setup user structures for each entry.
*/

static bool
SetupUsers()
{
	register char **cpp;
	register char *	cp;
	register User *	up;
	register int	i;
	register int *	ip;
	int		nargs;
	char *		buf;
	char *		dp;
	char *		vec[MAXUSERFIELDS];

	Trace((1, "SetupUsers()"));

	if ( (buf = ReadFile(USERFILE)) == NULLSTR )
	{
		if ( RdFileTime == 0 )
			SysError(CouldNot, ReadStr, USERFILE);
empty:
		ErrVal = EX_UNAVAILABLE;
		Error("\"%s\" empty", USERFILE);
		return false;
	}

	for ( dp = buf ; (cp = GetLine(&dp)) != NULLSTR ; )
	{
		if ( (nargs = SplitSpace(vec, cp, MAXUSERFIELDS)) == 0 )
			continue;

		if ( nargs > MAXUSERFIELDS )
			Error("\"%s\" has line with >%d fields", USERFILE, MAXUSERFIELDS);

		up = Talloc(User);

		up->user = newstr(vec[0]);

		if ( (cp = strchr(up->user, ',')) != NULLSTR )
		{
			*cp++ = '\0';
			if ( strlen(cp) > NODENAMEMAXSIZE )
				cp[NODENAMEMAXSIZE] = '\0';
		}
		else
			cp = EmptyStr;
		up->node = cp;

		Trace((2, "SetupUser \"%s,%s\"", up->user, up->node));

		if ( up->user[0] == '\0' && UserDefault == (User *)0 ) {
			Trace((2, "\t(using this for UserDefault)\n"));
			UserDefault = up;
		}

		if ( up->node[0] == '\0' && NodeDefault == (User *)0 ) {
			Trace((2, "\t(using this for NodeDefault)\n"));
			NodeDefault = up;
		}

		if ( nargs > 1 && strccmp(vec[1], "c") == STREQUAL )
		{
			up->callb = true;
			i = 1;
		}
		else
		{
			up->callb = false;
			i = 0;
		}

		up->paths = cpp = (char **)Malloc((nargs-i)*sizeof(char *));
		up->lens  = ip = (int *)Malloc((nargs-i-1)*sizeof(int));
		while ( ++i < nargs )
		{
			Trace((5, "\tpath: %s", vec[i]));
			*cpp++ = newstr(vec[i]);
			*ip++  = strlen(vec[i]);
		}
		*cpp = NULLSTR;

		up->next = Head;
		Head = up;
	}

	free(buf);

	if ( Head == (User *)0 )
		goto empty;

	return true;
}
