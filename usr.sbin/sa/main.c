/*
 * $Header: /bsdi/MASTER/BSDI_OS/usr.sbin/sa/main.c,v 1.2 1993/03/03 19:40:24 sanders Exp $
 * $Log: main.c,v $
 * Revision 1.2  1993/03/03  19:40:24  sanders
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
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include "sa.h"

static void
dump_acct (void)

{
  static int signals[] = {
    SIGHUP, SIGINT, SIGQUIT, SIGXCPU, SIGXFSZ, SIGVTALRM, 0,
  };

  trap_signals (signals, terminate);
  if (show_user_dump)
    iterate_accounting_file (accounting_data_path, dump_user_command);
  else
    {
      initialize_summaries ();
      iterate_accounting_file (accounting_data_path, add_to_summaries);
      show_summary ();
      if (store_summaries)
	save_summaries ();
    }
}

int
main (int argc, char * const * argv)

{
  for ( ; ; )
    switch (getopt (argc, argv, "abcdDfijkKlmnrstuv:S:U:"))
      {
      case 'a':
	use_other = FALSE;
	break;
      case 'b':
	sort_mode = AVERAGE_TIME;
	break;
      case 'c':
	show_percent_time = TRUE;
	break;
      case 'd':
	sort_mode = AVERAGE_DISK;
	break;
      case 'D':
	show_total_disk = TRUE;
	sort_mode = TOTAL_DISK;
	break;
      case 'f':
	junk_interactive = FALSE;
	break;
      case 'i':
	read_summary = FALSE;
	break;
      case 'j':
	use_average_times = TRUE;
	break;
      case 'k':
	sort_mode = AVERAGE_MEMORY;
	break;
      case 'K':
	show_kcore_secs = TRUE;
	sort_mode = KCORE_SECS;
	break;
      case 'l':
	separate_sys_and_user = TRUE;
	break;
      case 'm':
	show_user_info = TRUE;
	break;
      case 'n':
	sort_mode = CALL_COUNT;
	break;
      case 'r':
	reverse_sort = TRUE;
	break;
      case 's':
	store_summaries = TRUE;
	break;
      case 't':
	show_real_over_cpu = TRUE;
	break;
      case 'u':
	show_user_dump = TRUE;
	break;
      case 'v':
	junk_threshold = atoi (optarg);
	break;
      case 'S':
	command_summary_path = optarg;
	break;
      case 'U':
	user_summary_path = optarg;
	break;
      case EOF:
	switch (argc - optind)
	  {
	  case 1:
	    accounting_data_path = argv[optind];
	  case 0:
	    dump_acct ();
	    return EX_OK;
	  }
      default:
	fprintf (stderr, "Usage: sa [-abcdDfijkKlnrstuv] [-S savacctfile] [-U usracctfile] [file]\n");
	return EX_USAGE;
      }

}
