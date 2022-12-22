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
#include "build.h"
#include "getopt.h"

int print = 0;

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif


unsigned char *tmp_hused, *hused;
#define BITVECSIZE 8192		/* 64K bits */
unsigned short hused_limit;

char inbuf[100];
comp_char outbuf[100];
struct hash_table_entry hte;

#ifdef __STDC__

unsigned short parse_flags (char *, char *);
hash_index find_slot (int);
void toomanywords (void);
void mklextbl (char *, FILE *);
void hash_write (FILE *);
void hash_awrite (FILE *);
void hash_ewrite (FILE *);
int lexword (char *, int, unsigned char *);
void set_bitvec (unsigned char *, int);

#else

extern unsigned short parse_flags ();
extern hash_index find_slot ();
extern void toomanywords ();
extern void hash_write ();
extern void hash_awrite ();
extern void hash_ewrite ();
extern int lexword ();

#endif

int binary_flag = 0;
int ascii_flag = 0;
int reap_flag = 0;
int debug_flag = 0;

char buildout[100];

/* this array contains letters to use when generating near misses */
char near_miss_letters[256];
int nnear_miss_letters;

/* this array has 1 for any character that is in near_miss_letters */
char near_map[256];

int
main (argc, argv)
  int argc;
  char **argv;
{
  FILE *in;
#ifdef __STDC__
  void reapall (void);
#else
  extern void reapall ();
#endif
  int c;
  extern char *optarg;
  extern int optind;
  char *name;
  char *freqfile = NULL;

  buildout[0] = 0;

  while ((c = getopt (argc, argv, "prbao:f:d")) != EOF)
    {
      switch (c)
	{
	case 'd':
	  debug_flag = 1;
	  break;
	case 'f':
	  freqfile = optarg;
	  break;
	case 'p':
	  print = 1;
	  break;
	case 'r':
	  reap_flag = 1;
	  break;
	case 'b':
	  binary_flag = 1;
	  break;
	case 'a':
	  ascii_flag = 1;
	  break;
	case 'o':
	  (void) strcpy (buildout, optarg);
	  break;
	default:
	  /* message printed by getopt */
	  exit (1);
	}
    }

  if (freqfile == NULL)
    {
      (void) fprintf (stderr, "no freq file specified\n");
      exit (1);
    }
  name = argv[optind];

  if (binary_flag == 0 && ascii_flag == 0)
    {
      (void) fprintf (stderr, "must have either -a or -b\n");
      exit (1);
    }

  if (debug_flag)
    {
      mklextbl (freqfile, stdout);
      exit (0);
    }

  mklextbl (freqfile, (FILE *) NULL);

  if ((in = fopen (name, "r")) == NULL)
    {
      (void) fprintf (stderr, "can't open %s\n", name);
      exit (1);
    }

  build (in);

  (void) fclose (in);

  if (reap_flag)
    reapall ();

  if (binary_flag)
    {
      if (buildout[0] == 0)
	{
	  strcpy (buildout, name);
	  strcat (buildout, ".hsh");
	}
      write_binary (buildout);
    }

  if (ascii_flag)
    {
      if (buildout[0] == 0)
	{
	  strcpy (buildout, name);
	  strcat (buildout, ".f");
	}
      write_ascii (buildout);
    }
  return (0);
}


void
write_binary (name)
  char *name;
{
  FILE *out;
  char *mode;

#ifdef MSDOS
  mode = "wb";
#else
  mode = "w";
#endif
  if ((out = fopen (name, mode)) == NULL)
    {
      (void) fprintf (stderr, "can't create %s\n", name);
      exit (1);
    }
  hash_write (out);

  fclose (out);
}


void
write_ascii (name)
  char *name;
{
  FILE *out;

  if ((out = fopen (name, "w")) == NULL)
    {
      (void) fprintf (stderr, "can't create %s\n", name);
      exit (1);
    }

  hash_awrite (out);

  if (ferror (out))
    {
      (void) fprintf (stderr, "error writing %s\n", name);
      exit (1);
    }

  fclose (out);
}


