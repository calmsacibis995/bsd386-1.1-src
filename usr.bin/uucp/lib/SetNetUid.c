/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Switch between UUCP and original user.
**
**	RCSID $Id: SetNetUid.c,v 1.1.1.1 1992/09/28 20:08:42 trent Exp $
**
**	$Log: SetNetUid.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:42  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#include	"global.h"


static char	SetEUStr[]	= "seteuid";
static char	SetEGStr[]	= "setegid";
static char	CouldNotD[]	= "Could not %s to %d";



void
SetNetUid()
{
	GetNetUid();

	if ( R_uid != 0 && R_uid != NetUid && E_uid == 0 )
		(void)setruid(0);

	if ( E_uid != 0 && R_uid == 0 )
		(void)seteuid(0);

	if ( E_gid != NetGid && setegid(NetGid) == SYSERROR )
		SysWarn(CouldNot, SetEGStr, UUCPGROUP);

	if ( E_uid != NetUid && R_uid == 0 && seteuid(NetUid) == SYSERROR )
		SysError(CouldNot, SetEUStr, UUCPUSER);

	Trace((1, "SetUid() => e=%d, r=%d", geteuid(), getuid()));
}

void
RestoreUid()
{
	if ( E_uid != NetUid && seteuid(E_uid) == SYSERROR )
		SysError(CouldNotD, SetEUStr, E_uid);

	if ( R_uid != 0 && setruid(R_uid) == SYSERROR )
		SysError(CouldNotD, "setruid", R_uid);

	if ( E_gid != NetGid && setegid(E_gid) == SYSERROR )
		SysError(CouldNotD, SetEGStr, E_gid);

	Trace((1, "RestoreUid() => e=%d, r=%d", geteuid(), getuid()));
}
