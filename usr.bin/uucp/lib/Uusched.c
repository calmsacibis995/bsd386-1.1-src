/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Exec uusched.
**
**	RCSID $Id: Uusched.c,v 1.1.1.1 1992/09/28 20:08:44 trent Exp $
**
**	$Log: Uusched.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:44  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	EXECUTE
#define	PARAMS
#define	STDIO
#define	SYSEXITS

#include	"global.h"



void
Uusched()
{
	ExBuf		args;

	if ( UUSCHED == NULLSTR )
		exit(EX_OK);

	FIRSTARG(&args.ex_cmd) = UUSCHED;

#	if	DEBUG
	if ( Traceflag > 0 )
		NEXTARG(&args.ex_cmd) = newprintf("-T%d", Traceflag);
#	endif	/* DEBUG */

	if ( Debugflag > 0 )
		NEXTARG(&args.ex_cmd) = newprintf("-x%d", Debugflag);

	if ( NewParamsFile )
		NEXTARG(&args.ex_cmd) = concat("-P", PARAMSFILE, NULLSTR);

	ExpandArgs(&args.ex_cmd, UUSCHEDARGS);

	(void)Execute(&args, ExIgnSigs, ex_nofork, SYSERROR);

	/** NO RETURN **/
}
