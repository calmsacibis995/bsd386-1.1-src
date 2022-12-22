
static char rcsid[] ="@(#)Id: lock.c,v 5.6 1993/01/19 03:55:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * 			Copyright (c) 1988-1992 USENET Community Trust
 * 			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein - elm@DSI.COM
 *			dsinc!elm
 *
 *******************************************************************************
 * $Log: lock.c,v $
 * Revision 1.2  1994/01/14  00:51:35  cp
 * 2.4.23
 *
 * Revision 5.6  1993/01/19  03:55:39  syd
 * exitprog.c makes a reference to a null character pointer, savecopy.c
 * tries to reference an uninitialized variable, and the previous patch to
 * src/lock.c to get rid of an uninitialized variable compiler message
 * needed to be put in filter/lock.c as well.
 * From: wdh@grouper.mkt.csd.harris.com (W. David Higgins)
 *
 * Revision 5.5  1992/12/07  03:49:49  syd
 * use BSD or not apollo on file.h include as its not valid
 * for Apollos under sys5.3 compile type
 * From: gordonb@mcil.comm.mot.com (Gordon Berkley)
 *
 * Revision 5.4  1992/10/17  22:35:33  syd
 * Change lock file name to add user name on filter locking of mail spool
 * From: Peter Brouwer <pb@idca.tds.philips.nl>
 *
 * Revision 5.3  1992/10/12  00:25:52  syd
 * Lock error codes (fcntl vs flock) were backwards
 * From: howardl@wb3ffv.ampr.org (Howard Leadmon - WB3FFV)
 *
 * Revision 5.2  1992/10/11  00:52:11  syd
 * Switch to wrapper for flock and fcntl locking.
 * Change order to fcntl first, other order blocked.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.1  1992/10/03  22:18:09  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/


/** The lock() and unlock() routines herein duplicate exactly the
    equivalent routines in the Elm Mail System, and should also be
    compatible with sendmail, rmail, etc etc.

  
**/

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "defs.h"
#include "filter.h"
#include "s_filter.h"

static  int  we_locked_it;
static char *lockfile=(char*)0;

extern char *mk_lockname();

#ifdef	USE_FLOCK_LOCKING
#define	NEEDS_FLOCK_FD
#include <sys/types.h>
#  if (defined(BSD) || !defined(apollo))
#    include <sys/file.h>
#  endif
#endif

#ifdef USE_FCNTL_LOCKING
#define	NEEDS_FLOCK_FD
static struct flock lock_info;
#endif

#ifdef NEEDS_FLOCK_FD
static	flock_fd = -1;
static	char flock_name[SLEN];
#endif

#ifdef	USE_DOTLOCK_LOCKING			/*  USE_DOTLOCK_LOCKING	*/
static	char dotlock_name[SLEN];
#endif

#define	FLOCKING_OK	0
#define	FLOCKING_RETRY	1
#define	FLOCKING_FAIL	-1

extern  int  errno;

int
Grab_the_file(flock_fd)
int flock_fd;
{
    int	retcode	= FLOCKING_OK;

    errno = 0;

#ifdef   USE_FCNTL_LOCKING
    lock_info.l_type = F_WRLCK;
    lock_info.l_whence = 0;
    lock_info.l_start = 0;
    lock_info.l_len = 0;

    if (fcntl(flock_fd, F_SETLK, &lock_info)) {
	return ((errno == EACCES) || (EAGAIN))
		? FLOCKING_RETRY
		: FLOCKING_FAIL ;
    }
#endif

#ifdef	USE_FLOCK_LOCKING
    if (flock (flock_fd, LOCK_NB | LOCK_EX)) {

	retcode = ((errno == EWOULDBLOCK) || (errno == EAGAIN))
		   ? FLOCKING_RETRY
		   : FLOCKING_FAIL ;

#ifdef USE_FCNTL_LOCKING
	lock_info.l_type = F_UNLCK;
	lock_info.l_whence = 0;
	lock_info.l_start = 0;
	lock_info.l_len = 0;

	/*
	 *  Just unlock it because we did not succeed with the
	 *  flock()-style locking. Never mind the return value.
	 *  It was our own lock anyway if we ever got this far.
	 */
	fcntl (flock_fd, F_SETLK, &lock_info);
#endif
	return retcode;
    }
#endif

    return FLOCKING_OK;
}

