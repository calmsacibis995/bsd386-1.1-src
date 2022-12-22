/*
 *   $Id: main.c,v 1.1.1.1 1993/02/10 18:05:35 sanders Exp $
 *
 *   This code was written by Craig Southeren whilst under contract
 *   to Computer Sciences of Australia, Systems Engineering Division.
 *   It has been kindly released by CSA into the public domain.
 *
 *   Neither CSA or me guarantee that this source code is fit for anything,
 *   so use it at your peril. I don't even work for CSA any more, so
 *   don't bother them about it. If you have any suggestions or comments
 *   (or money, cheques, free trips =8^) !!!!! ) please contact me
 *   care of geoffw@extro.ucc.oz.au
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "version.h"
#include "defs.h"
#include "postscri.h"
#include "print.h"
#include "main.h"

/********************************
  defines
 ********************************/
#ifndef True
#define True	1
#define False	0
#endif

/* environment variables */
#define		PRINTER		"PRINTER"		/* sets name of printer */
#define		NENSCRIPT	"NENSCRIPT"		/* contains options */

#ifndef MSDOS
#define		LPR		"lpr -P"		/* spooler with option to set name of printer */
#endif

#define	strdup(s)	(strcpy ((char *)malloc (strlen(s)+1), s))

/********************************
  imports
 ********************************/

/********************************
  exports
 ********************************/
char *progname;

/********************************
  globals
 ********************************/


/********************************
  usage
 ********************************/

void usage ()

{
  printf ("usage: %s -?12BGghlNnRrsVWw -f font -F titlefont -b header -L lines -p filename -P printer -S classification -# copies file...\n", progname);
  exit (0);
}

/*********************************

variables set by options

 *********************************/

int  landscape = False;			/* True if in ladnscape mode, i.e. -r specified */
int  columns = 1;				/* number of columns to print with */
char *outputfilename = NULL;			/* name of output file, if still NULL after parsing args then use lpr */
char *bodyfont = NULL;			/* font to use for text */
char *titlefont = NULL;			/* font to use for titles */
int  wrap = True;				/* True if to wrap long lines */
int  titlesenabled = True;			/* True if page titles enables */
char *title = NULL;				/* set to title if specified */
int  copies = 1;				/* number of copies of each page */
int  gaudy = False;				/* enables gaudy mode */
int  force_lines = 0;				/* force number of lines per page */
char *classification = NULL;			/* security classification */
int  line_numbers = False;                    /* true if to display line numbers */
char *printername = DEF_PRINTER;		/* name of printer to use when output goes to lpr */


/*
 * forward declarations if in ANSI mode
 */

#ifdef __STDC__
char * get_string  (char **);
void parse_options (int *, char ***);
void parse_env     (char *);


#endif


/********************************
  parse_options
 ********************************/

#define	ARGC	(*argc)
#define	ARGV	(*argv)

void parse_options (argc, argv)

int *argc;
char ***argv;

