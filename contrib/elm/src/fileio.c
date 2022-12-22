
static char rcsid[] = "@(#)Id: fileio.c,v 5.14 1993/09/27 01:51:38 syd Exp ";

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
 * $Log: fileio.c,v $
 * Revision 1.2  1994/01/14  00:54:56  cp
 * 2.4.23
 *
 * Revision 5.14  1993/09/27  01:51:38  syd
 * Add elm_chown to consolidate for Xenix not allowing -1
 * From: Syd
 *
 * Revision 5.13  1993/09/19  23:15:52  syd
 * Here's some more patch stuff for undersize buffers for header lines.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.12  1993/08/23  12:28:23  syd
 * Fix placement of ifdef for PC_CHOWN
 * From: syd
 *
 * Revision 5.11  1993/08/23  03:26:24  syd
 * Try setting group id separate from user id in chown to
 * allow restricted systems to change group id of file
 * From: Syd
 *
 * Revision 5.10  1993/08/10  20:29:52  syd
 * add PC_CHOWN_RESTRICTED where needed
 * From: Syd
 *
 * Revision 5.9  1993/08/03  19:28:39  syd
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
 * Revision 5.8  1993/02/08  18:38:12  syd
 * Fix to copy_file to ignore unescaped from if content_length not yet reached.
 * Fixes to NLS messages match number of newlines between default messages
 * and NLS messages. Also an extra ) was removed.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.7  1993/01/20  03:02:19  syd
 * Move string declarations to defs.h
 * From: Syd
 *
 * Revision 5.6  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.5  1992/12/07  04:23:23  syd
 * fix typo
 * From: Syd
 *
 * Revision 5.4  1992/11/26  01:46:26  syd
 * add Decode option to copy_message, convert copy_message to
 * use bit or for options.
 * From: Syd and bjoerns@stud.cs.uit.no (Bjoern Stabell)
 *
 * Revision 5.3  1992/11/26  00:48:34  syd
 * Make it do raw(off) before final error message to
 * display error message on proper screen
 * From: Syd
 *
 * Revision 5.2  1992/11/07  20:05:52  syd
 * change to use header_cmp to allow for linear white space around the colon
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** File I/O routines, including deletion from the folder! 

**/

#include "headers.h"
#include "s_elm.h"
#include <sys/stat.h>
#include <errno.h>

extern int errno;

char *error_description();

static void copy_write_error_exit()
{
	MoveCursor(LINES, 0);
	Raw(OFF);
	printf(catgets(elm_msg_cat, ElmSet, ElmWriteCopyMessageFailed,
		"\nWrite in copy_message failed\n"));
	dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
	rm_temps_exit();
}

copy_message(prefix, 
	     dest_file, 
	     cm_options)
