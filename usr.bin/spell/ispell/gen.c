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
#include <setjmp.h>
#include "ispell.h"
#include "hash.h"
#include "good.h"

extern int intr_typed;

char newword[MAX_WORD_LEN];

jmp_buf genjmp;

#ifdef __STDC__

void wrongletter (char *);
void extraletter (char *);
void missingletter (char *);
void transposedletter (char *);

#else

void wrongletter ();
void extraletter ();
void missingletter ();
void transposedletter ();

#endif

struct sp_corrections corrections;

int
makepossibilities (word)
  char *word;
{
  int i;
  char uword[MAX_WORD_LEN];

  corrections.nwords = 0;

  if (strlen (word) > MAX_WORD_LEN - 5)
    return (0);

  downcase (uword, word);

  if (setjmp (genjmp))
    goto done;

  for (i = 0; i < MAXPOS; i++)
    corrections.posbuf[i][0] = 0;

  wrongletter (uword);
  extraletter (uword);
  missingletter (uword);
  transposedletter (uword);

done:;
  fixcase (word, &corrections);
  if (intr_typed)
    return (1);

  return (0);
}

void
insert (word)
     char *word;
{
  int i, n;

  n = corrections.nwords;
  for (i = 0; i < n; i++)
    if (strcmp (corrections.posbuf[i], word) == 0)
      return;

  (void) strcpy (corrections.posbuf[n++], word);

  corrections.nwords = n;

  if (n >= 10)
    longjmp (genjmp, 1);
}

void
wrongletter (word)
  char *word;
{
  int i, len;
  int j;

  if (intr_typed)
    return;

  len = strlen (word);
  (void) strcpy (newword, word);

  for (i = 0; i < len; i++)
    {
      if (intr_typed)
	return;
      for (j = 0; j < nnear_miss_letters; j++)
	{
	  newword[i] = near_miss_letters[j];
	  if (good (newword, len, 1))
	    insert (newword);
	}
      newword[i] = word[i];
    }
}

void
extraletter (word)
  char *word;
{
  char *p, *s, *t;
  int len;

  if (intr_typed)
    return;

  len = strlen (word);

  if (len <= 2)
    return;

  for (p = word; *p; p++)
    {
      if (intr_typed)
	return;
      for (s = word, t = newword; *s; s++)
	if (s != p)
	  *t++ = *s;
      *t = 0;
      if (good (newword, len - 1, 1))
	insert (newword);
    }
}

void
missingletter (word)
  char *word;
{
  char *p, *r, *s, *t;
  int len;
  int i;

  if (intr_typed)
    return;

  len = strlen (word);

  for (p = word; p == word || p[-1]; p++)
    {
      if (intr_typed)
	return;
      for (s = newword, t = word; t != p; s++, t++)
	*s = *t;
      r = s++;
      while (*t)
	*s++ = *t++;
      *s = 0;

      for (i = 0; i < nnear_miss_letters; i++)
	{
	  *r = near_miss_letters[i];
	  if (good (newword, len + 1, 1))
	    insert (newword);
	}
    }
}

void
transposedletter (word)
  char *word;
{
  char t;
  char *p;
  int len;

  if (intr_typed)
    return;

  len = strlen (word);

  (void) strcpy (newword, word);
  for (p = newword; p[1]; p++)
    {
      if (intr_typed)
	return;
      t = p[0];
      p[0] = p[1];
      p[1] = t;
      if (good (newword, len, 1))
	insert (newword);
      t = p[0];
      p[0] = p[1];
      p[1] = t;
    }
}
