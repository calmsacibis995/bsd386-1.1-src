/* Open a pipe to write to a program without intermediary sh.
   Copyright (C) 1992 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Written by David MacKenzie.  */

#include <stdio.h>
#include <varargs.h>

#if defined (HAVE_UNISTD_H)
#include <unistd.h>
#endif

/* Open a pipe to write to a program without intermediary sh.
   Checks PATH.
   Sample use:
   stream = writepipe ("progname", "arg1", "arg2", (char *) 0);
   Return 0 on error. */

/* VARARGS */
FILE *
writepipe (va_alist)
  va_dcl
{
  int fds[2];
  va_list ap;
  char *args[100];
  int argno = 0;

  /* Copy arguments into `args'. */
  va_start (ap);
  while ((args[argno++] = va_arg (ap, char *)) != NULL)
    /* Do nothing. */ ;
  va_end (ap);

  if (pipe (fds) == -1)
    return 0;

  switch (fork ())
    {
    case 0:			/* Child.  Read from pipe. */
      close (fds[1]);		/* Not needed. */
      if (fds[0] != 0)		/* Redirect 0 (stdin) only if needed.  */
	{
	  close (0);		/* We don't want the old stdin. */
	  dup (fds[0]);		/* Guaranteed to dup to 0 (stdin). */
	  close (fds[0]);	/* No longer needed. */
	}
      execvp (args[0], args);
      _exit (2);		/* 2 for `cmp'. */
    case -1:			/* Error. */
      return 0;
    default:			/* Parent.  Write to pipe. */
      close (fds[0]);		/* Not needed. */
      return fdopen (fds[1], "w");
    }
  /* NOTREACHED */
}