{
  char c;

  int i;

  /* parse the options */

next_argv:
 while (--ARGC > 0 && (*++ARGV)[0] == '-')
    while (c = *++ARGV[0])
      switch (c) {

/* -? : print help message */
        case '?' :
        case 'h' :
          usage ();
          break;

/* -1 : single column mode */
        case '1' :
          columns = 1;
          break;

/* -2 : double column mode */
        case '2' :
          columns = 2;
          break;

/* -#: set number of copies */
        case '#' :
          if (*++ARGV[0])
            copies = atoi (ARGV[0]);
          else {
            if (ARGC < 1)
              fprintf (stderr, "%s: -# option requires argument\n", progname);
            else {
              --ARGC;
              copies = atoi (*++ARGV);
            }
          }
          if (copies < 1) {
            fprintf (stderr, "%s: illegal number of copies specified - defaulting to one\n");
            copies = 1;
          }
          goto next_argv;
          break;

/* -b: set page title */
        case 'b' :
          if (*++ARGV[0])
            title = ARGV[0];
          else {
            if (ARGC < 1)
              fprintf (stderr, "%s: -b option requires argument\n", progname);
            else {
              --ARGC;
              title = *++ARGV;
            }
          }
          goto next_argv;
          break;

/* -f : set font to use for printing body */
        case 'f' :
          if (*++ARGV[0])
            bodyfont = ARGV[0];
          else {
            if (ARGC < 1)
              fprintf (stderr, "%s: -f option requires argument\n", progname);
            else {
              --ARGC;
              bodyfont = *++ARGV;
            }
          }
          goto next_argv;
          break;

/* -N : display line numbers */
        case 'N' :
          line_numbers = True;
          break;

/* -n : disable line numbers */
        case 'n' :
          line_numbers = False;
          break;

/* -p: send to file rather than lpr */
        case 'p' :
          if (*++ARGV[0])
            outputfilename = ARGV[0];
          else {
            if (ARGC < 1)
              fprintf (stderr, "%s: -p option requires argument\n", progname);
            else {
              --ARGC;
              outputfilename = *++ARGV;
            }
          }
          goto next_argv;
          break;

/* -r : landscape mode */
        case 'r' :
          landscape = True;
          break;

/* -w : wrap mode */
        case 'w' :
          wrap = True;
          break;

/* -B : disable page headings and disable gaudy mode */
        case 'B' :
          titlesenabled = False;
          break;

/* -F : set font to use for printing titles */
        case 'F' :
          if (*++ARGV[0])
            titlefont = ARGV[0];
          else {
            if (ARGC < 1)
              fprintf (stderr, "%s: -F option requires argument\n", progname);
            else {
              --ARGC;
              titlefont = *++ARGV;
            }
          }
          goto next_argv;
          break;

/* -g : disable gaudy mode */
        case 'g' :
          gaudy = False;
          break;

/* -G : enable gaudy mode */
        case 'G' :
          gaudy = True;
          break;

/* -l : don't set number of lines of page, i.e. use maximum */
        case 'l' :
          force_lines = 0;
          break;

/* -L : force number of lines per page */
        case 'L' :
          if (*++ARGV[0])
            force_lines = atoi(ARGV[0]);
          else {
            if (ARGC < 1)
              fprintf (stderr, "%s: -x option requires argument\n", progname);
            else {
              --ARGC;
              force_lines = atoi (*++ARGV);
            }
          }
	  if (force_lines <= 0) {
            fprintf (stderr, "%s: ignoring illegal arguement to -L option\n", progname);
	    force_lines = 0;
          }
          goto next_argv;
          break;

/* -P: set printer name - overrides PRINTER environment variables */
        case 'P' :
          if (*++ARGV[0])
            printername = ARGV[0];
          else {
            if (ARGC < 1)
              fprintf (stderr, "%s: -P option requires argument\n", progname);
            else {
              --ARGC;
              printername = *++ARGV;
            }
          }
          goto next_argv;
          break;

/* -R : portrait mode */
        case 'R' :
          landscape = False;
          break;

/* -S: set security classification */
        case 'S' :
          if (*++ARGV[0])
            classification = ARGV[0];
          else {
            if (ARGC < 1)
              fprintf (stderr, "%s: -S option requires argument\n", progname);
            else {
              --ARGC;
              classification = *++ARGV;
            }
          }
          goto next_argv;
          break;

/* -s : disable security classification printing */
        case 's' :
          classification = NULL;
          break;


/* -V : print version number */
	case 'V' :
          fputs (version_string,   stderr);
          fputs ("\n\n",           stderr);
          fputs (copyright_string, stderr);
          exit (0);
          break;

/* -W : turn off wrap mode */
        case 'W' :
          wrap = False;
          break;

/* single char option */
/*        case 'x' :
          allow_checkouts = True;
          break;
*/

/* option with string */
/*        case 'x' :
          if (*++ARGV[0])
            optionstring = ARGV[0];
          else {
            if (ARGC < 1)
              fprintf (stderr, "%s: -x option requires argument\n", progname);
            else {
              --ARGC;
              optionstring = *++ARGV;
            }
          }
          goto next_argv;
          break;
*/
        default :
          fprintf (stderr, "%s: unknown option %c\n", progname, c);
          ARGC = 0;
          break;
      }
}

/********************************
  get_string
    return the next string from the argument
 ********************************/

char * get_string (str)

char **str;

#define	STR	(*str)

