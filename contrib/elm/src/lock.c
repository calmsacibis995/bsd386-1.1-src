
static char rcsid[] = "@(#)Id: lock.c,v 5.15 1993/05/31 19:16:24 syd Exp ";

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
 * $Log: lock.c,v $
 * Revision 1.2  1994/01/14  00:55:12  cp
 * 2.4.23
 *
 * Revision 5.15  1993/05/31  19:16:24  syd
 * It looks like there was some earlier patch that re-introduced
 * some lock problems from the past time of 2.4beta.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.14  1993/04/16  14:13:49  syd
 * Make it clear errno, some systems arent
 * From: Syd
 *
 * Revision 5.13  1993/04/16  03:58:57  syd
 * Fix where Solaris 2.0 leaves an error code
 * yet creates the lock file
 * From: Syd via pointer from Daniel Bidwell <bidwell@elrond.cs.andrews.edu>
 *
 * Revision 5.12  1993/02/03  17:12:53  syd
 * move more declarations to defs.h, including sleep
 * From: Syd
 *
 * Revision 5.11  1993/01/20  03:02:19  syd
 * Move string declarations to defs.h
 * From: Syd
 *
 * Revision 5.10  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.9  1992/12/07  03:49:49  syd
 * use BSD or not apollo on file.h include as its not valid
 * for Apollos under sys5.3 compile type
 * From: gordonb@mcil.comm.mot.com (Gordon Berkley)
 *
 * Revision 5.8  1992/12/07  02:23:35  syd
 * Always init fcntlerr and flockerr so useage after ifdef code doesnt cause problem
 * From: Syd via prompt from wdh@grouper.mkt.csd.harris.com (W. David Higgins)
 *
 * Revision 5.7  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.6  1992/10/27  01:52:16  syd
 * Always include <sys/ioctl.h> in curses.c When calling ioctl()
 *
 * Remove declaration of getegid() from leavembox.c & lock.c
 * They aren't even used there.
 * From: tom@osf.org
 *
 * Revision 5.5  1992/10/19  16:50:41  syd
 * Fix a couple more compiler gripes from SYSVR4
 * From: Tom Moore <tmoore@wnas.DaytonOH.NCR.COM>
 *
 * Revision 5.4  1992/10/17  22:13:50  syd
 * ci -u src/lock.c < mail/lock.pat.i
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
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** leave current folder, updating etc. as needed...
  
**/


#include "headers.h"
#include "s_elm.h"
#include <sys/stat.h>

#ifdef USE_FCNTL_LOCKING
# define SYSCALL_LOCKING
#else
# ifdef USE_FLOCK_LOCKING
#   define SYSCALL_LOCKING
# endif
#endif

#ifdef SYSCALL_LOCKING
#  if (defined(BSD) || !defined(apollo))
#    include <sys/file.h>
#  endif
#endif
#include <errno.h>
#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif


extern int errno;

char *error_description();

#define	FLOCKING_OK	0
#define	FLOCKING_RETRY	1
#define	FLOCKING_FAIL	-1

extern char *mk_lockname();

static int  lock_state = OFF;

#ifdef	USE_DOTLOCK_LOCKING
static char *lockfile=(char*)0;
#endif  /* USE_DOTLOCK_LOCKING */

static int flock_fd,	/* file descriptor for flocking mailbox itself */
	   create_fd;	/* file descriptor for creating lock file */

#ifdef USE_FCNTL_LOCKING
static struct flock lock_info;
#endif

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
	return ((errno == EACCES) || (errno == EAGAIN))
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
	errno = flockerr;
	return flockret;
    }
    return 0;
}

