/*
 * $Header: /bsdi/MASTER/BSDI_OS/usr.sbin/sa/summary.c,v 1.2 1993/03/03 19:40:28 sanders Exp $
 * $Log: summary.c,v $
 * Revision 1.2  1993/03/03  19:40:28  sanders
 * fixes to copy accounting file and not zero
 *
 * Revision 1.1.1.1  1992/09/25  19:11:42  trent
 * Initial import of sa from andy@terasil.terasil.com (Andrew H. Marrinson)
 *
 * Revision 1.7  1992/12/28  20:04:03  andy
 * No longer zeroes accounting file when creating summaries.
 *
 * Revision 1.6  1992/08/04  21:03:39  andy
 * Fixed up COMPATSUM to avoid compile-time error, and cleaned up
 * the COMPATSUM code a bit.
 *
 * Revision 1.5  1992/08/04  20:51:08  andy
 * No longer stores other and junk records in the summary files.
 * This allows them to be recalculated every time sa is run (so
 * that -a works as expected and commands run just once a day don't
 * wind up in other).  Also added code to reject entries with zero
 * counts that find their way into the summaries (paranoia).
 *
 * Revision 1.4  1992/08/04  19:03:27  andy
 * Fixed bug that stored reclassified commands in both the
 * summary record and individually.
 *
 * Revision 1.3  1992/08/04  18:40:08  andy
 * Eliminated places where divide by zero could occur.
 *
 * Revision 1.2  1992/05/12  18:02:35  andy
 * Changed RCS ids.
 *
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/stat.h>
#include "sa.h"


#define NENTRY 521
struct entry {
  long uid;
  char command[10];
  void *data;
  struct entry *next;
} *table[NENTRY];

double total_time = 0.0;
struct acctcmd *other_summary;
struct acctcmd *junk_summary;


static short
merge_memory (comp_t setime, comp_t retime, short sm, short rm)

{
  COMP_T se = comp_conv (setime);
  COMP_T re = comp_conv (retime);

  if (se + re != 0)
    {
      COMP_T answer = (sm*se + rm*re) / (se + re);

      if (answer >= SHRT_MAX)
	return SHRT_MAX;
      return answer;
    }
  return (sm + rm) / 2;
}


#define MOVE(F) s->F = record->F
#define ADD(F) s->F = comp_add (s->F, record->F)

static void
merge_user (void *summary, const struct acct *record)

{
  struct acctusr *s = summary;

  ADD (ac_utime);
  ADD (ac_stime);
  ADD (ac_etime);
  ADD (ac_io);
  s->ac_mem = merge_memory (s->ac_etime, record->ac_etime,
			    s->ac_mem, record->ac_mem);
  ++s->ac_count;
}

static void *
init_user (const struct acct *record)

{
  struct acctusr *s = malloc (sizeof (struct acctusr));

  if (s == NULL)
    fatal_error (NULL, "Out of memory.", EX_SOFTWARE);
  MOVE (ac_uid);
  MOVE (ac_utime);
  MOVE (ac_stime);
  MOVE (ac_etime);
  MOVE (ac_io);
  MOVE (ac_mem);
  s->ac_count = 1;
  return s;
}

static void
merge_command (void *summary, const struct acct *record)

{
  struct acctcmd *s = summary;

  ADD (ac_utime);
  ADD (ac_stime);
  ADD (ac_etime);
  ADD (ac_io);
  s->ac_mem = merge_memory (s->ac_etime, record->ac_etime,
			    s->ac_mem, record->ac_mem);
  ++s->ac_count;
}

void *
init_command (const struct acct *record)

{
  struct acctcmd *s = malloc (sizeof (struct acctusr));

  if (s == NULL)
    fatal_error (NULL, "Out of memory.", EX_SOFTWARE);
  memcpy (s->ac_comm, record->ac_comm, 10);
  MOVE (ac_utime);
  MOVE (ac_stime);
  MOVE (ac_etime);
  MOVE (ac_io);
  MOVE (ac_mem);
  s->ac_count = 1;
  return s;
}

static struct entry **find_entry (const long uid, const char *command)

{
  int i;
  struct entry *entry, **previous;
  unsigned hash = uid & 0xffff;

  for (i = 0; i < 10; ++i)
    {
      if (command[i] == 0)
	break;
      hash ^= command[i];
      hash &= 0xffff;
    }
  previous = &table[hash % NENTRY];
  for (entry = *previous; entry != NULL;
       previous = &entry->next, entry = entry->next)
    if (entry->uid == uid && strncmp (entry->command, command, 10) == 0)
      break;
  return previous;
}

static void
add_to_summary (const struct acct *record,
		const long uid, const char *command,
		void (*merge) (void *, const struct acct *),
		void *(*init) (const struct acct *))

{
  struct entry **previous = find_entry (uid, command);

  if (*previous != NULL)
    merge ((*previous)->data, record);
  else
    {
      struct entry *entry = *previous = malloc (sizeof (struct entry));

      if (entry == NULL)
	fatal_error (NULL, "Out of memory.", EX_SOFTWARE);
      entry->uid = uid;
      strncpy (entry->command, command, 10);
      entry->data = init (record);
    }
}

int
add_to_summaries (const struct acct *record)

{
  if (show_user_info || store_summaries)
    add_to_summary (record, record->ac_uid, "", merge_user, init_user);
  if (! show_user_info || store_summaries)
    add_to_summary (record, -1, record->ac_comm, merge_command, init_command);
  return TRUE;
}

static int
add_user_summary (const struct acctusr *record)

{
  if (record->ac_count > 0)	/* silently weed out zero count records */
    {
      struct entry **previous = find_entry (record->ac_uid, "");

      if (*previous == NULL)
	{
	  struct acctusr *s = malloc (sizeof (struct acctusr));
	  struct entry *entry = *previous = malloc (sizeof (struct entry));

	  if (s == NULL || entry == NULL)
	    fatal_error (NULL, "Out of memory.", EX_SOFTWARE);
	  entry->uid = record->ac_uid;
	  entry->command[0] = 0;
	  entry->data = s;
	  MOVE (ac_uid);
	  MOVE (ac_utime);
	  MOVE (ac_stime);
	  MOVE (ac_etime);
	  MOVE (ac_io);
	  MOVE (ac_mem);
	  MOVE (ac_count);
	  return TRUE;
	}
      fatal_error (user_summary_path, "Duplicate user id found.", EX_DATAERR);
    }
  return TRUE;
}

