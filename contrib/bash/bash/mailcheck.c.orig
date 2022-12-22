/* mailcheck.c -- The check is in the mail... */

/* Copyright (C) 1987,1989 Free Software Foundation, Inc.

This file is part of GNU Bash, the Bourne Again SHell.

Bash is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 1, or (at your option) any later
version.

Bash is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with Bash; see the file COPYING.  If not, write to the Free Software
Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. */

#include <stdio.h>
#include <sys/types.h>
#include "posixstat.h"
#include <sys/param.h>
#include "bashansi.h"
#include "shell.h"

#include "maxpath.h"

#ifndef NOW
#define NOW ((time_t)time ((time_t *)0))
#endif

extern struct user_info current_user;

extern char *extract_colon_unit ();
extern char *tilde_expand ();

typedef struct {
  char *name;
  time_t access_time;
  time_t mod_time;
  long file_size;
} FILEINFO;

/* The list of remembered mail files. */
FILEINFO **mailfiles = (FILEINFO **)NULL;

/* Number of mail files that we have. */
int mailfiles_count = 0;

/* The last known time that mail was checked. */
int last_time_mail_checked = 0;

/* Returns non-zero if it is time to check mail. */
time_to_check_mail ()
{
  char *temp = get_string_value ("MAILCHECK");
  time_t now = NOW;
  long seconds = -1;

  /* Skip leading whitespace in MAILCHECK. */
  if (temp)
    {
      while (whitespace (*temp))
	temp++;

      seconds = atoi (temp);
    }

  /* Negative number, or non-numbers (such as empty string) cause no checking
     to take place. */
  if (seconds < 0)
    return (0);

  /* Time to check if MAILCHECK is explicitly set to zero, or if enough
     time has passed since the last check. */
  if (!seconds || ((now - last_time_mail_checked) >= seconds))
    return (1);

  return (0);
}

/* Okay, we have checked the mail.  Perhaps I should make this function
   go away. */
reset_mail_timer ()
{
  last_time_mail_checked = NOW;
}

/* Locate a file in the list.  Return index of
   entry, or -1 if not found. */
find_mail_file (file)
     char *file;
{
  register int index = 0;

  while (index < mailfiles_count)
    if (strcmp ((mailfiles[index])->name, file) == 0) return index;
    else index++;
  return (-1);
}

/* Add this file to the list of remembered files. */
void
add_mail_file (file)
     char *file;
{
  struct stat finfo;
  char *filename = full_pathname (file);
  int index = find_mail_file (file);

  if (index > -1)
    {
      if (stat (filename, &finfo) == 0)
	{
	  mailfiles[index]->mod_time = finfo.st_mtime;
	  mailfiles[index]->access_time = finfo.st_atime;
	  mailfiles[index]->file_size = (long)finfo.st_size;
	}
      free (filename);
      return;
    }

  mailfiles = (FILEINFO **)
    xrealloc (mailfiles,
	      ((++mailfiles_count) * sizeof (FILEINFO *)));

  mailfiles[mailfiles_count - 1] = (FILEINFO *)xmalloc (sizeof (FILEINFO));
  mailfiles[mailfiles_count - 1]->name = filename;
  if (stat (filename, &finfo) == 0)
    {
      mailfiles[mailfiles_count - 1]->access_time = finfo.st_atime;
      mailfiles[mailfiles_count - 1]->mod_time = finfo.st_mtime;
      mailfiles[mailfiles_count - 1]->file_size = finfo.st_size;
    }
  else
    {
      mailfiles[mailfiles_count - 1]->access_time
	= mailfiles[mailfiles_count - 1]->mod_time = (time_t)-1;
      mailfiles[mailfiles_count - 1]->file_size = (long)-1;
    }
}

/* Reset the existing mail files access and modification times to zero. */
reset_mail_files ()
{
  register int i;

  for (i = 0; i < mailfiles_count; i++)
    {
      mailfiles[i]->access_time = mailfiles[i]->mod_time = 0;
      mailfiles[i]->file_size = (long)0;
    }
}

/* Free the information that we have about the remembered mail files. */
free_mail_files ()
{
  while (mailfiles_count--)
    {
      free (mailfiles[mailfiles_count]->name);
      free (mailfiles[mailfiles_count]);
    }

  if (mailfiles)
    free (mailfiles);

  mailfiles_count = 0;
  mailfiles = (FILEINFO **)NULL;
}

/* Return non-zero if FILE's mod date has changed. */
file_mod_date_changed (file)
     char *file;
{
  time_t time = (time_t)NULL;
  struct stat finfo;

  int index = find_mail_file (file);
  if (index != -1)
    time = mailfiles[index]->mod_time;

  if (stat (file, &finfo) == 0)
    {
      if (finfo.st_size != 0)
	return (time != finfo.st_mtime);
    }
  return (0);
}

/* Return non-zero if FILE's access date has changed. */
file_access_date_changed (file)
     char *file;
{
  time_t time = (time_t)NULL;
  struct stat finfo;

  int index = find_mail_file (file);
  if (index != -1)
    time = mailfiles[index]->access_time;

  if (stat (file, &finfo) == 0)
    {
      if (finfo.st_size != 0)
	return (time != finfo.st_atime);
    }
  return (0);
}

/* Return non-zero if FILE's size has increased. */
file_has_grown (file)
     char *file;
{
  long size = 0L;
  struct stat finfo;

  int i = find_mail_file (file);
  if (i != -1)
    size = mailfiles[i]->file_size;

  if (stat (file, &finfo) == 0)
    {
      return (finfo.st_size > size);
    }
  return (0);
}

