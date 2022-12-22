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

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "ispell.h"
#include "hash.h"

static int p_nextsize = 53;	/* first hash size, should be prime */
static char **p_hash, **p_end;
static int p_size, p_used, p_thresh;
static int p_modified;

int
p_lookup (word, alllower)
  char *word;
  int alllower;
{
  int code;
  char **pe;
  char local[MAX_WORD_LEN], *p, *q;

  if (p_size == 0)
    return (0);

  if (alllower)
    {
      p = word;			/* already lower case */
    }
  else
    {
      downcase (local, word);
      p = local;
    }

  code = hash (p) % p_size;
  pe = p_hash + code;
  while ((q = *pe))
    {
      if (*q == '!')
	q++;
      if (strcmp (q, p) == 0)
	return (1);
      pe++;
      if (pe == p_end)
	pe = p_hash;
    }
  return (0);
}

int
p_delete (word)
  char *word;
{
  int code;
  char **pe;
  char local[MAX_WORD_LEN], *p, *q;

  if (p_size == 0)
    return (-1);

  downcase (local, word);
  p = local;

  code = hash (p) % p_size;
  pe = p_hash + code;
  while ((q = *pe))
    {
      if (*q == '!')
	q++;
      if (strcmp (q, p) == 0)
	{
	  (*pe)[0] = '#';
	  (*pe)[1] = 0;
	  p_modified = 1;
	  return (0);
	}
      pe++;
      if (pe == p_end)
	pe = p_hash;
    }
  return (-1);
}

int
p_enter (word, copy, keep)
  char *word;
  int copy, keep;
{
  int newsize;
  char **oldp_hash, **pe, *p, *q;
  int oldp_used, oldp_size;
  int code, i;

  if (*word == 0)
    return (-1);

  if (p_used + 1 >= p_thresh)
    {
      newsize = p_nextsize;
      if (newsize < 0 || newsize < p_size)
	return (-1);

      oldp_hash = p_hash;
      oldp_used = p_used;
      oldp_size = p_size;

      p_hash = (char **) xcalloc ((unsigned) newsize, sizeof (char *));
      p_used = 0;
      p_size = newsize;
      p_thresh = p_size / 2;
      p_end = p_hash + p_size;
      for (i = 0, pe = oldp_hash; i < oldp_size; i++, pe++)
	{
	  if (*pe)
	    {
	      if (p_enter (*pe, 0, 1) < 0)
		{
		  p_hash = oldp_hash;
		  p_used = oldp_used;
		  p_size = oldp_size;
		  p_thresh = p_size / 2;
		  p_end = p_hash + p_size;
		  return (-1);
		}
	    }
	}
      if (oldp_hash)
	free ((char *) oldp_hash);
      p_nextsize = nextprime ((unsigned short) (newsize * 2));
    }
  if (word[0] == '!')
    {
      p = word;
      code = hash (p + 1);
    }
  else if (keep)
    {
      if (copy)
	{
	  p = (char *) xmalloc ((unsigned) (strlen (word) + 1));

	  if (copy == 1)
	    downcase (p, word);
	  else
	    (void) strcpy (p, word);
	}
      else
	{
	  p = word;
	}
      code = hash (p);
    }
  else
    {
      p = (char *) xmalloc ((unsigned) (strlen (word) + 2));
      p[0] = '!';
      downcase (p + 1, word);
      code = hash (p + 1);
    }
  /* make sure it is not already there */
  pe = p_hash + (code % p_size);
  while ((q = *pe))
    {
      if (*q == '!')
	{
	  if (strcmp (q + 1, p) == 0)
	    {
	      /* he did 'A', now 'I', skip the '!' */
	      (*pe)++;
	      return (0);
	    }
	}
      else if (strcmp (q, p) == 0)
	{
	  return (0);
	}
      pe++;
      if (pe == p_end)
	pe = p_hash;
    }
  *pe = p;
  p_used++;
  if (keep)
    p_modified = 1;
  addchars (p);
  return (0);
}