lock(direction)
int direction;
{
      /** Create lock file to ensure that we don't get any mail 
	  while altering the folder contents!
	  If it already exists sit and spin until 
	     either the lock file is removed...indicating new mail
	  or
	     we have iterated MAX_ATTEMPTS times, in which case we
	     either fail or remove it and make our own (determined
	     by if REMOVE_AT_LAST is defined in header file

	  If direction == INCOMING then DON'T remove the lock file
	  on the way out!  (It'd mess up whatever created it!).

	  But if that succeeds and if we are also locking by flock(),
	  follow a similar algorithm. Now if we can't lock by flock(),
	  we DO need to remove the lock file, since if we got this far,
	  we DID create it, not another process.
      **/

      register int create_iteration = 0,
		   flock_iteration = 0;
      int kill_status;
      char pid_buffer[SHORT];

#ifdef	USE_DOTLOCK_LOCKING		/* { USE_DOTLOCK_LOCKING  */
      /* formulate lock file name */
      lockfile=mk_lockname(cur_folder);

#ifdef SAVE_GROUP_MAILBOX_ID
      setgid(mailgroupid); /* reset id so that we can get at lock file */
#endif

#ifdef PIDCHECK
      /** first, try to read the lock file, and if possible, check the pid.
	  If we can validate that the pid is no longer active, then remove
	  the lock file.
       **/
      errno = 0; /* some systems don't clear the errno flag */
      if((create_fd=open(lockfile,O_RDONLY)) != -1) {
	if (read(create_fd, pid_buffer, SHORT) > 0) {
	  create_iteration = atoi(pid_buffer);
	  if (create_iteration) {
	    kill_status = kill(create_iteration, 0);
	    if (kill_status != 0 && errno != EPERM) {
	      close(create_fd);
	      if (unlink(lockfile) != 0) {
	        MoveCursor(LINES, 0);
		Raw(OFF);
		dprint(1, (debugfile,
		  "Error %s\n\ttrying to unlink file %s (%s)\n", 
		  error_description(errno), lockfile, "lock"));
		printf(catgets(elm_msg_cat, ElmSet, ElmLeaveCouldntRemoveCurLock,
		  "\nCouldn't remove the current lock file %s\n"), lockfile);
		printf("** %s **\n", error_description(errno));
#ifdef SAVE_GROUP_MAILBOX_ID
		setgid(groupid);
#endif
		if (direction == INCOMING)
		  leave(0);
		else
		  emergency_exit();
	      }
	    }
	  }
	}
	create_iteration = 0;
      }
#endif
	      
      /* try to assert create lock file MAX_ATTEMPTS times */
      do {

	errno = 0;
	if((create_fd=open(lockfile,O_WRONLY | O_CREAT | O_EXCL,0444)) != -1)
	  break;
	else {
	  if(errno != EEXIST) {
	    /* Creation of lock failed NOT because it already exists!!! */

	    MoveCursor(LINES, 0);
	    Raw(OFF);
	    if (direction == OUTGOING) {
	      dprint(1, (debugfile, 
		"Error encountered attempting to create lock %s\n", lockfile));
	      dprint(1, (debugfile, "** %s **\n", error_description(errno)));
	      printf(catgets(elm_msg_cat, ElmSet, ElmLeaveErrorCreatingLock,
	   "\nError encountered while attempting to create lock file %s;\n"),
		lockfile);
	      printf("** %s.**\n\n", error_description(errno));
	    } else {	/* incoming - permission denied in the middle?  Odd. */
	      dprint(1, (debugfile,
	       "Can't create lock file: creat(%s) raises error %s (lock)\n", 
		lockfile, error_description(errno)));
	      printf(catgets(elm_msg_cat, ElmSet, ElmLeaveCantCreateLock,
	       "Can't create lock file! Need write permission in \"%s\".\n"),
		mailhome);
	    }
#ifdef SAVE_GROUP_MAILBOX_ID
	    setgid(groupid);
#endif
	    leave(0);
	  }
	}
	dprint(2, (debugfile,"File '%s' already exists!  Waiting...(lock)\n", 
	  lockfile));
	error1(catgets(elm_msg_cat, ElmSet, ElmLeaveWaitingToRead,
	  "Waiting to read mailbox while mail is being received: attempt #%d"),
	  create_iteration);
	sleep(5);
      } while (create_iteration++ < MAX_ATTEMPTS);
      clear_error();
      if (errno == ENOENT && create_fd >= 0)
	errno = 0; /* Some os leave the errno from the open on the creat call */

      if(errno != 0) {
	
	/* we weren't able to create the lock file */

#ifdef REMOVE_AT_LAST

	/** time to waste the lock file!  Must be there in error! **/
	dprint(2, (debugfile, 
	   "Warning: I'm giving up waiting - removing lock file(lock)\n"));
	if (direction == INCOMING)
	  PutLine0(LINES, 0, catgets(elm_msg_cat, ElmSet, ElmLeaveTimedOutRemoving,
		"\nTimed out - removing current lock file..."));
	else
	  error(catgets(elm_msg_cat, ElmSet, ElmLeaveThrowingAwayLock,
		"Throwing away the current lock file!"));

	if (unlink(lockfile) != 0) {
	  MoveCursor(LINES, 0);
	  Raw(OFF);
	  dprint(1, (debugfile,
	    "Error %s\n\ttrying to unlink file %s (%s)\n", 
	    error_description(errno), lockfile, "lock"));
	  printf(catgets(elm_msg_cat, ElmSet, ElmLeaveCouldntRemoveCurLock,
	    "\nCouldn't remove the current lock file %s\n"), lockfile);
	  printf("** %s **\n", error_description(errno));
#ifdef SAVE_GROUP_MAILBOX_ID
	  setgid(groupid);
#endif
	  if (direction == INCOMING)
	    leave(0);
	  else
	    emergency_exit();
	}

	/* we've removed the bad lock, let's try to assert lock once more */
	if((create_fd=open(lockfile,O_WRONLY | O_CREAT | O_EXCL,0444)) == -1){

	  /* still can't lock it - just give up */
	  dprint(1, (debugfile, 
	    "Error encountered attempting to create lock %s\n", lockfile));
	  dprint(1, (debugfile, "** %s **\n", error_description(errno)));
	  MoveCursor(LINES, 0);
	  Raw(OFF);
	  printf(catgets(elm_msg_cat, ElmSet, ElmLeaveErrorCreatingLock,
	  "\nError encountered while attempting to create lock file %s;\n"),
	    lockfile);
	  printf("** %s. **\n\n", error_description(errno));
#ifdef SAVE_GROUP_MAILBOX_ID
	  setgid(groupid);
#endif
	  leave(0);
	}
#else
	/* Okay...we die and leave, not updating the mailfile mbox or
	   any of those! */

	MoveCursor(LINES, 0);
	Raw(OFF);
	if (direction == INCOMING) {
	  printf(catgets(elm_msg_cat, ElmSet, ElmLeaveGivingUp,
		"\n\nGiving up after %d iterations.\n"), 
	    create_iteration);
	  printf(catgets(elm_msg_cat, ElmSet, ElmLeavePleaseTryAgain,
	  "\nPlease try to read your mail again in a few minutes.\n\n"));
	  dprint(1, (debugfile, 
	    "Warning: bailing out after %d iterations...(lock)\n",
	    create_iteration));
#ifdef SAVE_GROUP_MAILBOX_ID
	  setgid(groupid);
#endif
	  leave_locked(0);
	} else {
	  dprint(1, (debugfile, 
	   "Warning: after %d iterations, timed out! (lock)\n",
	   create_iteration));
#ifdef SAVE_GROUP_MAILBOX_ID
	  setgid(groupid);
#endif
	  printf(catgets(elm_msg_cat, ElmSet, ElmLeaveErrorTimedOutLock,
		"Timed out on locking mailbox.  Leaving program.\n"));
	  leave(0);
	}
#endif
      }

      /* If we're here we successfully created the lock file */
      dprint(5,
	(debugfile, "Lock %s %s for file %s on.\n", lockfile,
	(direction == INCOMING ? "incoming" : "outgoing"), cur_folder));

      /* Place the pid of Elm into the lock file for SVR3.2 and its ilk */
      sprintf(pid_buffer, "%d", getpid());
      write(create_fd, pid_buffer, strlen(pid_buffer));

      (void)close(create_fd);
#ifdef SAVE_GROUP_MAILBOX_ID
      setgid(groupid);
#endif
#endif	/* } USE_DOTLOCK_LOCKING */
				   
#ifdef SYSCALL_LOCKING
      /* Now we also need to lock the file with flock(2) */

      /* Open mail file separately for locking */
      if((flock_fd = open(cur_folder, O_RDWR)) < 0) {
	dprint(1, (debugfile, 
	    "Error encountered attempting to reopen %s for lock\n", cur_folder));
	dprint(1, (debugfile, "** %s **\n", error_description(errno)));
	MoveCursor(LINES, 0);
	Raw(OFF);
	printf(catgets(elm_msg_cat, ElmSet, ElmLeaveErrorReopenMailbox,
 "\nError encountered while attempting to reopen mailbox %s for lock;\n"), 
	      cur_folder);
	printf("** %s. **\n\n", error_description(errno));
#ifdef	USE_DOTLOCK_LOCKING
	(void)unlink(lockfile);
#endif /* USE_DOTLOCK_LOCKING */
	leave(0);
      }

      /* try to assert lock MAX_ATTEMPTS times */
      do {

	switch (Grab_the_file (flock_fd)) {

	case	FLOCKING_OK:
	    goto    EXIT_RETRY_LOOP;

	case	FLOCKING_FAIL:
	    /*
	     *	Creation of lock failed
	     *	NOT because it already exists!!!
	     */

	    dprint (1, (debugfile, 
			"Error encountered attempting to flock %s\n",
			cur_folder));
	    dprint (1, (debugfile, "** %s **\n", error_description(errno)));
	    MoveCursor(LINES, 0);
	    Raw(OFF);
	    printf (catgets(elm_msg_cat, ElmSet, ElmLeaveErrorFlockMailbox,
	 "\nError encountered while attempting to flock mailbox %s;\n"), 
		  cur_folder);
	    printf("** %s. **\n\n", error_description(errno));
#ifdef	USE_DOTLOCK_LOCKING
	    (void)unlink(lockfile);
#endif /* USE_DOTLOCK_LOCKING */
	    leave(0);

	    break;

	case	FLOCKING_RETRY:
	default:
	    dprint (2, (debugfile,
	      "Mailbox '%s' already locked!  Waiting...(lock)\n", cur_folder));
	    error1 (catgets(elm_msg_cat, ElmSet, ElmLeaveWaitingToRead,
			   "Waiting to read mailbox while mail is being received: attempt #%d"),
		    flock_iteration);
	    sleep(5);
	}
      } while (flock_iteration++ < MAX_ATTEMPTS);

EXIT_RETRY_LOOP:
      clear_error();

      if(errno != 0) {

	MoveCursor(LINES, 0);
	Raw(OFF);
	/* We couldn't lock the file. We die and leave not updating
	 * the mailfile mbox or any of those! */

	if (direction == INCOMING) {
	  printf(catgets(elm_msg_cat, ElmSet, ElmLeaveGivingUp,
		"\n\nGiving up after %d iterations.\n"), 
	    flock_iteration);
	  printf(catgets(elm_msg_cat, ElmSet, ElmLeavePleaseTryAgain,
	  "\nPlease try to read your mail again in a few minutes.\n\n"));
	  dprint(1, (debugfile, 
	    "Warning: bailing out after %d iterations...(lock)\n",
	    flock_iteration));
	} else {
	  dprint(1, (debugfile, 
	   "Warning: after %d iterations, timed out! (lock)\n",
	   flock_iteration));
	}
#ifdef	USE_DOTLOCK_LOCKING
	(void)unlink(lockfile);
#endif
	printf(catgets(elm_msg_cat, ElmSet, ElmLeaveErrorTimedOutLock,
		"Timed out on locking mailbox.  Leaving program.\n"));
	leave(0);
      }

      /* We locked the file */
      dprint(5,
	(debugfile, "Lock %s on file %s on.\n",
	(direction == INCOMING ? "incoming" : "outgoing"), cur_folder));
#endif /* SYSCALL_LOCKING */

      dprint(5,
	(debugfile, "Lock %s for file %s on successfully.\n",
	(direction == INCOMING ? "incoming" : "outgoing"), cur_folder));
      lock_state = ON;
      return(0);
}

