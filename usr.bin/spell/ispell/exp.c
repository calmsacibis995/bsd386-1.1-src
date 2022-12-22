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

#include <stdio.h>
#include <ctype.h>
#include "ispell.h"
#include "hash.h"
#include "good.h"
char words[50][MAX_WORD_LEN];

int
expand (w)
  char *w;
{
  char *p;
  int i;
  int len;
  int c;

  i = 0;

  p = strchr (w, '/');

  if (p)
    *p = 0;

  downcase (words[i++], w);

  if (p == NULL)
    {
      words[i][0] = 0;
      return (i);
    }

  len = strlen (w);

  p++;
  while (1)
    {
      (void) strcpy (words[i], words[0]);

      c = *p;
      if (isupper (c))
	c = tolower (c);

      switch (*p)
	{
	case 'v':
	  if (w[len - 1] == 'e')
	    (void) strcpy (words[i] + len - 1, "ive");
	  else
	    (void) strcpy (words[i] + len, "ive");
	  break;
	case 'n':
	  if (w[len - 1] == 'e')
	    (void) strcpy (words[i] + len - 1, "ion");
	  else if (w[len - 1] == 'y')
	    (void) strcpy (words[i] + len - 1, "ication");
	  else
	    (void) strcpy (words[i] + len, "en");
	  break;
	case 'x':
	  if (w[len - 1] == 'e')
	    (void) strcpy (words[i] + len - 1, "ions");
	  else if (w[len - 1] == 'y')
	    (void) strcpy (words[i] + len - 1, "ications");
	  else
	    (void) strcpy (words[i] + len, "ens");
	  break;
	case 'h':
	  if (w[len - 1] == 'y')
	    (void) strcpy (words[i] + len - 1, "ieth");
	  else
	    (void) strcpy (words[i] + len, "th");
	  break;
	case 'y':
	  (void) strcpy (words[i] + len, "ly");
	  break;
	case 'm':
	  (void) strcpy (words[i] + len, "'s");
	  break;
	case 'g':
	  if (w[len - 1] == 'e')
	    (void) strcpy (words[i] + len - 1, "ing");
	  else
	    (void) strcpy (words[i] + len, "ing");
	  break;
	case 'j':
	  if (w[len - 1] == 'e')
	    (void) strcpy (words[i] + len - 1, "ings");
	  else
	    (void) strcpy (words[i] + len, "ings");
	  break;
	case 'd':
	  if (w[len - 1] == 'e')
	    {
	      (void) strcpy (words[i] + len - 1, "ed");
	    }
	  else if (w[len - 1] == 'y')
	    {
	      if (isvowel (w[len - 2]))
		(void) strcpy (words[i] + len, "ed");
	      else
		(void) strcpy (words[i] + len - 1, "ied");
	    }
	  else
	    {
	      (void) strcpy (words[i] + len, "ed");
	    }
	  break;
	case 't':
	  if (w[len - 1] == 'e')
	    {
	      (void) strcpy (words[i] + len - 1, "est");
	    }
	  else if (w[len - 1] == 'y')
	    {
	      if (isvowel (w[len - 2]))
		(void) strcpy (words[i] + len, "est");
	      else
		(void) strcpy (words[i] + len - 1, "iest");
	    }
	  else
	    {
	      (void) strcpy (words[i] + len, "est");
	    }
	  break;
	case 'r':
	  if (w[len - 1] == 'e')
	    {
	      (void) strcpy (words[i] + len - 1, "er");
	    }
	  else if (w[len - 1] == 'y')
	    {
	      if (isvowel (w[len - 2]))
		(void) strcpy (words[i] + len, "er");
	      else
		(void) strcpy (words[i] + len - 1, "ier");
	    }
	  else
	    {
	      (void) strcpy (words[i] + len, "er");
	    }
	  break;
	case 'z':
	  if (w[len - 1] == 'e')
	    {
	      (void) strcpy (words[i] + len - 1, "ers");
	    }
	  else if (w[len - 1] == 'y')
	    {
	      if (isvowel (w[len - 2]))
		(void) strcpy (words[i] + len, "ers");
	      else
		(void) strcpy (words[i] + len - 1, "iers");
	    }
	  else
	    {
	      (void) strcpy (words[i] + len, "ers");
	    }
	  break;
	case 's':
	  if (w[len - 1] == 'y')
	    {
	      if (isvowel (w[len - 2]))
		(void) strcpy (words[i] + len, "s");
	      else
		(void) strcpy (words[i] + len - 1, "ies");
	    }
	  else if (issxzh (w[len - 1]))
	    {
	      (void) strcpy (words[i] + len, "es");
	    }
	  else
	    {
	      (void) strcpy (words[i] + len, "s");
	    }
	  break;
	case 'p':
	  if (w[len - 1] == 'y')
	    {
	      if (isvowel (w[len - 2]))
		(void) strcpy (words[i] + len, "ness");
	      else
		(void) strcpy (words[i] + len - 1, "iness");
	    }
	  else
	    {
	      (void) strcpy (words[i] + len, "ness");
	    }
	  break;
	default:
	  (void) fprintf (stderr, "syntax error %s\n", w);
	  break;
	}
      if (strlen (words[i]) < 4)
	{
	  (void) fprintf (stderr, "warning meaningless flag: ");
	  (void) fprintf (stderr, "%s/%c ", words[0], *p);
	  (void) fprintf (stderr, "produces less than 4 bytes\n");
	}
      else
	{
	  i++;
	}
      p++;
      if (*p == 0 || *p == '\n')
	break;
      if (*p != '/')
	{
	  (void) fprintf (stderr, "syntax error %s\n", w);
	  break;
	}
      p++;
      if (*p == 0)
	{
	  (void) fprintf (stderr, "syntax error %s\n", w);
	  break;
	}
    }
  words[i][0] = 0;
  return (i);
}