void
build (in)
  FILE *in;
{
  int len;
  struct hash_table_entry *htep;
  unsigned short nwords = 0;
  int i;
  hash_index h, prev_hindex;
  char *p;
  unsigned short flags;
  unsigned short strsize = 0;
#ifdef __STDC__
  unsigned short nextprime (unsigned short);
#else
  extern unsigned short nextprime ();
#endif

  tmp_hused = (unsigned char *) xcalloc (1, BITVECSIZE);

  (void) fprintf (stderr, "Counting words: ");
  while (fgets (inbuf, (int) sizeof inbuf, in) != NULL)
    {
      if (inbuf[0] == '#')
	continue;
      p = (char *) strchr (inbuf, '/');
      if (p)
	*p = 0;
      p = (char *) strchr (inbuf, '\n');
      if (p)
	*p = 0;
      if (inbuf[0] == 0)
	continue;
      if (strlen (inbuf) >= MAX_WORD_LEN)
	{
	  (void) fprintf (stderr, "warning: %s too long\n", inbuf);
	  continue;
	}
      len = lexword (inbuf, strlen (inbuf), outbuf);
      if (len == 0)
	{
	  (void) fprintf (stderr,
			  "%s can't be compressed; try re-running 'freq'\n",
			  inbuf);
	  continue;
	}
      if (nwords >= HASH_SPECIAL - 1)
	toomanywords ();
      if ((nwords % 1000) == 0)
	{
	  (void) fprintf (stderr, "%u ", nwords);
	  (void) fflush (stderr);
	}
      nwords++;
      if (len > sizeof hte.u.s.data)
	strsize += len - sizeof hte.u.l.data;
      h = hash ((char *) outbuf);
      set_bitvec (tmp_hused, h);
    }
  nwords++;			/* add one since slot 0 is reserved */

  (void) fprintf (stderr, "%u words\n", nwords);
  hashsize = nextprime (nwords);
  if (hashsize >= HASH_SPECIAL)
    toomanywords ();

  hused_limit = (hashsize + 7) >> 3;
  hused = (unsigned char *) xcalloc (1, hused_limit);

  /* make the slots that were introduced when rounding up
	 * by 8 be busy
	 */
  for (h = hashsize; h < hused_limit * 8; h++)
    set_bitvec (hused, h);
  for (i = 0, h = 0; i < BITVECSIZE; i++)
    {
      unsigned char x = tmp_hused[i];
      unsigned short bit;

      for (bit = 1; bit != 0x100; bit <<= 1, h++)
	{
	  if (x & bit)
	    {
	      hash_index real_hash = h % hashsize;
	      set_bitvec (hused, real_hash);
	    }
	}
    }
  /* now hused has 1 where ever something will hash to */

  free ((char *) tmp_hused);
  tmp_hused = NULL;

  alloc_tables ((long) hashsize, (long) strsize);

  set_bitvec (hused, 0);
  htep = (struct hash_table_entry *) xcalloc (1, sizeof *htep);
  htep->next = HASH_END;
  hash_store (0, htep);
  free ((char *) htep);

  rewind (in);

  (void) fprintf (stderr, "Starting hash: ");
  nwords = 0;
  while (fgets (inbuf, (int) sizeof inbuf, in) != NULL)
    {
      if ((nwords % 1000) == 0)
	{
	  (void) fprintf (stderr, "%u ", nwords);
	  (void) fflush (stderr);
	}
      nwords++;
      flags = 0;
      p = (char *) strchr (inbuf, '/');
      if (p)
	{
	  flags = parse_flags (inbuf, p);
	  *p = 0;
	}
      p = (char *) strchr (inbuf, '\n');
      if (p)
	*p = 0;
      if (inbuf[0] == 0)
	continue;
      if (strlen (inbuf) >= MAX_WORD_LEN)
	continue;
      len = lexword (inbuf, strlen (inbuf), outbuf);
      if (len == 0)
	continue;
      h = hash ((char *) outbuf) % hashsize;
      if (print)
	(void) printf ("%s hash %u; ", inbuf, h);
      if (!hash_emptyp (h))
	{
	  if (print)
	    (void) printf ("already used ");
	  prev_hindex = hash_find_end (h);
	  if (print)
	    (void) printf ("tail %u; ", prev_hindex);
	  h = find_slot (prev_hindex);
	  if (print)
	    (void) printf ("new slot %u; ", h);
	  hash_set_next (prev_hindex, h);
	}
      htep = make_hash_table_entry (outbuf, len);
      hash_store (h, htep);
      store_flags (h, flags);
      if (print)
	(void) printf ("\n");
    }
  (void) printf ("\n");
  free ((char *) hused);
}

