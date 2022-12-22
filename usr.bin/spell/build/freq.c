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

/* compute the frequencies of character pairs in a file
   then write a list of characters used, plus a sorted list
   of frequencies
 
   this program needs to run on 80286's, so it doesn't use
   any individual data structure larger than 64k. */

#include <stdio.h>
#include <ctype.h>
#include "charset.h"

/* this array contains letters to use when generating near misses */
char near_miss_letters[256];
int nnear_miss_letters;

/* this array has 1 for any character that is in near_miss_letters */
char near_map[256];

long *table[256];
char used[256];

int
main ()
{
  int c1, c2;
  long count;
  int i, j;
  FILE *out;

  if ((out = popen ("sort -nr", "w")) == NULL)
    {
      fprintf (stderr, "can't make pipe to sort\n");
      exit (1);
    }

  for (i = 0; i < 256; i++)
    table[i] = (long *) xcalloc (256, sizeof (long));

  c1 = 0;
  while (c1 == 0)
    {
      c1 = getchar ();
      if (c1 == EOF)
	{
	  fprintf (stderr, "no input\n");
	  exit (1);
	}
      c1 = charset[c1].lowercase;
    }
  used[c1] = 1;

  while ((c2 = getchar ()) != EOF)
    {
      c2 = charset[c2].lowercase;
      if (c2 == 0)
	continue;
      used[c2] = 1;
      table[c1][c2]++;
      c1 = c2;
    }

  /* a big "count" so this line will be at the top of the
	 * sorted output
	 */
  fprintf (out, "30000 ");
  for (c1 = 0; c1 < 256; c1++)
    if (used[c1])
      putc (c1, out);
  putc ('\n', out);

  for (i = 0; i < 256; i++)
    {
      if (used[i] == 0)
	continue;
      for (j = 0; j < 256; j++)
	{
	  if (used[j] == 0)
	    continue;
	  count = table[i][j];
	  if (count)
	    fprintf (out, "%ld %c%c\n", count, i, j);
	}
    }

  pclose (out);
  return 0;
}