int
p_load (file, keep)
  char *file;
  int keep;
{
  FILE *f;
  int nwords;
  int c;
  char buf[MAX_WORD_LEN], *p, *start;

  int omodified;

  omodified = p_modified;

  if ((f = fopen (file, "r")) == NULL)
    return (-1);

  nwords = 0;

  while ((c = getc (f)) != EOF)
    if (c == '\n')
      nwords++;


  p_nextsize += nextprime ((unsigned) ((nwords + 20) * 2));

  rewind (f);
  while (fgets (buf, sizeof buf, f) != NULL)
    {
      if (buf[0] == '#')
	continue;
      downcase (buf, buf);
      start = buf;
      while (isspace (*start))
	start++;
      for (p = start; *p && !isspace (*p); p++)
	;
      *p = 0;
      if (*start == 0)
	continue;
      addchars (start);
      if (p_enter (start, 1, keep) < 0)
	{
	  (void) fclose (f);
	  return (-1);
	}
    }
  (void) fclose (f);
  p_modified = omodified;
  return (0);
}


void
addchars (word)
  char *word;
{
  char *p;
  int c;

  for (p = word; *p; p++)
    {
      /* characters above 128 and certain ascii
		 * characters can be promoted to be LEXLETTERS
		 * if they appear in the ispell.words file
		 */
      c = *(unsigned char *) p;
      if (c >= 128 || strchr ("$*+-/_~", c) != NULL)
	{
	  (ctbl + 1)[c] |= LEXLETTER;
	  if (near_map[c] == 0)
	    {
	      near_map[c] = 1;
	      near_miss_letters[nnear_miss_letters++]
		= c;
	    }
	}
    }
}


int
p_dump (file)
  char *file;
{
  FILE *f;
  char **pe;
  char *bakname;

  if (p_size == 0 || p_modified == 0)
    return (0);

  bakname = (char *) xmalloc ((unsigned) (strlen (file) + 50));

  (void) strcpy (bakname, file);
  (void) strcat (bakname, ".bak");
  (void) unlink (bakname);
  if (link (file, bakname) >= 0)
    (void) unlink (file);

  if ((f = fopen (file, "w")) == NULL)
    {
      free (bakname);
      return (-1);
    }

  pe = p_hash;

  while (pe != p_end)
    {
      if (*pe && **pe != '!' && **pe != '#')
	{
	  fputs (*pe, f);
	  putc ('\n', f);
	}
      pe++;
    }

  if (ferror (f))
    {
      (void) fclose (f);
      (void) unlink (file);
      (void) link (bakname, file);
      (void) unlink (bakname);
      free (bakname);
      return (-1);
    }

  (void) fclose (f);
  p_modified = 0;
  (void) unlink (bakname);
  free (bakname);
  return (0);
}


int
p_reload ()
{
  char **pe;
  extern char *privname;

  if (privname == (char *) NULL)
    return (-1);

  if (p_size)
    {
      pe = p_hash;
      while (pe != p_end)
	{
	  if (*pe)
	    free (*pe);
	  pe++;
	}
      free ((char *) p_hash);
    }
  p_nextsize = 53;
  p_hash = (char **) NULL;
  p_end = (char **) NULL;
  p_size = 0;
  p_used = 0;
  p_thresh = 0;
  p_modified = 0;
  return (p_load (privname, 1));
}

#if 0
main ()
{
  char buf[100];
  char *p;
  if (p_load ("testpriv", 1) < 0)
    (void) printf ("couldn't load testpriv\n");

  while (gets (buf) != NULL)
    {
      switch (buf[0])
	{
	case 'l':
	  p = buf + 2;
	  if (p_lookup (p, 0))
	    (void) printf ("%s: ok\n", p);
	  else
	    (void) printf ("%s: not found\n", p);
	  break;
	case 'i':
	case 'a':
	  p = buf + 2;
	  while (*p)
	    {
	      if (isupper (*p))
		*p = _tolower (*p);
	      p++;
	    }
	  p = buf + 2;
	  if (p_enter (p, 1, buf[0] == 'i' ? 1 : 0) < 0)
	    (void) printf ("enter failed\n");
	  else
	    (void) printf ("ok\n");
	  break;
	default:
	  (void) printf ("unknown cmd %s\n", buf);
	  break;
	}
    }

  if (p_dump ("testpriv") < 0)
    (void) printf ("dump failed\n");

}

#endif
