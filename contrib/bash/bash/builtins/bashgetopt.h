/* bashgetopt.h -- extern declarations for stuff defined in bashgetopt.c. */

/* See getopt.h for the explanation of these variables. */

#if !defined (__BASH_GETOPT_H)
#  define __BASH_GETOPT_H

extern char *list_optarg;

extern int list_optopt;

extern WORD_LIST *lcurrent;
extern WORD_LIST *loptend;

extern int internal_getopt ();
extern void reset_internal_getopt ();
extern void report_bad_option ();

#endif /* !__BASH_GETOPT_H */