static int
add_command_summary (const struct acctcmd *record)

{
  /* Note: we no longer store other and junk records in the summaries.
     Basically, if the old version has been used, we need to run sa -s
     once with the version compiled for COMPATSUM, then we can turn
     this code off, on the other hand unless there are commands called
     ***other or **junk** it does no harm to leave this in.  One other
     thing, a bug in the original version left the commands that went
     into the other and junk categories in the summary individually
     as well.  This turns out to be fortuitous, because it lets us
     ignore other and junk and get exactly the current behavior of
     the program! (Fixing this bug was revision 1.4 of this file.) */

#ifdef COMPATSUM
  if (record->ac_count >= 0 &&	/* silently weed out zero count records */
      strncmp (record->ac_comm, other_summary->ac_comm, 10) != 0 &&
      strncmp (record->ac_comm, junk_summary->ac_comm, 10) != 0)
#else
  if (record->ac_count >= 0)	/* silently weed out zero count records */
#endif
    {
      struct acctcmd *s;
      struct entry **previous = find_entry (-1, record->ac_comm);
      struct entry *entry;

      if (*previous != NULL)	/* must not have been seen before */
	goto error;		/* might handle this more softly... XXX */
      s = malloc (sizeof (struct acctcmd));
      entry = *previous = malloc (sizeof (struct entry));
      if (s == NULL || entry == NULL)
	fatal_error (NULL, "Out of memory.", EX_SOFTWARE);
      entry->uid = -1;
      memcpy (entry->command, record->ac_comm, 10);
      entry->data = s;
      memcpy (s->ac_comm, record->ac_comm, 10);
      MOVE (ac_utime);
      MOVE (ac_stime);
      MOVE (ac_etime);
      MOVE (ac_io);
      MOVE (ac_mem);
      MOVE (ac_count);
      return TRUE;
    error:
      fatal_error (command_summary_path,
		   "Duplicate command name found.", EX_DATAERR);
    }
  return TRUE;
}

