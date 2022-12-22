
static char rcsid[] = "@(#)Id: arepdaem.c,v 5.13 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * 			Copyright (c) 1988-1992 USENET Community Trust
 * 			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: arepdaem.c,v $
 * Revision 1.2  1994/01/14  00:56:08  cp
 * 2.4.23
 *
 * Revision 5.13  1993/08/03  19:28:39  syd
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
 * Revision 5.12  1993/05/31  19:39:24  syd
 * check for missing replyfile
 * From: roy@lorien.gatech.edu (Roy Mongiovi)
 *
 * Revision 5.11  1993/02/03  20:18:52  syd
 * with unistd being included now, setpgrp complains about
 * calling sequence errors, force appropriate type.
 * From: Syd
 *
 * Revision 5.10  1993/01/20  03:37:16  syd
 * Nits and typos in the NLS messages and corresponding default messages.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.9  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.8  1992/12/07  04:21:58  syd
 * add missing err declares
 * From: Syd
 *
 * Revision 5.7  1992/12/07  03:00:27  syd
 * Fix long -> time_t
 * From: Syd
 *
 * Revision 5.6  1992/11/26  00:46:50  syd
 * Fix how errno is used so err is inited and used instead
 * as errno gets overwritten by print system call
 * From: Syd
 *
 * Revision 5.5  1992/11/07  20:05:52  syd
 * change to use header_cmp to allow for linear white space around the colon
 * From: Syd
 *
 * Revision 5.4  1992/10/27  01:43:40  syd
 * Move posix_signal to lib directory
 * From: tom@osf.org
 *
 * Revision 5.3  1992/10/25  02:54:00  syd
 * add posix signal stuff as stop gap, needs to be moved to lib
 * From: syd
 *
 * Revision 5.2  1992/10/11  00:59:39  syd
 * Fix some compiler warnings that I receive compiling Elm on my SVR4
 * machine.
 * From: Tom Moore <tmoore@fievel.DaytonOH.NCR.COM>
 *
 * Revision 5.1  1992/10/04  00:46:45  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** Keep track of mail as it arrives, and respond by sending a 'recording'
    file to the sender as new mail is received.

    Note: the user program that interacts with this program is the
    'autoreply' program and that should be consulted for further
    usage information.

    This program is part of the 'autoreply' system, and is designed
    to run every hour and check all mailboxes listed in the file 
    "/etc/autoreply.data", where the data is in the form:

	username	replyfile	current-mailfile-size

    To avoid a flood of autoreplies, this program will NOT reply to mail 
    that contains header "X-Mailer: fastmail".  Further, each time the 
    program responds to mail, the 'mailfile size' entry is updated in
    the file /etc/autoreply.data to allow the system to be brought 
    down and rebooted without any loss of data or duplicate messages.

    This daemon also uses a lock semaphore file, /usr/spool/uucp/LCK..arep,
    to ensure that more than one copy of itself is never running.  For this
    reason, it is recommended that this daemon be started up each morning
    from cron, since it will either start since it's needed or simply see
    that the file is there and disappear.

    Since this particular program is the main daemon answering any
    number of different users, it must be run with uid root.
**/

#include "elmutil.h"
#include "s_arepdaem.h"

#ifdef BSD
# include <sys/time.h>
#else
# include <time.h>
#endif

#include <sys/stat.h>
#include <errno.h>

#define arep_lock_file	"LCK..arep"

#define autoreply_file	"/etc/autoreply.data"

#define logfile		"/etc/autoreply.log"	/* first choice   */
#define logfile2	"/tmp/autoreply.log"	/* second choice  */

#define BEGINNING	0		/* see fseek(3S) for info */
#define SLEEP_TIME	3600		/* run once an hour       */
#define MAX_PEOPLE	20		/* max number in program  */

#define EXISTS		00		/* lock file exists??     */

#define remove_return(s)	if (strlen(s) > 0) { \
			          if (s[strlen(s)-1] == '\n') \
				    s[strlen(s)-1] = '\0'; \
		                }