void
toomanywords ()
{
  (void) fprintf (stderr, "too many words, max = %u\n", HASH_SPECIAL - 1);
  exit (1);
}

#if 0
get_bitvec (bitvec, n)
     unsigned char *bitvec;
     unsigned short n;
{
  static unsigned char bittbl[] =
  {1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80};

  return (bitvec[n >> 3] & bittbl[n & 7]);
}

#endif

/*
 * this can return free slot that will not be used as the start of a
 * hash chain.  It tries to find a slot on the same page,
 * but I've not measured it to see if that helps.
 */
hash_index
find_slot (hindex)
  hash_index hindex;
{
  int start;
  int i;
  int x, bit, n;

  if (hindex >= hashsize)
    {
      (void) fprintf (stderr, "bad index to find_slot %x %x\n",
		      hindex, hashsize);
      exit (1);
    }
  /* round down to beginning of page: 128 entries per 1K page */
  start = (hindex & ~127) >> 3;
  i = start;
  while (1)
    {
      x = hused[i];
      if (x != 0xff)
	{
	  for (bit = 1, n = 0; bit < 0x100; bit <<= 1, n++)
	    {
	      if ((x & bit) == 0)
		{
		  hused[i] |= bit;
		  hindex = i * 8 + n;
		  if (hindex >= hashsize)
		    {
		      (void) fprintf (stderr,
				      "find_slot r bad %x\n",
				      hindex);
		      exit (1);
		    }
		  return (hindex);
		}
	    }
	  (void) fprintf (stderr, "hash table error\n");
	  exit (1);
	}
      i++;
      if (i >= hused_limit)
	i = 0;
      if (i == start)
	{
	  (void) fprintf (stderr, "hused table overflow??\n");
	  exit (1);
	}
    }
  /* NOTREACHED */
}

struct hash_table_entry *
make_hash_table_entry (string, len)
  comp_char *string;
  int len;
{
  static struct hash_table_entry e;
  comp_char *p;
  int i;
  e.next = HASH_END;
  if (len <= sizeof e.u.s.data)
    {
      (void) strncpy ((char *) e.u.s.data,
		      (char *) string,
		      (int) sizeof e.u.s.data);
      return (&e);
    }
  else
    {
      e.u.l.short_flag = 0;
      e.u.l.len = (unsigned char) len;
      p = e.u.l.data;
      for (i = 0; i < sizeof e.u.l.data; i++)
	*p++ = *string++;
      e.u.l.sindex =
	copy_out_string (string, len - sizeof e.u.l.data);
      return (&e);
    }
}

unsigned short
parse_flags (word, p)
  char *word;
  char *p;
{
  unsigned short flags = 0;
  int c;

  while (*p && *p != '\n')
    {
      if (*p != '/')
	goto error;
      c = *p++;
      if (isupper (c))
	c = tolower (c);

      switch (*p)
	{
	case 'v':
	  flags |= V_FLAG;
	  break;
	case 'n':
	  flags |= N_FLAG;
	  break;
	case 'x':
	  flags |= X_FLAG;
	  break;
	case 'h':
	  flags |= H_FLAG;
	  break;
	case 'y':
	  flags |= Y_FLAG;
	  break;
	case 'g':
	  flags |= G_FLAG;
	  break;
	case 'j':
	  flags |= J_FLAG;
	  break;
	case 'd':
	  flags |= D_FLAG;
	  break;
	case 't':
	  flags |= T_FLAG;
	  break;
	case 'r':
	  flags |= R_FLAG;
	  break;
	case 'z':
	  flags |= Z_FLAG;
	  break;
	case 's':
	  flags |= S_FLAG;
	  break;
	case 'p':
	  flags |= P_FLAG;
	  break;
	case 'm':
	  flags |= M_FLAG;
	  break;
	default:
	  goto error;
	}
      p++;
    }
  return (flags);

error:
  (void) fprintf (stderr, "Syntax error: %s\n", word);
  return (flags);
}
