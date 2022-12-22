/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Start uucico (optional wait).
**
**	RCSID $Id: Uucico.c,v 1.1.1.1 1992/09/28 20:08:41 trent Exp $
**
**	$Log: Uucico.c,v $
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


extern bool	IgnoreTimeToCall;
extern bool	LocalOnly;
extern bool	ReverseRole;
extern int	TurnTime;


void
Uucico(
	char *		node,
	Wait		wait
)
{
	register int	n;
	char *		errs;
	ExBuf		args;

	DODEBUG(if(node==NULLSTR)Fatal("No node for %s", UUCICO));

	if ( UUCICO == NULLSTR )
		return;

	FIRSTARG(&args.ex_cmd) = UUCICO;

#	if	DEBUG
	NEXTARG(&args.ex_cmd) = newprintf("-T%d", Traceflag);
#	endif	/* DEBUG */

	NEXTARG(&args.ex_cmd) = newprintf("-x%d", Debugflag);

	if ( NewParamsFile )
		NEXTARG(&args.ex_cmd) = concat("-P", PARAMSFILE, NULLSTR);

	ExpandArgs(&args.ex_cmd, UUCICOARGS);

	NEXTARG(&args.ex_cmd) = newprintf("-%c%s", IgnoreTimeToCall?'S':'s', node);

	if ( TurnTime > 0 && TurnTime != DEFAULT_CICO_TURN )
		NEXTARG(&args.ex_cmd) = newprintf("-t%d", TurnTime);

	n = NARGS(&args.ex_cmd);	/* These should be free'd */

	if ( LocalOnly )
		NEXTARG(&args.ex_cmd) = "-L";
	if ( ReverseRole )
		NEXTARG(&args.ex_cmd) = "-R";

	errs = Execute(&args, ExIgnSigs, (wait==WAIT)?ex_exec:ex_nowait, SYSERROR);

	if ( errs != NULLSTR )
	{
		Warn(errs);
		free(errs);
	}

	while ( --n > 0 )		/* Free all except first */
		free(ARG(&args.ex_cmd, n));
}
