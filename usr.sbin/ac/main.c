/*
 * ac -- login accounting program  
 */

#include "ac2.h"

void make_name_list(void);

void 
main(int argc, char **argv)
{
  struct utmp ut;
  FILE *fp;
  char *wtmp_file;	/* the name of the wtmp file.  Default is WTMP_FILE */
  
  int c;
  extern char *optarg;
  extern int optind;
  extern int opterr;
  int now;
  
  print_names_flag = FALSE;
  daily_flag = FALSE;
  wtmp_file = WTMP_FILE;
  opterr = 0;

  while ((c = getopt(argc, argv, "w:pd")) != -1)
    switch (c) 
      {
      case 'w':
	wtmp_file = optarg;
	break;
      case 'p':
	print_names_flag++;
	break;
      case 'd':
	daily_flag++;
	break;
      default:
	fprintf(stderr, "Usage: %s [-w wtmp] [-p] [-d] [username ...]\n", argv[0]);
	exit(1);
	break;
      }
  

  fp = fopen(wtmp_file, "r");
  if (!fp) 
    {
      perror(wtmp_file);
      exit(1);
    }
  
  /* set everything in tty_list to NULL, in case it is not */
  t_list.next = NULL;
  t_list.name = mstrdup("\0");
  t_list.line = mstrdup("\0");
  t_list.time_in = 0;
  t_list.time_out = 0;
  
  while (fread(&ut, sizeof(struct utmp), 1, fp)) 
    {
      *ut.ut_host = '\0';
      do_tty_list(ut.ut_line, ut.ut_name, ut.ut_time);
    }
  
  now = time(0);
  
  update_times(now);
  
  if (daily_flag)
    {
      int this_midnight;

      this_midnight = find_midnight(now);

      ti_list.next = NULL;
      ti_list.name = mstrdup("\0");
      ti_list.total = 0;
      ti_list.midnight = this_midnight;
      
      make_time_list(this_midnight);

      print_time(argc, optind, argv);
    }
  else 
    {
      reset_name_list();
      make_name_list();
  
      print_name(argc, optind, argv, TRUE);
    }
  
  exit(0);
}


