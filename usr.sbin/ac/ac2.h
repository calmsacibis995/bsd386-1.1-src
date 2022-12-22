#include "copyright.h"
#include <stdlib.h>
#include <stdio.h>
#include <utmp.h>
#include <string.h>
#include <sys/types.h>

#ifndef WTMP_FILE
#define WTMP_FILE	"/usr/adm/wtmp"
#endif		/* not WTMP_FILE */

#ifndef TRUE
#define TRUE	1
#endif		/* not TRUE */

#define FALSE	0

typedef struct ttylist *tty_list;

struct ttylist {
    char *name;		/* login name.  This will not be more than 8 */
    char *line;		/* tty.  This will not be more than 5 */
    int time_in;	/* the time logged in */
    int time_out;	/* the time logged out */
    tty_list next;	/* the next entry in the list */
} t_list;

typedef struct namelist *name_list;

struct namelist {
    char *name;			/* login name */
    int time;			/* time logged in */
    name_list next;		/* the next entry in the list */
} n_list;

typedef struct timelist *time_list;

struct timelist {
  int midnight;	      	/* the beginning of this day. */
  int total;	/* the total time logged in for the day beginning @ midnight */
  char *name;		/* the user's name */
  time_list next;	/* the next entry in the list (when we search) */
} ti_list;

#if 0
/* standard prototypes (put in to try to get gcc -Wall to stop complaining)*/
int getopt(int, char **, char *);
void exit(int);
#endif

/* non-standard prototypes */
void do_tty_list(char *, char *, int);
void do_name_list(char*, int);
void do_time_list(char *, int, int);
void make_name_node(name_list, char *, int);
void make_tty_node(tty_list, char *, char *, int);
void make_time_node(time_list, char *, int, int);
void print_name(int, int, char **, int);
void print_time(int, int, char **);
void update_times(int);
void reset_name_list(void);
char *mstrdup(char *);
int find_midnight(time_t);
int make_time_list(int);
int day_day(char *, time_t);

/* global variables (for passing command-line flags */
int print_names_flag;		/* print individual totals if not FALSE */
int daily_flag;	 	/* calculate from midnight to midnight if not FALSE */