int
Release_the_file(flock_fd)
int flock_fd;
{
    int	fcntlret = 0,
	flockret = 0,
	fcntlerr = 0,
	flockerr = 0;

#ifdef	USE_FLOCK_LOCKING
    errno = 0;
    flockret = flock (flock_fd, LOCK_UN);
    flockerr = errno;
#endif

#ifdef	USE_FCNTL_LOCKING
    lock_info.l_type = F_UNLCK;
    lock_info.l_whence = 0;
    lock_info.l_start = 0;
    lock_info.l_len = 0;

    errno = 0;
    fcntlret = fcntl (flock_fd, F_SETLK, &lock_info);
    fcntlerr = errno;
#endif

    if (fcntlret) {
	errno = fcntlerr;
	return fcntlret;
    }
    else if (flockret) {
	errno = flockret;
	return flockret;
    }
    return 0;
}

int
lock()
{
	/** This routine will return 1 if we could lock the mailfile,
	    zero otherwise.
	**/

	int attempts = 0, ret;

#ifdef	USE_DOTLOCK_LOCKING			/* { USE_DOTLOCK_LOCKING	*/
	strcpy(dotlock_name,mailhome);
	strcat(dotlock_name,username);
	lockfile = mk_lockname(dotlock_name);
#ifdef PIDCHECK
	/** first, try to read the lock file, and if possible, check the pid.
	    If we can validate that the pid is no longer active, then remove
	    the lock file.
	**/
	if((ret=open(lockfile,O_RDONLY)) != -1) {
	  char pid_buffer[SHORT];
	  if (read(ret, pid_buffer, SHORT) > 0) {
	    attempts = atoi(pid_buffer);
	    if (attempts) {
	      if (kill(attempts, 0)) {
	        close(ret);
	        if (unlink(lockfile) != 0)
		  return(1);
	      }
	    }
	  }
	  attempts = 0;
        }
#endif

	while ((ret = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, 0444)) < 0 
	       && attempts++ < 10) {
	  sleep(3);	/* wait three seconds each pass, okay?? */
	}

	if (ret >= 0) {
	  we_locked_it++;
	  close(ret);			/* no need to keep it open! */
	  ret = 1;
	} else {
	  ret = 0;
	}
	  
#endif					/* } USE_DOTLOCK_LOCKING	*/

#ifdef	NEEDS_FLOCK_FD			/* { NEEDS_FLOCK_FD	*/
	(void)sprintf(flock_name,"%s%s",mailhome,username);
	flock_fd = open(flock_name, O_RDWR | O_CREAT, 0600);

	if ( flock_fd >= 0 ) {
	    for (attempts = 0; attempts < 10; attempts++) {

		ret = Grab_the_file (flock_fd);
	    
		if ((ret == FLOCKING_FAIL) || (ret == FLOCKING_OK)) {
		    break;
		}
		(void)sleep((unsigned)3);
	    }

	    if ( ret == FLOCKING_OK ) {
		we_locked_it++;
		ret = 1;
	    }
	    else {
		we_locked_it = 0;
		if ( lockfile[0] ) {
		    (void)unlink(lockfile);
		    lockfile[0] = 0;
		}
		(void)close(flock_fd);
		flock_fd = -1;
		ret = 0;
	    }
	}
#endif					 /* { NEEDS_FLOCK_FD	*/
	return(ret);
}

unlock()
{
	/** this routine will remove the lock file, but only if we were
	    the people that locked it in the first place... **/

#ifdef	USE_DOTLOCK_LOCKING
	if (we_locked_it && lockfile[0]) {
	    unlink(lockfile);	/* blamo! */
	    lockfile[0] = 0;
	}
#endif
#ifdef	NEED_FLOCK_FD
	if (we_locked_it && flock_fd >= 0) {
	    /*
	     *	Give it at least a decent try. OK?
	     */
	    (void) Release_the_file (flock_fd);
	    /*
	     *	And then blast the locks anyway...
	     */
	    (void) close (flock_fd);
	    flock_fd = -1;
	}
#endif
	we_locked_it = 0;
}
