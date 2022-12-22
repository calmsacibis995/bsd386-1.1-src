/* error.c -- Functions for handling errors. */

#include <stdio.h>
#include <fcntl.h>

#if defined (HAVE_VFPRINTF)
#include <varargs.h>
#endif

#include <errno.h>
#if !defined (errno)
extern int errno;
#endif /* !errno */

#include "bashansi.h"
#include "flags.h"
#include "error.h"

extern char *shell_name;
extern char *base_pathname ();
extern char *strerror ();

/* Report an error having to do with FILENAME. */
void
file_error (filename)
     char *filename;
{
  report_error ("%s: %s", filename, strerror (errno));
}

#if !defined (HAVE_VFPRINTF)
void
programming_error (reason, arg1, arg2, arg3, arg4, arg5)
     char *reason;
{
  extern char *the_current_maintainer;

#if defined (JOB_CONTROL)
  {
    extern pid_t shell_pgrp;
    give_terminal_to (shell_pgrp);
  }
#endif /* JOB_CONTROL */

  report_error (reason, arg1, arg2);
  fprintf (stderr, "Tell %s to fix this someday.\n", the_current_maintainer);

#if defined (MAKE_BUG_REPORTS)
  if (1)
    {
      fprintf (stderr, "Mailing a bug report...");
      fflush (stderr);
      make_bug_report (reason, arg1, arg2, arg3, arg4, arg5);
      fprintf (stderr, "done.\n");
    }
#endif /* MAKE_BUG_REPORTS */

  fprintf (stderr, "Stopping myself...");
  fflush (stderr);
  abort ();
}

void
report_error (format, arg1, arg2, arg3, arg4, arg5)
     char *format;
{
  fprintf (stderr, "%s: ", base_pathname (shell_name));

  fprintf (stderr, format, arg1, arg2, arg3, arg4, arg5);
  fprintf (stderr, "\n");
  if (exit_immediately_on_error)
    exit (1);
}  

void
fatal_error (format, arg1, arg2, arg3, arg4, arg5)
     char *format;
{
  report_error (format, arg1, arg2, arg3, arg4, arg5);
  exit (2);
}

void
internal_error (format, arg1, arg2, arg3, arg4, arg5)
     char *format;
{
  fprintf (stderr, "%s: ", base_pathname (shell_name));

  fprintf (stderr, format, arg1, arg2, arg3, arg4, arg5);
  fprintf (stderr, "\n");
}

#else /* We have VARARGS support, so use it. */

void
programming_error (va_alist)
     va_dcl
{
  extern char *the_current_maintainer;
  va_list args;
  char *format;

#if defined (JOB_CONTROL)
  {
    extern pid_t shell_pgrp;
    give_terminal_to (shell_pgrp);
  }
#endif /* JOB_CONTROL */

  va_start (args);
  format = va_arg (args, char *);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");
  va_end (args);

  fprintf (stderr, "Tell %s to fix this someday.\n", the_current_maintainer);

#if defined (MAKE_BUG_REPORTS)
  if (1)
    {
      fprintf (stderr, "Mailing a bug report...");
      fflush (stderr);
      make_bug_report (va_alist);
      fprintf (stderr, "done.\n");
    }
#endif
  fprintf (stderr, "Stopping myself...");
  fflush (stderr);
  abort ();
}

void
report_error (va_alist)
     va_dcl
{
  va_list args;
  char *format;

  fprintf (stderr, "%s: ", base_pathname (shell_name));
  va_start (args);
  format = va_arg (args, char *);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
  if (exit_immediately_on_error)
    exit (1);
}

void
fatal_error (va_alist)
     va_dcl
{
  va_list args;
  char *format;

  fprintf (stderr, "%s: ", base_pathname (shell_name));
  va_start (args);
  format = va_arg (args, char *);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
  exit (2);
}

void
internal_error (va_alist)
     va_dcl
{
  va_list args;
  char *format;

  fprintf (stderr, "%s: ", base_pathname (shell_name));
  va_start (args);
  format = va_arg (args, char *);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);
}

itrace (va_alist)
     va_dcl
{
  va_list args;
  char *format;

  fprintf(stderr, "TRACE: pid %d: ", getpid());
  va_start (args);
  format = va_arg (args, char *);
  vfprintf (stderr, format, args);
  fprintf (stderr, "\n");

  va_end (args);

  fflush(stderr);
}

#if 0
/* A trace function for silent debugging -- doesn't require a control
   terminal. */
trace (va_alist)
     va_dcl
{
  va_list args;
  char *format;
  static FILE *tracefp = (FILE *)NULL;

  if (tracefp == NULL)
    tracefp = fopen("/usr/tmp/bash-trace.log", "a+");

  if (tracefp == NULL)
    tracefp = stderr;
  else
    fcntl (fileno (tracefp), F_SETFD, 1);     /* close-on-exec */

  va_start (args);
  format = va_arg (args, char *);
  vfprintf (tracefp, format, args);
  fprintf (tracefp, "\n");

  va_end (args);

  fflush(tracefp);
}
#endif
#endif /* HAVE_VFPRINTF */
