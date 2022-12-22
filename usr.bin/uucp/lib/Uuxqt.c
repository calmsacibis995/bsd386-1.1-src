/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Start uuxqt (no wait).
**
**	RCSID $Id: Uuxqt.c,v 1.1.1.1 1992/09/28 20:08:41 trent Exp $
**
**	$Log: Uuxqt.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:41  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	EXECUTE
#define	PARAMS
#define	STDIO

#include	"global.h"



void
Uuxqt(
	char *		node
)
{
	register int	n;
	ExBuf		args;

	if ( UUXQT == NULLSTR )
		return;

	if ( node == NULLSTR || node[0] == '\0' )
	{
		/** Only scan all if requested **/

		Trace((1, "Uuxqt - access %s", NEEDUUXQT));

		if ( access(NEEDUUXQT, 0) == SYSERROR )
			return;

		(void)unlink(NEEDUUXQT);
	}

	FIRSTARG(&args.ex_cmd) = UUXQT;

#	if	DEBUG
	if ( Traceflag > 0 )
		NEXTARG(&args.ex_cmd) = newprintf("-T%d", Traceflag);
#	endif	/* DEBUG */

	if ( Debugflag > 0 )
		NEXTARG(&args.ex_cmd) = newprintf("-x%d", Debugflag);

	if ( NewParamsFile )
		NEXTARG(&args.ex_cmd) = concat("-P", PARAMSFILE, NULLSTR);

	ExpandArgs(&args.ex_cmd, UUXQTARGS);

	if ( node != NULLSTR && node[0] != '\0' )
		NEXTARG(&args.ex_cmd) = newprintf("-s%s", node);

	n = NARGS(&args.ex_cmd);	/* These should be free'd */

	(void)Execute(&args, ExIgnSigs, ex_nowait, SYSERROR);

	while ( --n > 0 )		/* Free all except first */
		free(ARG(&args.ex_cmd, n));
}
