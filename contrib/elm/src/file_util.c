
static char rcsid[] = "@(#)Id: file_util.c,v 5.10 1993/09/27 01:51:38 syd Exp ";

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
 * $Log: file_util.c,v $
 * Revision 1.2  1994/01/14  00:54:55  cp
 * 2.4.23
 *
 * Revision 5.10  1993/09/27  01:51:38  syd
 * Add elm_chown to consolidate for Xenix not allowing -1
 * From: Syd
 *
 * Revision 5.9  1993/09/19  23:37:29  syd
 * I found a few places more where the code was missing a call
 * to fflush() before it called unlock() and fclose()/exit()
 * right after unlocking the mail drop.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.8  1993/08/23  03:26:24  syd
 * Try setting group id separate from user id in chown to
 * allow restricted systems to change group id of file
 * From: Syd
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
 * Revision 5.6  1993/02/03  19:06:31  syd
 * Remove extra strchr/strcat/strcpy et al declarations
 * From: Syd
 *
 * Revision 5.5  1993/01/20  03:37:16  syd
 * Nits and typos in the NLS messages and corresponding default messages.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.4  1992/12/16  14:30:52  syd
 * Make forward to checking consistent on sign ignoring
 * From: Syd
 *
 * Revision 5.3  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.2  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** File oriented utility routines for ELM 

**/

#include "headers.h"
#include "s_elm.h"
#include <sys/stat.h>
#include <errno.h>

#ifdef BSD
# include <sys/wait.h>
#endif

extern int errno;		/* system error number */

char *error_description(), *getlogin();
long  fsize();

long
bytes(name)
char *name;
{
	int err;
	/** return the number of bytes in the specified file.  This
	    is to check to see if new mail has arrived....  (also
	    see "fsize()" to see how we can get the same information
	    on an opened file descriptor slightly more quickly)
	**/

	int ok = 1;
	struct stat buffer;

	if (stat(name, &buffer) != 0)
	  if (errno != 2) {
	    err = errno;
	    MoveCursor(LINES, 0);
	    Raw(OFF);
	    dprint(1,(debugfile,
		     "Error: errno %s on fstat of file %s (bytes)\n", 
		     error_description(err), name));
	    printf(catgets(elm_msg_cat, ElmSet, ElmErrorFstat,
		     "\nError attempting fstat on file %s!\n"),
		     name);
	    printf("** %s. **\n", error_description(err));
	    emergency_exit();
	  }
	  else
	    ok = 0;
	
	return(ok ? (long) buffer.st_size : 0L);
}

int
copy(from, to)
char *from, *to;
{
	/** this routine copies a specified file to the destination
	    specified.  Non-zero return code indicates that something
	    dreadful happened! **/

	FILE *from_file, *to_file;
	char buffer[VERY_LONG_STRING];
	int  len;

	dprint (1, (debugfile, "Copy: from='%s' to='%s'\n", from, to));

	if ((from_file = fopen(from, "r")) == NULL) {
	    dprint(1, (debugfile, "Error: could not open %s for reading (copy)\n",
		       from));
	    error1(catgets(elm_msg_cat, ElmSet, ElmCouldNotOpenFile,
			   "Could not open file %s."), from);
	    return(1);
	}

	if ((to_file = fopen(to, "w")) == NULL) {
	    dprint(1, (debugfile, "Error: could not open %s for writing (copy)\n",
		       to));
	    error1(catgets(elm_msg_cat, ElmSet, ElmCouldNotOpenFile,
			   "Could not open file %s."), to);
	    return(1);
	}

	while (len = fread(buffer, 1, VERY_LONG_STRING, from_file)) {
	    if (fwrite(buffer, 1, len, to_file) != len) {
		Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmWriteFailedCopy,
			"\n\rWrite failed to temp file in copy\n\r"), 0);
		perror(to);
		/*
		 *  NEVER close anything just at whim!!
		 *  If the file has been locked using fcntl() or lockf()
		 *	YOU WILL DROP ALL LOCKS refering to the file.
		 */
		fflush(to_file);
		fclose(to_file);
		fclose(from_file);
		return(1);
	    }
	}

	fclose(from_file);
	fflush(to_file);

	if (fclose(to_file) == EOF) {
	    Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmCloseFailedCopy,
		    "\n\rClose failed on temp file in copy\n\r"), 0);
	    perror(to);
	    return(1);
	}
	(void) elm_chown( to, userid, groupid);

	return(0);
}