{
  char *ret = NULL;
  int  in_dquote, in_squote, in_bquote;
  int  quote_count;

  /* skip leading whitespace */
  while (*STR == ' ' ||
         *STR == '\t')
    STR++;

  /* return if end of string */
  if (*STR == '\0')
    return NULL;

  /* collect characters until end of string or whitespace */
  in_dquote = -1;
  in_squote = -1;
  in_bquote = -1;
  quote_count = 0;
  ret = STR;
  while (*STR != '\0' &&
         (quote_count > 0 || (*STR != ' ' && *STR != '\t'))
        ) {
    switch (*STR) {
      case '"':
        in_dquote = -in_dquote;
        quote_count += in_dquote;
        break;

      case '\'':
        in_squote = -in_squote;
        quote_count += in_squote;
        break;

      case '`':
        in_bquote = -in_bquote;
        quote_count += in_bquote;
        break;

      case '\\':
        if (STR[1] != '\0')
          STR++;
        break;
    }
    STR++;
  }
  if (*STR != '\0') {
    *STR = '\0';
    STR++;
  }

  if (quote_count > 0)
    fprintf (stderr, "unmatched quote in %s environment variable\n", NENSCRIPT);

  return ret;
}


/********************************
  parse_env
    process the options specified in the supplied string
 ********************************/

void parse_env (env)

char *env;

{
  int argc;
  char *s;
  char **argv;
  int i;

  if (env == NULL)
    return;

  /* insert a "fake" argv[0] which corresponds to the real
     argv[0] passed to main. The allows us to use same routine
     for handling the real arguments */
  argv = (char **)malloc (sizeof (char *));
  argc = 1;
  argv[0] = progname;

  /* split the single environment string into separate strings so we can pass
     them to parse_options */
  while ((s = get_string (&env)) != NULL) {
    argv = (char **)realloc ((void *)argv, (argc+1) * sizeof (char *));
    argv[argc] = s;
    argc++;
  }

  /* parse the options */
  parse_options (&argc, &argv);
}

/********************************
  main
 ********************************/

int main (argc, argv)

int argc;
char *argv[];

{
  char *env;
  char cmd[1024];


  FILE *outputstream;				/* opened output stream - either file or lpr */
  FILE *inputstream;				/* opened input stream, 0 if max */

  /* extract the program name */
  if ((progname = strrchr (argv[0], '/')) == NULL)
    progname = argv[0];
  else
    progname++;

  /* get the printer environment variable */
  if ((env = getenv (PRINTER)) != NULL)
    printername = env;

  /* handle the NENSCRIPT environment variable */
  if ((env = getenv (NENSCRIPT)) != NULL)
    parse_env (strdup(env));

  /* parse arguments */
  parse_options (&argc, &argv);

  /* open either the output file or a pipe to lpr */
  if (outputfilename != NULL) {
    if (strcmp (outputfilename, "-") == 0)
      outputstream = stdout;
    else if ((outputstream = fopen (outputfilename, "w")) == NULL) {
      perror (outputfilename);
      exit (1);
    }
  } else {
#ifdef MSDOS
    if ((outputstream = fopen (printername, "w")) == NULL) {
      perror (printername);
      exit (1);
    }
#else
    sprintf (cmd, "%s %s", LPR, printername);
    if ((outputstream = popen (cmd, "w")) == NULL) {
      perror (LPR);
      exit (1);
    }
#endif
  }

  /* set up the fonts */
  if (bodyfont == NULL)
    bodyfont = (columns == 2 && landscape) ? LANDSCAPE2COLFONT : NORMALFONT;

  if (titlefont == NULL)
    titlefont = TITLEFONT;

  /* display the header */
  StartJob (outputstream, *argv, landscape, columns, bodyfont,
            titlefont, wrap, titlesenabled, title, copies, gaudy, force_lines,
            classification);

  /* if no arguments, then accept stdin */
  if (argc < 1)
    print_file (stdin, outputstream, "stdin", line_numbers);
  else for (;argc > 0;argc--) {
    if ((inputstream = fopen (*argv, "r")) == NULL)
      perror (*argv);
    else
      print_file (inputstream, outputstream, *argv, line_numbers);
    argv++;
  }

  /* output trailer */
  EndJob (outputstream);

  /* finished */
  exit (0);
}
