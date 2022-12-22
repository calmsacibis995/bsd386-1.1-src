/* Copyright (C) 1990, 1993 Free Software Foundation, Inc.

   This file is part of GNU ISPELL.

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


/* lookup a word in a dictionary
 
   the dictionary must be sorted with one lower case word
   per line.  all lines that begin with the argument
   are printed */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "getopt.h"
#include "ispell.h"

#ifdef __STDC__
static int contains_meta (char *);
void dogrep (char *, char *);
#else
extern int contains_meta ();
extern void dogrep ();
#endif

#ifndef _tolower
#define _tolower tolower
#endif

#ifdef DICT_LIB
char *dict_lib = DICT_LIB;
char *dict_name = "ispell.words";
#else
char *dict_lib = "/usr/dict";
char *dict_name = "words";
#endif

/* not dictionary, no fold */
int
cmp00 (line, word)
  unsigned char *line, *word;
{
  int x;
  while (1)
    {
      if (*word == 0)
	return (0);
      if (*line == 0)
	return (1);
      x = *line++ - *word++;
      if (x != 0)
	return (x);
    }
}

/* not dictionary, fold */
int
cmp01 (line, word)
  unsigned char *line, *word;
{
  int x;
  while (1)
    {
      if (*word == 0)
	return (0);
      if (*line == 0)
	return (1);
      if (isupper (*line))
	x = _tolower (*line) - *word;
      else
	x = *line - *word;
      if (x != 0)
	return (x);
      line++;
      word++;
    }
}

/* dictionary, no fold */
int
cmp10 (line, word)
  char *line, *word;
{
  int x;
  while (1)
    {
      if (*word == 0)
	return (0);
      if (*line == 0)
	return (1);
      if (!isalpha (*line) && !isdigit (*line) && !isspace (*line))
	{
	  line++;
	  continue;
	}
      x = *line++ - *word++;
      if (x != 0)
	return (x);
    }
}

/* dictionary, fold */
int
cmp11 (line, word)
  char *line, *word;
{
  int x;
  while (1)
    {
      if (*word == 0)
	return (0);
      if (*line == 0)
	return (1);
      if (!isalpha (*line) && !isdigit (*line) && !isspace (*line))
	{
	  line++;
	  continue;
	}
      if (isupper (*line))
	x = _tolower (*line) - *word;
      else
	x = *line - *word;
      if (x != 0)
	return (x);
      line++;
      word++;
    }
}

void
usage ()
{
#define P(s) fprintf (stderr, s)
  P ("usage: look [-dfr] string [file]\n");
  P (" -d  dictionary order: consider only letters, digits, and spaces\n");
  P (" -f  fold upper case to lower\n");
  P (" -r  string is a regular expression\n");
  exit (1);
}

int dflag;
int fflag;
int rflag;

int
main (argc, argv)
  int argc;
  char **argv;
{
  char *dict;
  char *word;
  FILE *f;
  struct stat statb;
  long start, end, pos;
  char buf[100];
  int c;
  extern char *optarg;
  extern int optind;
  int (*cmpf) ();

  while ((c = getopt (argc, argv, "dfr")) != EOF)
    {
      switch (c)
	{
	case 'r':
	  rflag = 1;
	  break;
	case 'd':
	  dflag = 1;
	  break;
	case 'f':
	  fflag = 1;
	  break;
	default:
	  usage ();
	}
    }

  if (optind == argc)
    usage ();

  word = argv[optind++];

  if (optind != argc)
    {
      dict = argv[optind++];
    }
  else
    {
      dflag = 1;
      fflag = 1;
      dict = (char *) xmalloc (strlen (dict_lib) + strlen (dict_name) + 2);
      strcpy (dict, dict_lib);
      strcat (dict, "/");
      strcat (dict, dict_name);
    }

  if (rflag)
    {
      if (word[0] == '^' && !contains_meta (word + 1))
	{
	  /* only meta character is '^' at beginning -
			 * it's ok to skip it and use the binary search
			 */
	  word++;
	}
      else
	{
	  dogrep (word, dict);
	  return 0;
	}
    }

  if (dflag)
    {
      /* if dictionary order, thow out non-dictionary characters */
      char *p, *q, *nword;
      nword = (char *) xmalloc (strlen (word) + 1);
      p = word;
      q = nword;
      while (*p)
	{
	  if (isalpha (*p) || isdigit (*p) || isspace (*p))
	    *q++ = *p;
	  p++;
	}
      *q = 0;
      word = nword;

      if (fflag)
	cmpf = cmp11;
      else
	cmpf = cmp10;
    }
  else if (fflag)
    {
      cmpf = cmp01;
    }
  else
    {
      cmpf = cmp00;
    }

  if (fflag)
    {
      char *p;
      for (p = word; *p; p++)
	if (isupper (*p))
	  *p = _tolower (*p);
    }

  if ((f = fopen (dict, "r")) == NULL)
    {
      fprintf (stderr, "can't open %s\n", dict);
      exit (1);
    }

  if (stat (dict, &statb) < 0)
    {
      fprintf (stderr, "can't stat %s\n", dict);
      exit (1);
    }

  start = 0;
  end = statb.st_size;
  fseek (f, (end - start) / 2, 0);
  while ((c = getc (f)) != EOF)
    if (c == '\n')
      break;

  /* first, find last word before the one we want */
  while (1)
    {
      pos = ftell (f);
      fgets (buf, sizeof buf, f);
      if ((*cmpf) (buf, word) < 0)
	start = pos;
      else
	end = pos;

      pos = start + (end - start) / 2 - 100;
      if (pos <= start)
	break;
      fseek (f, pos, 0);
      while ((c = getc (f)) != EOF)
	if (c == '\n')
	  break;
      if (c == EOF)
	break;
    }
  if (pos < 0)
    pos = 0;
  fseek (f, pos, 0);

  /* 
   * If we are at the file start, then don't skip to the next newline,
   * because then we would never match the first word in the file.  
   */
  if (pos > 0)
    while ((c = getc (f)) != EOF)
      if (c == '\n')
        break;

  while (fgets (buf, sizeof buf, f) != NULL)
    {
      int x;
      x = (*cmpf) (buf, word);
      if (x > 0)
	break;
      if (x == 0)
	fputs (buf, stdout);
    }

  return 0;
}

static int
contains_meta (p)
  char *p;
{
  while (1)
    {
      switch (*p)
	{
	case 0:
	  return (0);
	case '$':
	case '^':
	case '.':
	case '*':
	case '+':
	case '?':
	case '[':
	case ']':
	case '\\':
	  return (1);
	}
      p++;
    }
}

void
dogrep (word, dict)
  char *word;
  char *dict;
{
  char *av[5];
  char **avp;
  char *prog;

  if (!contains_meta (word))
    prog = "fgrep";
  else if (fflag)
    prog = "grep";
  else
    prog = "egrep";
  avp = av;
  *avp++ = prog;
  if (fflag)
    *avp++ = "-i";
  *avp++ = word;
  *avp++ = dict;
  *avp = NULL;

  execvp (prog, av);
  fprintf (stderr, "can't exec %s\n", prog);
}
