/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	`uucico' - program to transfer messages to remote.
**
**	RCSID $Id: main.c,v 1.3 1993/02/28 15:36:39 pace Exp $
**
**	$Log: main.c,v $
 * Revision 1.3  1993/02/28  15:36:39  pace
 * Add recent uunet changes; add P_HWFLOW_ON, etc; add hayesv dialer
 *
 * Revision 1.2  1992/11/20  19:40:08  trent
 *         - a bug-fix to prevent uucico to go into infinite loops when
 *           unsuccessfully trying to send a uucp job and getting a response
 *           "CN2" (permission denied) from the remote end.  Our uucico does
 *           not delete the job on our end (thinking it's a temporary error)
 *           and tries to send the file again when it rescans for work.
 *           The code for this is found in uucico/Control.c somewhere around
 *           line 730.  See the context below.
 *
 *         - a hack to limit a uucico session to prevent a subscriber on
 *           our 900 lines (tty9*) from being able to request files after
 *           30 minutes of connect time.  This code is optional and is
 *           ifdef'ed as SPRINT_HACK.  It's the phone company's fault,
 *           really.
 *         - We recently got a new MorningStar box on a Sparc for handing
 *           our X.25 lines.  In order to determine that a line is X.25
 *           instead of a normal tty or pty, we access teh environment
 *           variable passed to uucico named "X25_CALLED_ADDRESS" (which
 *           contains the DNIC).  We used to use an ioctl with MorningStar's
 *           older product for the Sequent.  some of the code deals with
 *           locking of ttyx* names to make our connect logs backward
 *           compatable with the Sequent.  I call this a "feature".
 *
 *         - the default tty name is now "notty" instead of "ttyp0".
 *
 * Revision 1.1.1.1  1992/09/28  20:08:53  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	FILES
#define	SETJMP
#define	SYSEXITS

#include	"global.h"
#include	"cico.h"

#ifdef USE_X25
char *	DNIC_STR;	/* Environment variable retrieved at startup */
#endif /* USE_X25 */


void
main(
	int	argc,
	char *	argv[],
	char *	envp[]
)
{
#	ifdef	USE_X25
	if (DNIC_STR = getenv("X25_CALLED_ADDRESS"))
		DNIC_STR = newstr(DNIC_STR);
	else
		DNIC_STR = "";
#	endif

	SetNameProg(argc, argv, envp);

	cico_args(argc, argv);

	Checkeuid();

	do_cico();

	finish(EX_OK);
}