#if defined (USG)
#define DEFAULT_MAIL_PATH "/usr/mail/"
#else
#define DEFAULT_MAIL_PATH "/usr/spool/mail/"
#endif

/* Return the colon separated list of pathnames to check for mail. */
char *
get_mailpaths ()
{
  char *mailpaths;

  mailpaths = get_string_value ("MAILPATH");

  if (!mailpaths)
    mailpaths = get_string_value ("MAIL");

  if (!mailpaths)
    {
      mailpaths = (char *)
	alloca (1 + strlen (DEFAULT_MAIL_PATH) + strlen (current_user.user_name));

      sprintf (mailpaths, "%s%s", DEFAULT_MAIL_PATH, current_user.user_name);
    }

  return (savestring (mailpaths));
}

/* Take an element from $MAILPATH and return the portion from
   the first unquoted `?' or `%' to the end of the string.  This is the
   message to be printed when the file contents change. */
static char *
parse_mailpath_spec (str)
     char *str;
{
  char *s;
  int pass_next;

  for (s = str, pass_next = 0; s && *s; s++)
    {
      if (pass_next)
	{
	  pass_next = 0;
	  continue;
	}
      if (*s == '\\')
	{
	  pass_next++;
	  continue;
	}
      if (*s == '?' || *s == '%')
        return s;
    }
  return ((char *)NULL);
}
      
/* Remember the dates of the files specified by MAILPATH, or if there is
   no MAILPATH, by the file specified in MAIL.  If neither exists, use a
   default value, which we randomly concoct from using Unix. */
remember_mail_dates ()
{
  char *mailpaths = get_mailpaths ();
  char *mailfile, *mp;
  int index = 0, pass_next = 0;
  
  while (mailfile = extract_colon_unit (mailpaths, &index))
    {
      mp = parse_mailpath_spec (mailfile);
      if (mp && *mp)
	*mp = '\0';
      add_mail_file (mailfile);
      free (mailfile);
    }
  free (mailpaths);
}

/* check_mail () is useful for more than just checking mail.  Since it has
   the paranoids dream ability of telling you when someone has read your
   mail, it can just as easily be used to tell you when someones .profile
   file has been read, thus letting one know when someone else has logged
   in.  Pretty good, huh? */

/* Check for mail in some files.  If the modification date of any
   of the files in MAILPATH has changed since we last did a
   remember_mail_dates () then mention that the user has mail.
   Special hack:  If the shell variable MAIL_WARNING is on and the
   mail file has been accessed since the last time we remembered, then
   the message "The mail in <mailfile> has been read" is printed. */
check_mail ()
{
  register int string_index;
  char *current_mail_file, *you_have_mail_message;
  char *mailpaths = get_mailpaths (), *mp;
  int index = 0;
  char *dollar_underscore;

  dollar_underscore = get_string_value ("_");

  if (dollar_underscore)
    dollar_underscore = savestring (dollar_underscore);

  while ((current_mail_file = extract_colon_unit (mailpaths, &index)))
    {
      char *t;
      int use_user_notification;

      if (!*current_mail_file)
	{
	  free (current_mail_file);
	  continue;
	}

      t = full_pathname (current_mail_file);
      free (current_mail_file);
      current_mail_file = t;

      use_user_notification = 0;
      you_have_mail_message = "You have mail in $_";

      mp = parse_mailpath_spec (current_mail_file);
      if (mp && *mp)
	{
	  *mp = '\0';
	  you_have_mail_message = ++mp;
	  use_user_notification++;
	}

      if (file_mod_date_changed (current_mail_file))
	{
	  WORD_LIST *tlist;
	  int i, file_is_bigger;
	  bind_variable ("_", current_mail_file);

	  /* Have to compute this before the call to add_mail_file, which
	     resets all the information. */
	  file_is_bigger = file_has_grown (current_mail_file);

	  add_mail_file (current_mail_file);

	  i = find_mail_file (current_mail_file);

	  /* If the user has just run a program which manipulates the
	     mail file, then don't bother explaining that the mail
	     file has been manipulated.  Since some systems don't change
	     the access time to be equal to the modification time when
	     the mail in the file is manipulated, check the size also.  If
	     the file has not grown, continue. */
	  if (i != -1 &&
	      (mailfiles[i]->access_time >= mailfiles[i]->mod_time) &&
	      !file_is_bigger)
	    goto next_mail_file;

#if 0
	  /* XXX - this test is useless, and so is the assignment.
		   have already gone to the next mail file if the test
		   succeeds, since it is the same as the one above, and
		   the assignment has already been made. */
	  if (!use_user_notification)
	    {
	      if (i != -1 &&
		  (mailfiles[i]->access_time < mailfiles[i]->mod_time) &&
		  file_is_bigger)
		you_have_mail_message = "You have new mail in $_";
	    }
#endif

	  if ((tlist = expand_string (you_have_mail_message, 1)))
	    {
	      char *tem = string_list (tlist);
	      you_have_mail_message = (char *)alloca (1 + strlen (tem));
	      strcpy (you_have_mail_message, tem);
	      free (tem);
	    }
	  else
	    you_have_mail_message = "";

	  printf ("%s\n", you_have_mail_message);
	  dispose_words (tlist);
	}

      if (find_variable ("MAIL_WARNING")
	  && file_access_date_changed (current_mail_file))
	{
	  add_mail_file (current_mail_file);
	  printf ("The mail in %s has been read!\n", current_mail_file);
	}
    next_mail_file: 
      free (current_mail_file);
    }
  free (mailpaths);

  if (dollar_underscore)
    {
      bind_variable ("_", dollar_underscore);
      free (dollar_underscore);
    }
  else
    unbind_variable ("_");
}
