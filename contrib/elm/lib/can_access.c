
static char rcsid[] = "@(#)Id: can_access.c,v 5.8 1993/08/23 02:46:07 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: can_access.c,v $
 * Revision 1.2  1994/01/14  00:53:01  cp
 * 2.4.23
 *
 * Revision 5.8  1993/08/23  02:46:07  syd
 * Don't declare _exit() if <unistd.h> already did it.
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.7  1993/08/03  19:28:39  syd
 * Elm tries to replace the system toupper() and tolower() on current
 * BSD systems, which is unnecessary.  Even worse, the replacements
 * collide during linking with routines in isctype.o.  This patch adds
 * a Configure test to determine whether replacements are really needed
 * (BROKE_CTYPE definition).  The <ctype.h> header file is now included
 * globally through hdrs/defs.h and the BROKE_CTYPE patchup is handled
 * there.  Inclusion of <ctype.h> was removed from *all* the individual
 * files, and the toupper() and tolower() routines in lib/opt_utils.c
 * were dropped.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.6  1993/05/14  03:52:10  syd
 * When compiled on a POSIX host PL22 failed checking whether the file is
 * readable and a regular file or not. There was one `!' missing in the
 * `if (S_ISREG(mode))' test which should read `if (! S_ISREG(mode))'.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.5  1993/05/08  19:16:41  syd
 * Remove symlink code from can_access, we dont care if its a symlink
 * only if we can access the file pointed to by the symlink anyway and
 * stat resolves to that file.
 *
 * Revision 5.4  1993/04/12  04:08:36  syd
 * Fix if alignment
 *
 * Revision 5.3  1993/04/12  03:33:39  syd
 * the posix macros to interpret the result of the stat-call.
 * From: vogt@isa.de (Gerald Vogt)
 *
 * Revision 5.2  1992/12/12  01:29:26  syd
 * Fix double inclusion of sys/types.h
 * From: Tom Moore <tmoore@wnas.DaytonOH.NCR.COM>
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** can_access - can this user access this file using their normal uid/gid

**/

#include "headers.h"
#include <sys/stat.h>
#include <errno.h>

#ifdef BSD
# include <sys/wait.h>
#endif

#ifndef I_UNISTD
void _exit();
#endif

extern int errno;		/* system error number */

int
can_access(file, mode)
char *file; 
int   mode;
{
	/** returns ZERO iff user can access file or "errno" otherwise **/

	int the_stat = 0, pid, w; 
	struct stat stat_buf;
#if defined(BSD) && !defined(WEXITSTATUS)
	union wait status;
#else
	int status;
#endif
	register SIGHAND_TYPE (*istat)(), (*qstat)();
	
#ifdef VFORK
	if ((pid = vfork()) == 0) {
#else
	if ((pid = fork()) == 0) {
#endif
	  setgid(groupid);
	  setuid(userid);		/** back to normal userid **/

	  errno = 0;

	  if (access(file, mode) == 0) 
	    _exit(0);
	  else 
	    _exit(errno != 0? errno : 1);	/* never return zero! */
	  _exit(127);
	}

	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);

	while ((w = wait(&status)) != pid && w != -1)
		;

#if	defined(WEXITSTATUS)
	/* Use POSIX macro if defined */
	the_stat = WEXITSTATUS(status);
#else
#ifdef BSD
	the_stat = status.w_retcode;
#else
	the_stat = status >> 8;
#endif
#endif	/*WEXITSTATUS*/

	signal(SIGINT, istat);
	signal(SIGQUIT, qstat);
	if (the_stat == 0) {
	  if (stat(file, &stat_buf) == 0) {
#ifndef _POSIX_SOURCE
	    w = stat_buf.st_mode & S_IFMT;
	    if (w != S_IFREG)
	      the_stat = 1;
#else /* _POSIX_SOURCE */
	    w = stat_buf.st_mode;

	    if (! S_ISREG(w))
	      the_stat = 1;
#endif
	  }
	}

	return(the_stat);
}