struct replyrec {
	char 	username[NLEN];		/* login name of user */
	char	mailfile[SLEN];		/* name of mail file  */
	char    replyfile[SLEN];	/* name of reply file */
	long    mailsize;		/* mail file size     */
	int     in_list;		/* for new replies    */
      } reply_table[MAX_PEOPLE];

FILE  *logfd;				/* logfile (log action)   */
time_t autoreply_time = 0L;		/* modif date of autoreply file */
int   active = 0;			/* # of people 'enrolled' */

static char	*def_subj,		/* Default subject text              */
		*subj_fw,		/* First word of subject             */
		*arep_subj;		/* Autorepysubject with subject text */

FILE  *open_logfile();			/* forward declaration    */

long  bytes();				/*       ditto 		  */
time_t ModTime();			/*       ditto		  */
void read_autoreply_file();

SIGHAND_TYPE	term_signal();

extern int errno;	/* system error number! */

main()
{
	long size;
	int  person, data_changed;
	time_t last_mod_time;

	if (fork()) exit(0);

	if (! lock())
	  exit(0);	/* already running! */

	signal(SIGTERM, term_signal); 	/* Terminate signal         */

#ifdef BSD
	person = getpid();
	setpgrp(person, person);
#else
	setpgrp();
#endif

#ifdef I_LOCALE
	setlocale(LC_ALL, "");
#endif

	elm_msg_cat = catopen("elm2.4", 0);

	/* Get the subject texts */

	def_subj = catgets(elm_msg_cat, ArepdaemSet, ArepdaemDefSubj,
	  "Auto-reply Mail");
	subj_fw = catgets(elm_msg_cat, ArepdaemSet, ArepdaemSubjFw,
	  "Auto-reply");
	arep_subj = catgets(elm_msg_cat, ArepdaemSet, ArepdaemArepSubj,
	  "Auto-reply to:%s");

	while (1) {

	  logfd = open_logfile();	/* open the log */

	  /* 1. check to see if autoreply table has changed.. */

	  if ((last_mod_time = ModTime(autoreply_file)) != autoreply_time) {
	    read_autoreply_file(); 
	    autoreply_time = last_mod_time;
	  }

	  /* 2. now for each active person... */
	
	  data_changed = 0;

	  for (person = 0; person < active; person++) {
	    if (access(reply_table[person].replyfile, READ_ACCESS) == -1)
	    {
	       log_msg(catgets(elm_msg_cat, ArepdaemSet, ArepdaemRemovingUser,
		 "removing %s from the active list"),
		      reply_table[person].username);
	       strcpy(reply_table[person].username, 
		      reply_table[active-1].username);
	       strcpy(reply_table[person].mailfile, 
		      reply_table[active-1].mailfile);
	       strcpy(reply_table[person].replyfile, 
		      reply_table[active-1].replyfile);
	       reply_table[person].mailsize = reply_table[active-1].mailsize;
	       active--;
	       data_changed++;
	    }
	    else
	       if ((size = bytes(reply_table[person].mailfile)) != reply_table[person].mailsize) {
		  if (size > reply_table[person].mailsize)
		     read_newmail(person);
		  /* else mail removed - resync */
		  reply_table[person].mailsize = size;
		  data_changed++;
	     }
	  }

	  /* 3. if data changed, update autoreply file */

	  if (data_changed)
	    update_autoreply_file();

	  close_logfile();		/* close the logfile again */

	  /* 4. Go to sleep...  */

	  sleep(SLEEP_TIME);
	}
}

