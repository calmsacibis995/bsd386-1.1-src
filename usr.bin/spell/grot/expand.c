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

/* this program takes a dictionary with flags on the standard input,
   and expands them onto the standard output. */

#include <stdio.h>
#include <ctype.h>
#include "ispell.h"
#include "hash.h"
#include "good.h"

/* this array contains letters to use when generating near misses */
char near_miss_letters[256];
int nnear_miss_letters;

/* this array has 1 for any character that is in near_miss_letters */
char near_map[256];

char buf[BUFSIZ];

int
main (argc, argv)
  int argc;
  char **argv;
{
  FILE *in;
  int i, n;
  char *p;

  argc--;
  argv++;

  if (argc)
    {
      if ((in = fopen (*argv, "r")) == NULL)
	{
	  (void) fprintf (stderr, "can't open %s\n", *argv);
	  exit (1);
	}
    }
  else
    {
      in = stdin;
    }

  while (fgets (buf, sizeof buf, in) != NULL)
    {
      if ((p = (char *) strchr (buf, '\n')))
	*p = 0;

      downcase (buf, buf);
      n = expand (buf);
      for (i = 0; i < n; i++)
	puts (words[i]);
    }

  return 0;
}
