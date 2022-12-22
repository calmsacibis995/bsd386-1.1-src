/*
 * $Header: /bsdi/MASTER/BSDI_OS/usr.sbin/sa/io.c,v 1.2 1993/03/03 19:40:23 sanders Exp $
 * $Log: io.c,v $
 * Revision 1.2  1993/03/03  19:40:23  sanders
 * fixes to copy accounting file and not zero
 *
 * Revision 1.1.1.1  1992/09/25  19:11:41  trent
 * Initial import of sa from andy@terasil.terasil.com (Andrew H. Marrinson)
 *
 * Revision 1.7  1992/12/28  20:04:03  andy
 * No longer zeroes accounting file when creating summaries.
 *
 * Revision 1.6  1992/08/04  19:41:17  andy
 * Modified to specify O_TRUNC rather than O_EXCL.  This error
 * appeared when we switched to rename.
 *
 * Revision 1.5  1992/08/04  18:53:55  andy
 * Modified to use rename(2) instead of link/unlink.
 *
 * Revision 1.4  1992/08/04  18:40:08  andy
 * Eliminated places where divide by zero could occur.
 *
 * Revision 1.3  1992/06/03  01:35:42  andy
 * Modified reclassify function to respect -a flag when merging into
 * other_summary.
 *
 * Revision 1.2  1992/05/12  18:02:35  andy
 * Changed RCS ids.
 *
 */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/file.h>
#include "sa.h"
#include "pathnames.h"


char command_temp_name[PATH_MAX];
char user_temp_name[PATH_MAX];


struct acctcmd *
reclassify (const struct acctcmd *record)

{
  int i;
  int answer;

  if (use_other)
    {
      for (i = 0; i < 10; ++i)
	{
	  if (record->ac_comm[i] == 0)
	    break;
	  if (! isprint (record->ac_comm[i]))
	    return other_summary;
	}
      if (record->ac_count <= 1)
	return other_summary;
    }
  if (junk_threshold <= 0)
    return NULL;
  if (record->ac_count > junk_threshold)
    return NULL;
  if (! junk_interactive)
    return junk_summary;
  printf ("%.10s: ", record->ac_comm);
  fflush (stdout);
  answer = getchar ();
  for ( ; ; )
    {
      switch (getchar ())
	{
	case EOF:
	case '\n':
	  if (answer == 'y')
	    return junk_summary;
	  return NULL;
	}
    }
}

static void
common_header (void)

{
  if (separate_sys_and_user)
    printf ("    u         s     ");
  else
    printf ("   cpu    ");
  if (show_percent_time)
    printf (" pct   ");
  printf ("   re     ");
  if (show_real_over_cpu)
    printf (" re/cpu  ");
  printf ("   k    ");
  if (show_kcore_secs)
    printf ("   k*sec    ");
  if (show_total_disk)
    printf ("   tio     ");
  else
    printf ("   avio    ");
  printf ("  cnt\n");
}


void
show_user_header (void)

{
  printf ("   user    ");
  common_header ();
}

void
show_command_header (void)

{
  printf (" command   ");
  common_header ();
}

static void
common_entry (comp_t stime, comp_t utime, comp_t etime,
	      short mem, comp_t io, long count)

{
  double sys, user, real;
  double osys, ouser, oreal;

  osys = sys = (double) comp_conv (stime) / AHZ;
  ouser = user = (double) comp_conv (utime) / AHZ;
  oreal = real = (double) comp_conv (etime) / AHZ;
  if (use_average_times)
    {
      sys /= count;
      user /= count;
      real /= count;
    }
  else
    {
      sys /= 60;
      user /= 60;
      real /= 60;
    }
  if (separate_sys_and_user)
    printf ("%9.2f %9.2f ", user, sys);
  else
    printf ("%9.2f ", user + sys);
  if (show_percent_time)
    if (total_time != 0)
      printf ("%6.2f ", (ouser + osys) / (total_time * AHZ));
    else
      printf ("%6.2f ", 1.0);	/* cpu == total == 0 */
  printf ("%9.2f ", real);
  if (show_real_over_cpu)
    if (ouser + osys != 0)
      printf ("%8.2f ", oreal / (ouser + osys));
    else if (oreal == 0)
      printf ("%8.2f ", 1.0);
    else
      printf ("   inf   ");
  printf ("%7.1f ", (double) mem / AHZ);
  if (show_kcore_secs)
    printf ("%11.2f ", (ouser + osys) * mem / AHZ);
  if (show_total_disk)
    printf ("%10.1f ", (double) comp_conv (io) / AHZ);
  else				/* assume count != 0 */
    printf ("%10.1f ", (double) comp_conv (io) / (AHZ * count));
  printf ("%7ld\n", count);
}

