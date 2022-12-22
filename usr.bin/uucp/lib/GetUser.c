/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Password routines.
**
**	RCSID $Id: GetUser.c,v 1.1.1.1 1992/09/28 20:08:39 trent Exp $
**
**	$Log: GetUser.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:39  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	PASSWD
#define	ERRNO

#include	"global.h"

static char	nosuchuser[]	= english("no such user");

static bool	pw_policy(struct passwd *, int *, char **, char **);



/*
**	Given a user name, find uid and
**	pass back details from the 'passwd' file.
*/

bool
GetUid(uidp, name, path)
	int *		uidp;
	char **		name;
	char **		path;
{
	register struct passwd *pw;
	extern struct passwd *	getpwnam();

	Trace((2, "GetUid(%#lx, %s, %#lx)", (Ulong)uidp, *name, (Ulong)path));

	errno = 0;

	if ( (pw = getpwnam(*name)) == (struct passwd *)0 )
	{
		endpwent();
		if ( path != (char **)0 )
			*path = nosuchuser;
		return false;
	}

	endpwent();

	return pw_policy(pw, uidp, (char **)0, path);
}



/*
**	Given a uid, find user name and
**	pass back details from the 'passwd' file.
*/

bool
GetUser(uidp, name, path)
	int *		uidp;
	char **		name;
	char **		path;
{
	register struct passwd *pw;
	char *			loginname;

	extern struct passwd *	getpwuid(uid_t);
	extern struct passwd *	getpwnam(const char *);
	extern char *		getlogin();
	extern char *		getenv();

	Trace((2, "GetUser(%d, %#lx, %#lx)", *uidp, (Ulong)name, (Ulong)path));

	errno = 0;

	if ( (loginname = getlogin()) == NULLSTR )
		loginname = getenv("USER");

	if ( loginname != NULLSTR )
	{
		pw = getpwnam(loginname);

		if ( pw != (struct passwd *)0 && pw->pw_uid == *uidp )
			goto out;
	}

	if ( (pw = getpwuid(*uidp)) == (struct passwd *)0 )
	{
		endpwent();
		if ( path != (char **)0 )
			*path = nosuchuser;
		return false;
	}
out:
	endpwent();

	return pw_policy(pw, (int *)0, name, path);
}



/*
**	Pass back details from the 'passwd' entry.
*/

static bool
pw_policy(pw, uidp, name, path)
	register struct passwd *pw;
	int *		uidp;
	char **		name;
	char **		path;
{
	Debug((4, "uid = %d, login name = %s", pw->pw_uid, pw->pw_name));

	if ( uidp != (int *)0 )
		*uidp = pw->pw_uid;

	if ( name != (char **)0 )
		*name = newstr(pw->pw_name);

	if ( path != (char **)0 )
		*path = newstr(pw->pw_dir);

	return true;
}
