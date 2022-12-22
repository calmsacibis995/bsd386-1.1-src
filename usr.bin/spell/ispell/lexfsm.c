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

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "ispell.h"

unsigned char **lextable;
unsigned char *lexchar;
char **lexdecode;
int lexletters;

/* if out is an odd number of bytes long, must zero the
 * remaining byte because hash() works 16 bits at a time.
 * therefore, always write 2 zeros
 *
 * return the number of bytes in the compessed output.
 *
 * If the string contains a character that is not in the lexchar[]
 * array, then the word cannot possiblally be in the dictionary.
 * Therefore return 0.
 */
int
lexword (in, len, out)
  char *in;
  int len;
  unsigned char *out;
{
  unsigned char c1, c2, code;
  unsigned char *startout;
  startout = out;
newc1:
  if (len-- == 0)
    {
      out[0] = 0;
      out[1] = 0;
      return (out - startout);
    }

  c1 = lexchar[*(unsigned char *) in++];
  if (c1 == 0)
    {
      /* this character was not used anywhere in the dictionary
		 * return 0 to tell lookup() not to bother looking in
		 * the hash table
		 */
      return (0);
    }

  if (len == 0)
    {
      *out++ = c1;
      out[0] = 0;
      out[1] = 0;
      return (out - startout);
    }

  while (len--)
    {
      /* c1 is a good letter */
      c2 = lexchar[*(unsigned char *) in++];
      code = lextable[c1][c2];
      switch (code)
	{
	case 255:		/* c2 is not a letter */
	  return (0);
	case 0:		/* both letters, but no code for this pair */
	  *out++ = c1;
	  c1 = c2;
	  break;
	default:
	  *out++ = code;
	  goto newc1;
	}
    }
  *out++ = c1;
  out[0] = 0;
  out[1] = 0;
  return (out - startout);
}


void
lexprint (out, verbose)
  unsigned char *out;
  int verbose;
{
  char *p;
  char *left, *right;

  if (verbose)
    {
      left = "<";
      right = ">";
    }
  else
    {
      left = "";
      right = "";
    }
  while (*out)
    {
      p = lexdecode[*out];
      if (*p == 0)
	(void) printf ("{illegal code %d}", *out);
      else if (p[1] == 0)
	putchar (p[0]);
      else
	printf ("%s%s%s", left, p, right);
      out++;
    }
}

int
lexsize (NOARGS)
{
  return (256 * 2 + 256 + lexletters * lexletters);
}


void
lexdump (f)
  FILE *f;
{
  int i, j;

  for (i = 0; i < 256; i++)
    {
      (void) putc (lexdecode[i][0], f);
      (void) putc (lexdecode[i][1], f);
    }
  for (i = 0; i < 256; i++)
    (void) putc ((int) lexchar[i], f);

  for (i = 0; i < lexletters; i++)
    for (j = 0; j < lexletters; j++)
      (void) putc ((int) lextable[i][j], f);
}


void
lexalloc (NOARGS)
{
  int i;
  char *p;


  lexdecode = (char **) xmalloc (256 * sizeof (char *));
  p = (char *) xcalloc (256, 3);

  for (i = 0; i < 256; i++)
    {
      lexdecode[i] = p;
      p += 3;
    }

  lexchar = (unsigned char *) xcalloc (256, 1);
  lextable = (unsigned char **) xmalloc (lexletters * sizeof (char *));

  for (i = 0; i < lexletters; i++)
    lextable[i] = (unsigned char *) xcalloc ((unsigned) lexletters, 1);
}


void
lexload (f, nlet)
  FILE *f;
  unsigned long nlet;
{
  int i, j;
  unsigned char *up;
  char *p;

  lexletters = (int) nlet;
  lexalloc ();
  for (i = 0; i < 256; i++)
    {
      p = lexdecode[i];
      *p++ = getc (f);
      *p++ = getc (f);
      *p = 0;
    }

  for (i = 0, up = lexchar; i < 256; i++, up++)
    *up = getc (f);

  for (i = 0; i < lexletters; i++)
    {
      for (j = 0; j < lexletters; j++)
	lextable[i][j] = getc (f);
    }
}