void
read_autoreply_file()
{
	/** We're here because the autoreply file has changed!!  It
	    could either be because someone has been added or because
	    someone has been removed...since the list will always be in
	    order (nice, eh?) we should have a pretty easy time of it...
	**/

	FILE *file;
	char username[SLEN], 	replyfile[SLEN];
	int  person;
 	long size;
	
	log_msg(catgets(elm_msg_cat, ArepdaemSet, ArepdaemDataFileChange,
	  "Autoreply data file has changed!  Reading..."));

/*
 * clear old entries prior to reread
 */
	for (person = 0; person < active; person++)
	  reply_table[person].in_list = 0;

	if ((file = fopen(autoreply_file,"r")) == NULL) {
	  log_msg(catgets(elm_msg_cat, ArepdaemSet, ArepdaemNoOne,
	    "No-one is using autoreply..."));
	} else {
	  while (fscanf(file, "%s %s %dl", username, replyfile, &size) != EOF) {
	    /* check to see if this person is already in the list */
	    if ((person = in_reply_list(username)) != -1) {
	      reply_table[person].in_list = 1;
	      reply_table[person].mailsize = size;	 /* sync */
	    }
	    else { 	/* if not, add them */
	      if (active == MAX_PEOPLE) {
		unlock();
		exit(log_msg(catgets(elm_msg_cat, ArepdaemSet, ArepdaemMaxPeople,
		  "Couldn't add %s - already at max people!"), 
			   username));
	      }
	      log_msg(catgets(elm_msg_cat, ArepdaemSet, ArepdaemAddingUser,
		"adding %s to the active list"), username);
	      strcpy(reply_table[active].username, username);
	      sprintf(reply_table[active].mailfile, "%s%s", mailhome, username);
	      strcpy(reply_table[active].replyfile, replyfile);
	      reply_table[active].mailsize = size;
	      reply_table[active].in_list = 1;	/* obviously! */
	      active++;
	    }
	  }
	  fclose(file);
	}

	/** now check to see if anyone has been removed... **/

	person = 0;
	while (person < active)
	  if (reply_table[person].in_list && access(reply_table[person].replyfile, READ_ACCESS) == 0) {
	    person++;
	  }
	  else {
	    log_msg(catgets(elm_msg_cat, ArepdaemSet, ArepdaemRemovingUser,
	      "removing %s from the active list"),
		   reply_table[person].username);
	    strcpy(reply_table[person].username, 
		   reply_table[active-1].username);
	    strcpy(reply_table[person].mailfile, 
		   reply_table[active-1].mailfile);
	    strcpy(reply_table[person].replyfile, 
		   reply_table[active-1].replyfile);
	    reply_table[person].mailsize = reply_table[active-1].mailsize;
	    active--;
	  }
}

update_autoreply_file()
{
	/** update the entries in the autoreply file... **/

	FILE *file;
	register int person;

	if ((file = fopen(autoreply_file,"w")) == NULL) {
	  log_msg(catgets(elm_msg_cat, ArepdaemSet, ArepdaemErrUpdate,
	    "Couldn't update autoreply file!"));
	  return;
	}

	for (person = 0; person < active; person++)
	  fprintf(file, "%s %s %ld\n",
		  reply_table[person].username,
		  reply_table[person].replyfile,
		  reply_table[person].mailsize);

	fclose(file);

	autoreply_time = ModTime(autoreply_file);
}

int
in_reply_list(name)
char *name;
{
	/** search the current active reply list for the specified username.
	    return the index if found, or '-1' if not. **/

	register int iindex;

	for (iindex = 0; iindex < active; iindex++)
	  if (strcmp(name, reply_table[iindex].username) == 0)
	    return(iindex);
	
	return(-1);
}

read_newmail(person)
int person;
{
	/** Read the new mail for the specified person. **/

	
	FILE *mailfile;
	char from_whom[SLEN], subject[SLEN];
	int  sendit;

	log_msg(catgets(elm_msg_cat, ArepdaemSet, ArepdaemNewMail,
	  "New mail for %s"), reply_table[person].username);

        if ((mailfile = fopen(reply_table[person].mailfile,"r")) == NULL)
           return(log_msg(catgets(elm_msg_cat, ArepdaemSet, ArepdaemErrOpen,
	     "can't open mail file for user %s"),
		reply_table[person].username));

        if (fseek(mailfile, reply_table[person].mailsize, BEGINNING) == -1) {
	   fclose(mailfile);
           return(log_msg(catgets(elm_msg_cat, ArepdaemSet, ArepdaemErrSeek,
	     "couldn't seek to %ld in mail file!"),
	   	reply_table[person].mailsize));
	}

	while (get_return(mailfile, person, from_whom, subject, &sendit) != -1)
	  if (sendit)
	    reply_to_mail(person, from_whom, subject);

	fclose(mailfile);
	return;
}