int
append(fd, filename, prefix_str)
FILE *fd;
char *filename;
char *prefix_str;
{
	/** This routine appends the specified file to the already
	    open file descriptor.. Returns non-zero if fails.  **/

	FILE *my_fd;
	char buffer[VERY_LONG_STRING];
	int  len;
	
	if ((my_fd = fopen(filename, "r")) == NULL) {
	  dprint(1, (debugfile,
		"Error: could not open %s for reading (append)\n", filename));
	  return(1);
	}

	if (prefix_str != NULL && fputs(prefix_str, fd) == EOF) {
	  MoveCursor(LINES, 0);
	  Raw(OFF);
	  printf(catgets(elm_msg_cat, ElmSet, ElmWriteFailedAppend,
		"\nWrite failed to temp file in append\n"));
	  perror(filename);
	  rm_temps_exit();
	}

	while (len = fread(buffer, 1, VERY_LONG_STRING, my_fd))
	  if (fwrite(buffer, 1, len, fd) != len) {
	      MoveCursor(LINES, 0);
	      Raw(OFF);
	      printf(catgets(elm_msg_cat, ElmSet, ElmWriteFailedAppend,
		      "\nWrite failed to temp file in append\n"));
	      perror(filename);
	      rm_temps_exit();
	  }

	if (fclose(my_fd) == EOF) {
	  MoveCursor(LINES, 0);
	  Raw(OFF);
	  printf(catgets(elm_msg_cat, ElmSet, ElmCloseFailedAppend,
		  "\nClose failed on temp file in append\n"));
	  perror(filename);
	  rm_temps_exit();
	}

	return(0);
}

#define FORWARDSIGN	"Forward to "
int
check_mailfile_size(mfile)
char *mfile;
{
	/** Check to ensure we have mail.  Only used with the '-z'
	    starting option. So we output a diagnostic if there is
	    no mail to read (including  forwarding).
	    Return 0 if there is mail,
		   <0 if no permission to check,
		   1 if no mail,
		   2 if no mail because mail is being forwarded.
	 **/

	char firstline[SLEN];
	int retcode;
	struct stat statbuf;
	FILE *fp = NULL;

	/* see if file exists first */
	if (access(mfile, ACCESS_EXISTS) != 0)
	  retcode = 1;					/* no file */

	/* exists - now see if user has read access */
	else if (can_access(mfile, READ_ACCESS) != 0)
	  retcode = -1;					/* no perm */

	/* read access - now see if file has a reasonable size */
	else if ((fp = fopen(mfile, "r")) == NULL)
	  retcode = -1;		/* no perm? should have detected this above! */
	else if (fstat(fileno(fp), &statbuf) == -1) 
	  retcode = -1;					/* arg error! */
	else if (statbuf.st_size < 2)		
	  retcode = 1;	/* empty or virtually empty, e.g. just a newline */

	/* file has reasonable size - see if forwarding */
	else if (mail_gets (firstline, SLEN, fp) == 0)
	  retcode = 1;		 /* empty? should have detected this above! */
	else if (first_word_nc(firstline, FORWARDSIGN))
	  retcode = 2;					/* forwarding */

	/* not forwarding - so file must have some mail in it */
	else
	  retcode = 0;

	/* now display the appropriate message if there isn't mail in it */
	switch(retcode) {

	case -1:	printf(catgets(elm_msg_cat, ElmSet, ElmNoPermRead,
				"You have no permission to read %s!\n\r"),
				mfile);
			break;
	case 1:		printf(catgets(elm_msg_cat, ElmSet, ElmNoMail,
				"You have no mail.\n\r"));
			break;
	case 2:		no_ret(firstline) /* remove newline before using */
			printf(catgets(elm_msg_cat, ElmSet,ElmMailBeingForwarded,
				"Your mail is being forwarded to %s.\n\r"),
			  firstline + strlen(FORWARDSIGN));
			break;
	}
	if (fp)
	  fclose(fp);

	return(retcode);
}


long fsize(fd)
FILE *fd;
{
	/** return the size of the current file pointed to by the given
	    file descriptor - see "bytes()" for the same function with
	    filenames instead of open files...
	**/

	struct stat buffer;

	(void) fstat(fileno(fd), &buffer);

	return( (long) buffer.st_size );
}