int
show_user_entry (const void *record)

{
  const struct acctusr *r = record;
  struct passwd *pw = getpwuid (r->ac_uid);

  if (pw == NULL)
    printf ("%-10hd", r->ac_uid);
  else
    printf ("%-10.10s", pw->pw_name);
  common_entry (r->ac_stime, r->ac_utime, r->ac_etime,
		r->ac_mem, r->ac_io, r->ac_count);
  return TRUE;
}

int
show_command_entry (const void *record)

{
  const struct acctcmd *r = record;

  printf ("%-10.10s ", r->ac_comm);
  common_entry (r->ac_stime, r->ac_utime, r->ac_etime,
		r->ac_mem, r->ac_io, r->ac_count);
  return TRUE;
}

FILE *
make_temporary (const char c, char *name)

{
  int fd;
  sigset_t signals;

  block_signals (&signals);
  sprintf (name, "%ssa%cXXXXXX", _PATH_TMP, c);
  fd = mkstemp (name);
  if (fd != -1)
    {
      FILE *result = fdopen (fd, "w");
      int save_error = errno;

      if (result != NULL)
	{
	  unblock_signals (&signals);
	  return result;
	}
      (void) close (fd);
      (void) unlink (name);
      errno = save_error;
    }
  name[0] = 0;
  unblock_signals (&signals);
  fatal_error ("creating temporary file", NULL, EX_IOERR);
}


void
move (const char *source, const char *dest)

{
  if (access (source, 0) == -1)
    fatal_error (source, NULL, EX_IOERR);
  if (access (dest, 0) == 0 && access (dest, W_OK) == -1)
    fatal_error (dest, NULL, EX_IOERR);
  if (rename (source, dest) == 0)
    return;
  if (errno == EXDEV)
    {
      char buf[1024];
      int src, dst;

      src = open (source, O_RDONLY, 0);
      dst = open (dest, O_WRONLY|O_CREAT|O_TRUNC, 0666);
      for ( ; ; )
	{
	  int len = read (src, buf, sizeof (buf));
	  switch (len)
	    {
	    case -1:
	      fatal_error (source, NULL, EX_IOERR);
	    case 0:
	      (void) close (src);
	      (void) close (dst);
	      (void) unlink (source); /* error here means tmp file left */
	      return;
	    default:
	      if (write (dst, buf, len) == -1)
		fatal_error (dest, NULL, EX_IOERR);
	    }
	}
    }
  fatal_error (dest, NULL, EX_IOERR);
}

int
copy_and_truncate (const char *source, const char *dest)

{
  int save_error;
  int src = open (source, O_RDWR, 0);
  int dst = open (dest, O_WRONLY, 0);

  if (src == -1)
    goto error;
  if (dst == -1)
    goto error;
  for ( ; ; )
    {
      char buf[1024];
      int len = read (src, buf, sizeof (buf));

      switch (len)
	{
	case -1:
	  goto error;
	case 0:
	  if (ftruncate (src, 0) == -1)
	    goto error;
	  (void) close (src);
	  (void) close (dst);
	  return TRUE;
	default:
	  if (write (dst, buf, len) == -1)
	    {
	    error:
	      save_error = errno;
	      (void) close (src);
	      (void) close (dst);
	      errno = save_error;
	      return FALSE;
	    }
	}
    }
}