int
unlock()
{
	/** Remove the lock file!    This must be part of the interrupt
	    processing routine to ensure that the lock file is NEVER
	    left sitting in the mailhome directory!

	    If also using flock(), remove the file lock as well.
	 **/

	int retcode = 0;

#ifndef USE_DOTLOCK_LOCKING
	dprint(5,
	  (debugfile, "Lock (no .lock) for file %s %s off.\n",
	    cur_folder, (lock_state == ON ? "going" : "already")));
#else   /* USE_DOTLOCK_LOCKING */
	dprint(5,
	  (debugfile, "Lock %s for file %s %s off.\n",
 	    (lockfile ? lockfile : "none"),
	    cur_folder,
	    (lock_state == ON ? "going" : "already")));
#endif  /* USE_DOTLOCK_LOCKING */

	if(lock_state == ON) {

#ifdef SYSCALL_LOCKING
	    if (retcode = Release_the_file (flock_fd)) {

		dprint(1, (debugfile,
			   "Error %s\n\ttrying to unlock file %s (%s)\n", 
			   error_description(errno), cur_folder, "unlock"));

		/* try to force unlock by closing file */
		if (close (flock_fd) == -1) {
		    dprint (1, (debugfile,
	      "Error %s\n\ttrying to force unlock file %s via close() (%s)\n", 
				error_description(errno),
				cur_folder, "unlock"));
		    error1 (catgets (elm_msg_cat, ElmSet,
				     ElmLeaveCouldntUnlockOwnMailbox,
				     "Couldn't unlock my own mailbox %s!"),
				     cur_folder);
		    return(retcode);
		}
	    }
	    (void)close(flock_fd);
#endif
#ifndef	USE_DOTLOCK_LOCKING	/* { USE_DOTLOCK_LOCKING */
	   lock_state = OFF;		/* indicate we don't have a lock on */
#else
#ifdef SAVE_GROUP_MAILBOX_ID
	  setgid(mailgroupid);
#endif
	  if((retcode = unlink(lockfile)) == 0) {	/* remove lock file */
	    *lockfile = '\0';		/* null lock file name */
	    lock_state = OFF;		/* indicate we don't have a lock on */
	  } else {
	    dprint(1, (debugfile,
	      "Error %s\n\ttrying to unlink file %s (%s)\n", 
	      error_description(errno), lockfile,"unlock"));
	      error1(catgets(elm_msg_cat, ElmSet, ElmLeaveCouldntRemoveOwnLock,
		"Couldn't remove my own lock file %s!"), lockfile);
	  }
#ifdef SAVE_GROUP_MAILBOX_ID
	  setgid(groupid);
#endif
#endif	/* } !USE_DOTLOCK_LOCKING */
	}
	return(retcode);
}