void
initialize_summaries (void)

{
  other_summary = calloc (1, sizeof (struct acctcmd));
  if (other_summary == NULL)
    fatal_error (NULL, "Out of memory.", EX_SOFTWARE);
  strcpy (other_summary->ac_comm, "***other");
  junk_summary = calloc (1, sizeof (struct acctcmd));
  if (junk_summary == NULL)
    fatal_error (NULL, "Out of memory.", EX_SOFTWARE);
  strcpy (junk_summary->ac_comm, "**junk**");
  if (read_summary)
    {
      iterate_user_summary (add_user_summary);
      iterate_command_summary (add_command_summary);
    }
}

void
show_summary (void)

{
  int i;
  struct entry *entry;

  for (i = 0; i < NENTRY; ++i)
    for (entry = table[i]; entry != NULL; entry = entry->next)
      {
	if (entry->uid == -1)
	  {
	    bool_t add_to_sort = TRUE;
	    if (! show_user_info || store_summaries)
	      {
		struct acctcmd *s = reclassify (entry->data);

		if (s)
		  {
		    struct acctcmd *record = entry->data;

		    ADD (ac_utime);
		    ADD (ac_stime);
		    ADD (ac_io);
		    s->ac_mem = merge_memory (s->ac_etime, record->ac_etime,
					      s->ac_mem, record->ac_mem);
		    ++s->ac_count;
		    add_to_sort = FALSE;
		  }
	      }
	    if (! show_user_info)
	      {
		if (show_percent_time)
		  {
		    struct acctcmd *c = entry->data;

		    total_time += comp_conv (c->ac_utime);
		    total_time += comp_conv (c->ac_stime);
		  }
		if (add_to_sort)
		  add_to_sort_table (entry->data);
	      }
	  }
	else if (show_user_info)
	  {
	    if (show_percent_time)
	      {
		struct acctusr *u = entry->data;

		total_time += comp_conv (u->ac_utime);
		total_time += comp_conv (u->ac_stime);
	      }
	    add_to_sort_table (entry->data);
	  }
      }
  if (show_user_info)
    {
      show_user_header ();
      iterate_sort_table (user_compare_funcs[sort_mode], show_user_entry);
    }
  else
    {
      if (other_summary->ac_count > 0)
	add_to_sort_table (other_summary);
      if (junk_summary->ac_count > 0)
	add_to_sort_table (junk_summary);
      show_command_header ();
      iterate_sort_table (command_compare_funcs[sort_mode],
			  show_command_entry);
    }
}

void
save_summaries (void)

{
  int i;
  struct entry *entry;
  sigset_t signals;
  FILE *cmd = make_temporary ('c', command_temp_name);
  FILE *usr = make_temporary ('u', user_temp_name);

  for (i = 0; i < NENTRY; ++i)
    for (entry = table[i]; entry != NULL; entry = entry->next)
      {
	if (entry->uid == -1)
	  {
	    if (fwrite (entry->data, sizeof (struct acctcmd), 1, cmd) != 1)
	      fatal_error (command_temp_name, "Write error.", EX_IOERR);
	  }
	else
	  {
	    if (fwrite (entry->data, sizeof (struct acctusr), 1, usr) != 1)
	      fatal_error (user_temp_name, "Write error.", EX_IOERR);
	  }
      }
  if (fclose (cmd) == EOF)
    fatal_error (command_temp_name, NULL, EX_IOERR);
  if (fclose (usr) == EOF)
    fatal_error (user_temp_name, NULL, EX_IOERR);

  block_signals (&signals);
  move (command_temp_name, command_summary_path);
  (void) chmod (command_summary_path, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  move (user_temp_name, user_summary_path);
  (void) chmod (user_summary_path, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
  unblock_signals (&signals);
}
