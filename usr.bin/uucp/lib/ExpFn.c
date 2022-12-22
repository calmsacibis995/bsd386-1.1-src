/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Expand file name: add current dir path, expand ~.
**	Writes expanded file name back over parameter.
**
**	RCSID $Id: ExpFn.c,v 1.2 1993/02/28 15:28:41 pace Exp $
**
**	$Log: ExpFn.c,v $
 * Revision 1.2  1993/02/28  15:28:41  pace
 * Add recent uunet changes
 *
 * Revision 1.1.1.1  1992/09/28  20:08:36  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"

char *		InvokeDir;	/* Invoker's working directory */

bool
ExpFn(
	register char *	file
)
{
	register char *	cp;
	register char *	xp;
	char *		user;
	char *		path;
	int		uid;
	bool		result;
	char		dir[PATHNAMESIZE];
	extern char *	getcwd(char *, size_t);

	Trace((1, "ExpFn(%s)", file));

	result = false;		/* Private directory */

	switch ( file[0] )
	{
	case '\\':	(void)strcpy(file, file+1);
	case '/':	break;

	case '~':	if ( (cp = strchr(++file, '/')) != NULLSTR )
			{
				user = strncpy(dir, file, cp-file);
				user[cp-file] = '\0';
			}
			else
				user = file;

			if ( strcmp(user, Invoker) == STREQUAL )
			{
				xp = strcpyend(dir, HomeDir);
			}
			else
			if ( user[0] == '\0' || !GetUid(&uid, &user, &path) )
			{
				xp = strcpyend(dir, PUBDIR);
				xp--;	/* Back over trailing '/' */
			}
			else
			{
				xp = strcpyend(dir, path);
				free(path);
			}

			if ( cp != NULLSTR )
				(void)strcpy(xp, cp);	/* Pathname on end */
			(void)strcpy(--file, dir);
			break;

	default:	if ( InvokeDir == NULLSTR )
			{
				while ( (cp = getcwd(dir, sizeof dir)) == NULLSTR )
					SysError(CouldNot, "get current directory", dir);

				Trace((1, "getwd(3) => %s", cp));
				InvokeDir = concat(cp, Slash, NULLSTR);
			}

			if ( strchr(file, '/') == NULLSTR )
				result = true;	/* Not at all sure why */

			xp = strcpyend(dir, InvokeDir);
			(void)strcpy(xp, file);	/* Pathname on end */
			(void)strcpy(file, dir);
			break;
	}

	Trace((2, "\tExpFn => %s [%s]", file, result?"true":"false"));

	return result;
}
