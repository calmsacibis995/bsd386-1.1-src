/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`uuxqt' - program to execute commands on remote.
**
**	RCSID $Id: main.c,v 1.1.1.1 1992/09/28 20:09:03 trent Exp $
**
**	$Log: main.c,v $
 * Revision 1.1.1.1  1992/09/28  20:09:03  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	SYSEXITS

#include	"global.h"



int
main(
	int	argc,
	char *	argv[],
	char *	envp[]
)
{
	SetNameProg(argc, argv, envp);

	uuxqt_args(argc, argv, NULLSTR);

	Checkeuid();

	(void)setgid(NetGid);
	(void)setuid(NetUid);

	do_uuxqt();

	finish(EX_OK);
}
