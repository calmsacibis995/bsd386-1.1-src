/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Setup network user/group details.
**
**	RCSID $Id: GetNetUid.c,v 1.1.1.1 1992/09/28 20:08:39 trent Exp $
**
**	$Log: GetNetUid.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:39  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	PASSWD
#define	SYSEXITS

#include	"global.h"

#include	<grp.h>


int		NetGid;
int		NetUid;
int		E_uid;
int		E_gid;
int		R_uid;

static bool	NetUidSet;



void
GetNetUid()
{
	char *	err;

	if ( NetUidSet )
		return;

	E_uid = geteuid();
	R_uid = getuid();
	E_gid = getegid();

	if ( !GetUid(&NetUid, &UUCPUSER, &err) )
	{
		ErrVal = EX_NOUSER;
		Error(english("UUCP id \"%s\" -- %s."), UUCPUSER, err);
	}

	free(err);	/* Set by successful GetUid() */

	if ( UUCPGROUP != NULLSTR )
	{
		register struct group * gp;
		extern struct group *	getgrnam(const char *);

		if ( (gp = getgrnam(UUCPGROUP)) == (struct group *)0 )
		{
			ErrVal = EX_NOUSER;
			Error(english("UUCP group id \"%s\" does not exist!"), UUCPGROUP);
		}

		endgrent();

		NetGid = gp->gr_gid;
	}

	NetUidSet = true;

	Trace((1, "GetNetUid() => netuid=%d, netgid=%d", NetUid, NetGid));
}
