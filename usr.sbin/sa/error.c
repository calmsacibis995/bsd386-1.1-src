/*
 * $Header: /bsdi/MASTER/BSDI_OS/usr.sbin/sa/error.c,v 1.2 1993/03/03 19:40:22 sanders Exp $
 * $Log: error.c,v $
 * Revision 1.2  1993/03/03  19:40:22  sanders
 * fixes to copy accounting file and not zero
 *
 * Revision 1.1.1.1  1992/09/25  19:11:41  trent
 * Initial import of sa from andy@terasil.terasil.com (Andrew H. Marrinson)
 *
 * Revision 1.3  1992/12/28  20:04:03  andy
 * No longer zeroes accounting file when creating summaries.
 *
 * Revision 1.2  1992/05/12  18:02:35  andy
 * Changed RCS ids.
 *
 */
#ifdef DEBUG
#include <signal.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sa.h"


void
done (int status)

{
#ifdef DEBUG
  sigset_t all_signals;

  sigemptyset (&all_signals);
  unblock_signals (&all_signals);
#endif
  if (user_temp_name[0] != 0)
    (void) unlink (user_temp_name);
  if (command_temp_name[0] != 0)
    (void) unlink (command_temp_name);
#ifdef DEBUG
  abort ();
#else
  exit (status);
#endif
}

void
terminate (void)

{
  done (1);
}

void
error (const char *irritant, const char *message)

{
  if (message)
    {
      if (irritant)
	fprintf (stderr, "sa: %s: %s\n", irritant, message);
      else
	fprintf (stderr, "sa: %s\n", message);
    }
  else
    {
      fprintf (stderr, "sa: ");
      perror (irritant);
    }
}

void
fatal_error (const char *irritant, const char *message, const int value)

{
  error (irritant, message);
  done (value);
}