int
get_return(file, person, from, subject, sendit)
FILE *file;
int  person, *sendit;
char *from, *subject;
{
	/** Reads the new message and return the from and subject lines.
	    sendit is set to true iff it isn't a machine generated msg

	    If USE_EMBEDDED_ADDRESSES is set, then the return address
	    is taken as the last From: or Reply-To: that is seen in
	    the header lines.  Crude, but it works.
	**/
	
    char name1[SLEN], name2[SLEN], lastname[SLEN];
    char buffer[SLEN], hold_return[SLEN];
    int done = 0, in_header = 0;

    from[0] = '\0';
    name1[0] = '\0';
    name2[0] = '\0';
    lastname[0] = '\0';
    *sendit = 1;

    while (! done) {

      if (fgets(buffer, SLEN, file) == NULL)
	return(-1);

#ifndef USE_EMBEDDED_ADDRESSES
      if (first_word(buffer, "From ")) {
	in_header++;
	sscanf(buffer, "%*s %s", hold_return);
      }
      else if (in_header) {
        if (first_word_nc(buffer, ">From")) {
	  sscanf(buffer,"%*s %s %*s %*s %*s %*s %*s %*s %*s %s", name1, name2);
	  add_site(from, name2, lastname);
        }
#else /* We're allowing USE_EMBEDDED_ADDRESSES */
      if (first_word(buffer, "From ")) 
	in_header++;
      else if (in_header) {
	if (header_cmp(buffer, "From", NULL) ||
	    header_cmp(buffer, "Reply-To", NULL))
	  get_address_from(buffer, from);
#endif
        else if (header_cmp(buffer,"Subject", NULL)) {
	  remove_return(buffer);
	  strcpy(subject, (char *) (buffer + 8));
        }
        else if (header_cmp(buffer,"X-Mailer", "fastmail"))
	  *sendit = 0;
        else if (strlen(buffer) == 1)
	  done = 1;
      }
    }

#ifndef USE_EMBEDDED_ADDRESSES
    if (from[0] == '\0')
      strcpy(from, hold_return); /* default address! */
    else
      add_site(from, name1, lastname);	/* get the user name too! */
#endif

    return(0);
}

reply_to_mail(person, from, subject)
int   person;
char *from, *subject;
{
	/** Respond to the message from the specified person with the
	    specified subject... **/
	
	char buffer[SLEN];

	if (strlen(subject) == 0)
	  strcpy(subject, def_subj);
	else if (! first_word_nc(subject, subj_fw)) {
	  sprintf(buffer, arep_subj, subject);
	  strcpy(subject, buffer);
	}

	log_msg(catgets(elm_msg_cat, ArepdaemSet, ArepdaemArepTo,
	  "auto-replying to '%s'"), from);

	mail(from, subject, reply_table[person].replyfile, person);
}	

long
bytes(name)
char *name;
{
	/** return the number of bytes in the specified file.  This
	    is to check to see if new mail has arrived....  **/

	int ok = 1, err;
	struct stat buffer;

	if (stat(name, &buffer) != 0)
	  if (errno != 2) {
	   err = errno;
	   unlock();
	   exit(MCfprintf(stderr, catgets(elm_msg_cat, ArepdaemSet,
	     ArepdaemErrFstat, "Error %d attempting fstat on %s"),
	       err, name));
	  }
	  else
	    ok = 0;
	
	return(ok ? buffer.st_size : 0);
}

time_t
ModTime(name)
char *name;
{
	/** return the modification time in the specified file.
	    This is to check to see if autoreply has changed....  **/

	int ok = 1, err;
	struct stat buffer;

	if (stat(name, &buffer) != 0)
	  if (errno != 2) {
	   err = errno;
	   unlock();
	   exit(MCfprintf(stderr, catgets(elm_msg_cat, ArepdaemSet,
	     ArepdaemErrFstat, "Error %d attempting fstat on %s"),
	       err, name));
	  }
	  else
	    ok = 0;

	return(ok ? buffer.st_mtime : (time_t) 0);
}

mail(to, subject, filename, person)
char *to, *subject, *filename;
int   person;
{
	/** Mail 'file' to the user from person... **/
	
	char buffer[VERY_LONG_STRING];

	MCsprintf(buffer, catgets(elm_msg_cat, ArepdaemSet, ArepdaemMailCommand,
	  "%s/fastmail -f '%s [autoreply]' -s '%s' %s %s"),
		BIN, reply_table[person].username,
	        subject, filename, to);
	
	system(buffer);
}

log_msg(message, arg)
char *message;
char *arg;
{
	/** Put log entry into log file.  Use the format:
	      date-time: <message>
	**/

	struct tm *thetime;
	time_t	  clock;
#ifndef	_POSIX_SOURCE
	struct tm *localtime(); 
	time_t     time();
#endif
	char      buffer[SLEN];

	/** first off, get the time and date **/

	clock = time((time_t *) 0);       /* seconds since ???   */
	thetime = localtime(&clock);	/* and NOW the time... */

	/** then put the message out! **/

	sprintf(buffer, message, arg);

	fprintf(logfd,"%d/%d-%d:%02d: %s\n", 
		thetime->tm_mon+1, thetime->tm_mday,
	        thetime->tm_hour,  thetime->tm_min,
	        buffer);
}

FILE *open_logfile()
{
	/** open the logfile.  returns a valid file descriptor **/

	FILE *fd;

	if ((fd = fopen(logfile, "a")) == NULL)
	  if ((fd = fopen(logfile2, "a")) == NULL) {
	    unlock();
	    exit(1);	/* give up! */
	  }

	return( (FILE *) fd);
}

close_logfile()
{
	/** Close the logfile until needed again. **/

	fclose(logfd);
}

/*** LOCK and UNLOCK - ensure only one copy of this daemon running at any
     given time by using a file existance semaphore (wonderful stuff!) ***/

lock()
{
	char lock_name[SLEN];		/* name of lock file  */
	char pid_buffer[SHORT];
	int pid, create_fd, err;

	sprintf(lock_name, "%s/%s", LOCK_DIR, arep_lock_file);
#ifdef PIDCHECK
      /** first, try to read the lock file, and if possible, check the pid.
	  If we can validate that the pid is no longer active, then remove
	  the lock file.
       **/
	if((create_fd=open(lock_name,O_RDONLY)) != -1) {
	  if (read(create_fd, pid_buffer, SHORT) > 0) {
	    pid = atoi(pid_buffer);
	    if (pid) {
	      if (kill(pid, 0)) {
	        close(create_fd);
	        if (unlink(lock_name) != 0) {
		    err = errno;
		    MCprintf(catgets(elm_msg_cat, ArepdaemSet,
		    ArepdaemErrUnlink,
		    "Error %s\n\ttrying to unlink file %s (%s)\n"), 
		    error_description(err), lock_name, "lock");
		    return(0);
	        }
	      } else /* kill pid check succeeded */
	        return(0);
	    } else /* pid was zero */
	      return(0);
	  } else /* read failed */
	    return(0);
	}
	/* ok, either the open failed or we unlinked it, now recreate it. */
#else
	if (access(lock_name, EXISTS) == 0)
	  return(0);	/* file already exists */
#endif

	if ((create_fd=creat(lock_name, 0444)) == -1)
	  return(0);	/* can't create file!!   */

	sprintf(pid_buffer,"%d\n", getpid() );		/* write the current pid to the file */
	write(create_fd, pid_buffer, strlen(pid_buffer));
	close(create_fd);				/* no need to keep it open */
	
	return(1);

}

unlock()
{
	/** remove lock file if it's there! **/

	(void) unlink(arep_lock_file);
}

SIGHAND_TYPE	term_signal()
{
	unlock();
	exit(1);	/* give up! */
}
