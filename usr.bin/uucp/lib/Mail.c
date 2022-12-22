/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Send network originated report via mail to `users'.
**
**	RCSID $Id: Mail.c,v 1.1.1.1 1992/09/28 20:08:42 trent Exp $
**
**	$Log: Mail.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:42  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	EXECUTE
#define	STDIO

#include	"global.h"


#define	Fprintf	(void)fprintf



char *
Mail(
	vFuncp		funcp,
	char *		users,
	char *		subj
)
{
	register int	n;
	register FILE *	fd;
	char *		cp;
	ExBuf		args;

	if ( SourceAddress == NULLSTR )
		SourceAddress = NODENAME;

	if ( SenderName == NULLSTR )
		SenderName = Invoker;

	if ( UserName == NULLSTR )
		UserName = SenderName;

	FIRSTARG(&args.ex_cmd) = MAILPROG;
	ExpandArgs(&args.ex_cmd, MAILPROGARGS);	/* Usually SenderName@SourceAddress */

	if ( users == NULLSTR || SplitArgs(&args.ex_cmd, users) == 0 )
	{
		n = NARGS(&args.ex_cmd);	/* These should be free'd */
		NEXTARG(&args.ex_cmd) = users = "root";
	}
	else
		n = NARGS(&args.ex_cmd);	/* These should be free'd */

	fd = (FILE *)Execute(&args, NULLVFUNCP, ex_pipe, SYSERROR);

	Fprintf(fd, english("From: %s@%s\n"), SenderName, NODENAME);

	Fprintf(fd, english("Date: %s"), rfc822time((long *)&Time));

	if ( strchr(users, '@') == NULLSTR )
		Fprintf(fd, english("To: %s@%s\n"), users, NODENAME);
	else
		Fprintf(fd, english("To: %s\n"), users);

	if ( subj != NULLSTR )
		Fprintf(fd, english("Subject: %s\n"), subj);

	if ( funcp != NULLVFUNCP )
		(*funcp)(fd);

	cp = ExClose(&args, fd);

	while ( --n > 0 )
		free(ARG(&args.ex_cmd, n));

	return cp;
}