char *prefix;
FILE *dest_file;
int cm_options;
{
	/** Copy current message to destination file, with optional 'prefix' 
	    as the prefix for each line.  If remove_header is true, it will 
	    skip lines in the message until it finds the end of header line...
	    then it will start copying into the file... If remote is true
	    then it will append "remote from <hostname>" at the end of the
	    very first line of the file (for remailing) 

	    If "forwarding" is true then it'll do some nice things to
	    ensure that the forwarded message looks pleasant; e.g. remove
	    stuff like ">From " lines and "Received:" lines.

	    If "update_status" is true then it will write a new Status:
	    line at the end of the headers.  It never copies an existing one.

	    If "decode" is true, prompt for key if the message is encrypted,
	    else just copy it as it is.
	**/

    /*
     *	Changed buffer[SLEN] to buffer[VERY_LONG_STRING] to make it
     *	big enough to contain a full length header line. Any header
     *	is allowed to be at least 1024 bytes in length. (r.t.f. RFC)
     *	14-Sep-1993 Jukka Ukkonen <ukkonen@csc.fi>
     */

    char buffer[VERY_LONG_STRING];
    register struct header_rec *current_header = headers[current-1];
    register int  lines, front_line, next_front,
		  in_header = 1, first_line = TRUE, ignoring = FALSE;
    int remove_header = cm_options & CM_REMOVE_HEADER;
    int remote = cm_options & CM_REMOTE;
    int update_status = cm_options & CM_UPDATE_STATUS;
    int remail = cm_options & CM_REMAIL;
    int decode = cm_options & CM_DECODE;
    int	end_header = 0;
    int sender_added = 0;
    int crypted = 0;
    int bytes_seen = 0;
    int buf_len, err;

      /** get to the first line of the message desired **/

    if (fseek(mailfile, current_header->offset, 0) == -1) {
       dprint(1, (debugfile, 
		"ERROR: Attempt to seek %d bytes into file failed (%s)",
		current_header->offset, "copy_message"));
       error1(catgets(elm_msg_cat, ElmSet, ElmSeekFailed,
	     "ELM [seek] failed trying to read %d bytes into file."),
	     current_header->offset);
       return;
    }

    /* how many lines in message? */

    lines = current_header->lines;

    /* set up for forwarding just in case... */

    if (forwarding)
      remove_header = FALSE;

    if (current_header->encrypted && decode) {
	getkey(OFF);
    }

    /* now while not EOF & still in message... copy it! */

    next_front = TRUE;

    while (lines) {
      if (! (buf_len = mail_gets(buffer, VERY_LONG_STRING, mailfile)))
        break;

      bytes_seen += buf_len;
      front_line = next_front;

      if(buffer[buf_len - 1] == '\n') {
	lines--;	/* got a full line */
	next_front = TRUE;
      }
      else
	next_front = FALSE;
      
      if (front_line && ignoring)
	ignoring = whitespace(buffer[0]);

      if (ignoring)
	continue;

#ifdef MMDF
      if ((cm_options & CM_MMDF_HEAD) && strcmp(buffer, MSG_SEPARATOR) == 0)
	continue;
#endif /* MMDF */

      /* are we still in the header? */

      if (in_header && front_line) {
	if (buf_len < 2) {
	  in_header = 0;
	  end_header = -1;
	  if (remail && !sender_added) {
	    if (fprintf(dest_file, "%sSender: %s\n", prefix, username) == EOF) {
	      copy_write_error_exit();
	    }
	  }
	}
	else if (!isspace(*buffer)
	      && index(buffer, ':') == NULL
#ifdef MMDF
	      && strcmp(buffer, MSG_SEPARATOR) != 0
#endif /* MMDF */
		) {
	  in_header = 0;
	  end_header = 1;
	  if (remail && !sender_added) {
	    if (fprintf(dest_file, "%sSender: %s\n", prefix, username) == EOF) {
	      copy_write_error_exit();
	    }
	  }
	} else if (in_header && remote && header_cmp(buffer, "Sender", NULL)) {
	  if (remail)
	    if (fprintf(dest_file, "%sSender: %s\n", prefix, username) == EOF) {
	      copy_write_error_exit();
	    }
	  sender_added = TRUE;
	  continue;
	}
	if (end_header) {
	  if (update_status) {
	      if (isoff(current_header->status, NEW)) {
		if (ison(current_header->status, UNREAD)) {
		  if (fprintf(dest_file, "%sStatus: O\n", prefix) == EOF) {
		    copy_write_error_exit();
		  }
		} else	/* read */
#ifdef BSD
		  if (fprintf(dest_file, "%sStatus: OR\n", prefix) == EOF) {
#else
		  if (fprintf(dest_file, "%sStatus: RO\n", prefix) == EOF) {
#endif
		    copy_write_error_exit();
		  }
		update_status = FALSE; /* do it only once */
	      }	/* else if NEW - indicate NEW with no Status: line. This is
		 * important if we resync a mailfile - we don't want
		 * NEW status lost when we copy each message out.
		 * It is the responsibility of the function that calls
		 * this function to unset NEW as appropriate to its
		 * reason for using this function to copy a message
		 */

		/*
		 * add the missing newline for RFC 822
		 */
	      if (end_header > 0) {
		/* add the missing newline for RFC 822 */
		if (fprintf(dest_file, "\n") == EOF) {
		  copy_write_error_exit();
		}
	      }
	  }
	}
      }

      if (in_header) {
	/* Process checks while in header area */

	if (remove_header) {
	  ignoring = TRUE;
	  continue;
	}

	/* add remote on to front? */
	if (first_line && remote) {
	  no_ret(buffer);
#ifndef MMDF
	  if (fprintf(dest_file, "%s%s remote from %s\n",
		  prefix, buffer, hostname) == EOF) {
		copy_write_error_exit();
	  }
#else
	  if (first_word(buffer, "From ")) {
	    if (fprintf(dest_file, "%s%s remote from %s\n",
		    prefix, buffer, hostname) == EOF) {
		copy_write_error_exit();
	    }
	  } else {
	    if (fprintf(dest_file, "%s%s\n", prefix, buffer) == EOF) {
		copy_write_error_exit();
	    }
	  }
#endif /* MMDF */
	  first_line = FALSE;
	  continue;
	}

	if (!forwarding) {
	  if(! header_cmp(buffer, "Status", NULL)) {
	    if (fprintf(dest_file, "%s%s", prefix, buffer) == EOF) {
	      copy_write_error_exit();
	      }
	    continue;
	  } else {
	    ignoring = TRUE;
	    continue;	/* we will output a new Status: line later, if desired. */
	  }
	}
	else { /* forwarding */

	  if (header_cmp(buffer, "Received", NULL   ) ||
	      first_word_nc(buffer, ">From"         ) ||
	      header_cmp(buffer, "Status", NULL     ) ||
	      header_cmp(buffer, "Return-Path", NULL))
	      ignoring = TRUE;
	  else
	    if (remail && header_cmp(buffer, "To", NULL)) {
	      if (fprintf(dest_file, "%sOrig-%s", prefix, buffer) == EOF) {
		copy_write_error_exit();
	      }
	    } else {
	      if (fprintf(dest_file, "%s%s", prefix, buffer) == EOF) {
		copy_write_error_exit();
	      }
	    }
	}
      }
      else { /* not in header */
        /* Process checks that occur after the header area */

	/* perform encryption checks */
	if (buffer[0] == '[' && decode) {
		if (!strncmp(buffer, START_ENCODE, strlen(START_ENCODE))) {
			crypted = ON;
		} else if (!strncmp(buffer, END_ENCODE, strlen(END_ENCODE))) {
			crypted = OFF;
		} else if (crypted) {
			no_ret(buffer);
			encode(buffer);
			strcat(buffer, "\n");
		}
	} else if (crypted) {
		no_ret(buffer);
		encode(buffer);
		strcat(buffer, "\n");
	}


#ifndef MMDF
#ifndef DONT_ESCAPE_MESSAGES
	if(first_word(buffer, "From ") && (real_from(buffer, NULL)) &&
	   current_header->content_length <= bytes_seen) {
	  /* If we have a content-length > bytes_seen and there is lines left
	  ** this is probably a From line that is part of the body, as an
	  ** included message. A simple heuristic test.
	  */
	  dprint(1, (debugfile,
		 "\n*** Internal Problem...Tried to add the following;\n"));
	  dprint(1, (debugfile,
		 "  '%s'\nto output file (copy_message) ***\n", buffer));
	  break;	/* STOP NOW! */
	}
#endif /* DONT_ESCAPE_MESSAGES */
#endif /* MMDF */

	err = fprintf(dest_file, "%s", prefix);
	if (err != EOF)
	  err = fwrite(buffer, 1, buf_len, dest_file);
	if (err != buf_len) {
	  copy_write_error_exit();
	}
      }
    }
#ifndef MMDF
    if (buf_len + strlen(prefix) > 1)
	if (fprintf(dest_file, "\n") == EOF) {	/* blank line to keep mailx happy *sigh* */
	  copy_write_error_exit();
	}
#endif /* MMDF */
}

static struct stat saved_buf;

/*
 *  Don't take chances that a file name is really longer than SLEN.
 *  You'll just pollute the memory right after the allocated space
 *  if you have MAXPATHLEN of 1024 (_PATH_MAX in POSIX).
 */

static char saved_fname[VERY_LONG_STRING];

int
save_file_stats(fname)
char *fname;
{
	/* if fname exists, save the owner, group, mode and filename.
	 * otherwise flag nothing saved. Return 0 if saved, else -1.
	 */

	if(stat(fname, &saved_buf) != -1) {
	  (void)strcpy(saved_fname, fname);
	  dprint(2, (debugfile,
	    "** saved stats for file owner = %d group = %d mode = %o %s **\n",
	    saved_buf.st_uid, saved_buf.st_gid, saved_buf.st_mode, fname));
	  return(0);
	}
	dprint(2, (debugfile,
	  "** couldn't save stats for file %s [errno=%d] **\n",
	  fname, errno));
	return(-1);

}

restore_file_stats(fname)
char *fname;
{
	/* if fname matches the saved file name, set the owner and group
	 * of fname to the saved owner, group and mode,
	 * else to the userid and groupid of the user and to 700.
	 * Return	-1 if the  either mode or owner/group not set
	 *		0 if the default values were used
	 *		1 if the saved values were used
	 */

	int old_umask, i, new_mode, new_owner, new_group, ret_code;


	new_mode = 0600;
	new_owner = userid;
	new_group = groupid;
	ret_code = 0;

	if(strcmp(fname, saved_fname) == 0) {
	  new_mode = saved_buf.st_mode;
	  new_owner = saved_buf.st_uid;
	  new_group = saved_buf.st_gid;
	  ret_code = 1;
	}
	dprint(2, (debugfile, "** %s file stats for %s **\n",
	  (ret_code ? "restoring" : "setting"), fname));

	old_umask = umask(0);
	if((i = chmod(fname, new_mode & 0777)) == -1)
	  ret_code = -1;

	dprint(2, (debugfile, "** chmod(%s, %.3o) returns %d [errno=%d] **\n",
		fname, new_mode & 0777, i, errno));

	(void) umask(old_umask);

#ifdef	BSD
	/*
	 * Chown is restricted to root on BSD unix
	 */
	(void) elm_chown(fname, new_owner, new_group);
#else
#  ifdef _PC_CHOWN_RESTRICTED
/*
 * Chown may or may not be restricted to root in SVR4, if it is,
 *	then need to copy must be true, and no restore of permissions
 *	should be performed.
 */
        if (!pathconf(fname, _PC_CHOWN_RESTRICTED)) {
#  endif
	    if((i = elm_chown(fname, new_owner, new_group)) == -1)
		ret_code = -1;

	    dprint(2, (debugfile, "** elm_chown(%s, %d, %d) returns %d [errno=%d] **\n",
		       fname, new_owner, new_group, i, errno));
#  ifdef _PC_CHOWN_RESTRICTED
	} else {
	    (void) elm_chown(fname, new_owner, new_group);
	}
#  endif /* _PC_CHOWN_RESTRICTED */
#endif /* BSD */

	return(ret_code);

}

/** and finally, here's something for that evil trick: site hiding **/

#ifdef SITE_HIDING

int
is_a_hidden_user(specific_username)
char *specific_username;
{
	/** Returns true iff the username is present in the list of
	   'hidden users' on the system.
	**/
	
    FILE *hidden_users;
    char  buffer[VERY_LONG_STRING];

    /* 
	this line is deliberately inserted to ensure that you THINK
	about what you're doing, and perhaps even contact the author
	of Elm before you USE this option...
     */

	if ((hidden_users = fopen (HIDDEN_SITE_USERS,"r")) == NULL) {
	  dprint(1, (debugfile,
		  "Couldn't open hidden site file %s [%s]\n",
		  HIDDEN_SITE_USERS, error_description(errno)));
	  return(FALSE);
	}

	while (fscanf(hidden_users, "%s", buffer) != EOF)
	  if (strcmp(buffer, specific_username) == 0) {
	    dprint(3, (debugfile, "** Found user '%s' in hidden site file!\n",
		    specific_username));
	    fclose(hidden_users);
	    return(TRUE);
	  }

	fclose(hidden_users);
	dprint(3, (debugfile, 
		"** Couldn't find user '%s' in hidden site file!\n",
		specific_username));

	return(FALSE);
}

#endif
