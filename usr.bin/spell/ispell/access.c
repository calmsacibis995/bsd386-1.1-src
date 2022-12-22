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
#include "hash.h"
#include "build.h"
#include "good.h"

#ifdef __STDC__

void lexload (FILE *, unsigned long);
int lexsize (void);
void set_bitvec (unsigned char *, int);
void lexdump (FILE *);

#else

extern void lexload ();
extern int lexsize ();
extern void lexdump ();
extern void set_bitvec ();

#endif


hash_index hashsize;

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

/* there are 3 tables:
 *
 * all are addressed by unsigned shorts
 *  htbl  hashsize hash table entries
 *  ftbl  hashsize flag entries
 *  stbl  <= 64K bytes of strings
 */
#if iAPX286 && (LARGE_M || HUGE_M)
#define SMALL
#endif
#ifdef SMALL
#define MAXMALLOC 32768

struct hash_table_entry **htbls;
/* number of whole hash_table_entries that fit in a table */
#define HTE_PER_TABLE ((MAXMALLOC-1) / sizeof (struct hash_table_entry))
#if 0
struct hash_table_entry *
find_hte (h)
     hash_index h;
{
  unsigned short tbl;
  unsigned short off;
  tbl = h / HTE_PER_TABLE;
  off = h % HTE_PER_TABLE;

  return (htbls[tbl] + off);
}

#endif
#define H_TO_HTE(h) (htbls[(h)/HTE_PER_TABLE] + ((h)%HTE_PER_TABLE))

unsigned short **ftbls;
#define FLAGS_PER_TABLE ((MAXMALLOC-1) / sizeof (unsigned short))
#if 0
unsigned short *
find_flags (h)
     hash_index h;
{
  unsigned short tbl;
  unsigned short off;
  tbl = h / FLAGS_PER_TABLE;
  off = h % FLAGS_PER_TABLE;
  return (ftbls[tbl] + off);
}

#endif
#define H_TO_FLAGS(h) (ftbls[(h)/FLAGS_PER_TABLE] + ((h)%FLAGS_PER_TABLE))

comp_char **stbls;
#define BYTES_PER_TABLE MAXMALLOC
#if 0
comp_char *
find_string (index)
     str_index index;
{
  unsigned short tbl;
  unsigned short off;
  tbl = index / BYTES_PER_TABLE;
  off = index % BYTES_PER_TABLE;
  return (stbls[tbl] + off);
}

#endif
#define INDEX_TO_STRING(index) (stbls[(index)/BYTES_PER_TABLE]+((index)%BYTES_PER_TABLE))
#else
#undef SMALL
struct hash_table_entry *htbl;
unsigned short *ftbl;
comp_char *stbl;

#define H_TO_HTE(h) (htbl + (h))
#define H_TO_FLAGS(h) (ftbl + (h))
#define INDEX_TO_STRING(index) (stbl + (index))
#endif

#define HASH_NEXT(h) (H_TO_HTE(h)->next)
#define HASH_EMPTY_P(h) (H_TO_HTE(h)->next == HASH_EMPTY)

struct hash_table_entry proto_hte;

static str_index stbl_size;
static str_index stbl_index;

char *
alloc (n)
  unsigned int n;
{
  char *p;
  p = (char *) xcalloc (n, 1);
  return (p);
}

void
alloc_tables (hsize, ssize)
  unsigned long hsize, ssize;
{
  hash_index h;
#ifdef SMALL
  unsigned short u, x;
  int i;
#endif

  stbl_size = ssize;
  stbl_index = 0;

#ifdef SMALL
  u = (ssize + BYTES_PER_TABLE - 1) / BYTES_PER_TABLE;
  stbls = (comp_char **) alloc (u * sizeof (comp_char *));
  u = ssize;
  i = 0;
  while (u != 0)
    {
      x = min (u, BYTES_PER_TABLE);
      stbls[i] = (comp_char *) alloc (x);
      u -= x;
      i++;
    }
  u = (hsize + FLAGS_PER_TABLE - 1) / FLAGS_PER_TABLE;
  ftbls = (unsigned short **) alloc (u * sizeof (unsigned short *));
  u = hsize;
  i = 0;
  while (u != 0)
    {
      x = min (u, FLAGS_PER_TABLE);
      ftbls[i] = (unsigned short *) alloc (x * sizeof (unsigned short));
      u -= x;
      i++;
    }
  u = (hsize + HTE_PER_TABLE - 1) / HTE_PER_TABLE;
  htbls = (struct hash_table_entry **)
    alloc (u * sizeof (struct hash_table_entry *));
  u = hsize;
  i = 0;
  while (u != 0)
    {
      x = min (u, HTE_PER_TABLE);
      htbls[i] = (struct hash_table_entry *)
	alloc (x * sizeof (struct hash_table_entry));
      u -= x;
      i++;
    }
#else
  htbl = (struct hash_table_entry *)
    alloc ((unsigned) (hsize * sizeof proto_hte));
  ftbl = (unsigned short *)
    alloc ((unsigned) (hsize * sizeof (unsigned short)));
  stbl = (comp_char *) alloc ((unsigned) ssize);
#endif

  for (h = 0; h < hsize; h++)
    HASH_NEXT (h) = HASH_EMPTY;
}

str_index
copy_out_string (s, n)
  comp_char *s;
  int n;
{
  str_index index;
  comp_char *cp;

#ifdef SMALL
  unsigned short t1, t2;
  t1 = stbl_index / BYTES_PER_TABLE;
  t2 = (stbl_index + n) / BYTES_PER_TABLE;
  if (t1 != t2)
    stbl_index += BYTES_PER_TABLE - stbl_index % BYTES_PER_TABLE;
#endif
  if (stbl_index + n > stbl_size)
    {
      (void) fprintf (stderr, "string table overflow\n");
      exit (1);
    }

  index = stbl_index;
  stbl_index += n;

  cp = INDEX_TO_STRING (index);
  while (n--)
    *cp++ = *s++;

  return (index);
}

/* returns index of end of chain */
hash_index
hash_find_end (index)
     hash_index index;
{
  hash_index next, cur;

  for (cur = index, next = HASH_NEXT (index);
       next < HASH_SPECIAL;
       cur = next, next = HASH_NEXT (next))
    ;
  return (cur);
}

/* clobbers whatever is there */
void
hash_store (index, htep)
  hash_index index;
  struct hash_table_entry *htep;
{
  *H_TO_HTE (index) = *htep;
}

void
hash_retrieve (index, htep)
  hash_index index;
  struct hash_table_entry *htep;
{
  *htep = *H_TO_HTE (index);
}

/* updates the next field */
void
hash_set_next (index, new_next)
  hash_index index;
  hash_index new_next;
{
  if (new_next < HASH_SPECIAL && new_next >= hashsize)
    {
      (void) fprintf (stderr, "bad index to set_next %x\n", new_next);
      exit (1);
    }
  HASH_NEXT (index) = new_next;
}

int
hash_emptyp (h)
  hash_index h;
{
  return (HASH_EMPTY_P (h));
}

void
store_flags (hindex, flags)
     hash_index hindex;
     unsigned short flags;
{
  *H_TO_FLAGS (hindex) = flags;
}


int
hash_init (name)
  char *name;
{
  FILE *in;
  struct hash_header hhdr;
  int i, j, c, bit;
#ifdef SMALL
  hash_index h;
  unsigned short flags;
  unsigned short u;
  unsigned short x;
  struct hash_table_entry hte;
  long l;
#endif
#ifdef MSDOS
  if ((in = fopen (name, "rb")) == NULL)
    return (-1);
#else
  if ((in = fopen (name, "r")) == NULL)
    return (-1);
#endif
  if (fread ((char *) &hhdr, (int) sizeof hhdr, 1, in) != 1)
    goto bad;

  if (hhdr.magic != HMAGIC)
    goto bad;
  if (hhdr.version != VERSION)
    {
      (void) fprintf (stderr,
		      "ispell: wanted dict version %d, got %d\n",
		      VERSION, hhdr.version);
      goto bad;
    }

  for (i = 0; i < 32; i++)
    {
      for (j = 0, bit = 1; j < 8; j++, bit <<= 1)
	{
	  if (hhdr.used[i] & bit)
	    {
	      c = i * 8 + j;
	      if (near_map[c] == 0)
		near_miss_letters[nnear_miss_letters++]
		  = c;
	    }
	}
    }

  lexload (in, hhdr.lexletters);

  if (ftell (in) != sizeof hhdr + hhdr.lexsize)
    {
      (void) fprintf (stderr, "bad dictionary file\n");
      exit (1);
    }

  hashsize = hhdr.hashsize;
  alloc_tables (hhdr.hashsize, hhdr.stblsize);

#ifdef SMALL
  l = (long) hashsize *sizeof hte;
  i = 0;
  while (l != 0)
    {
      x = min (l, HTE_PER_TABLE * sizeof hte);
      if (fread ((char *) htbls[i], 1, (int) x, in) != (int) x)
	goto bad;
      l -= x;
      i++;
    }
  l = (long) hashsize *sizeof flags;
  i = 0;
  while (l != 0)
    {
      x = min (l, FLAGS_PER_TABLE * sizeof flags);
      if (fread ((char *) ftbls[i], 1, (int) x, in) != (int) x)
	goto bad;
      l -= x;
      i++;
    }
  u = stbl_size;
  i = 0;
  while (u != 0)
    {
      x = min (u, BYTES_PER_TABLE);
      if (fread ((char *) stbls[i], 1, (int) x, in) != (int) x)
	goto bad;
      u -= x;
      i++;
    }
#else
  if (fread ((char *) htbl, (int) sizeof *htbl, (int) hashsize, in) !=
      (int) hashsize)
    goto bad;
  if (fread ((char *) ftbl, (int) sizeof *ftbl, (int) hashsize, in) !=
      (int) hashsize)
    goto bad;
  if (fread ((char *) stbl, 1, (int) stbl_size, in) != (int) stbl_size)
    goto bad;
#endif
  (void) fclose (in);
  return (0);
bad:
  (void) fclose (in);
  return (-1);
}

#define NHASHSTAT 100
long hashstat[NHASHSTAT];

void
dumphashstat (NOARGS)
{
  int i;

  for (i = 0; i < NHASHSTAT; i++)
    if (hashstat[i])
      (void) printf ("%d %ld\n", i, hashstat[i]);
}


int
hashchainlen (h)
  hash_index h;
{
  struct hash_table_entry *rh;
  int i = 0;

  for (h = h % hashsize;
       h < HASH_SPECIAL;
       h = rh->next)
    {
      rh = H_TO_HTE (h);
      i++;
    }
  if (i >= NHASHSTAT)
    i = NHASHSTAT - 1;
  return (i);
}


void
prhashchain ()
{
  hash_index h;
  int i;

  for (i = 0; i < NHASHSTAT; i++)
    hashstat[i] = 0;

  for (h = 0; h < hashsize; h++)
    hashstat[hashchainlen (h)]++;

  dumphashstat ();
}

/*
 * returns 0 for not found, otherwise, the index
 */

hash_index
hash_lookup (string, alen)
  comp_char *string;
  int alen;
{
  register int n;
  register comp_char *p;
  register comp_char *hp;
  register int len;
  register struct hash_table_entry *rh;
  register hash_index h;

  len = alen;

  if (len <= sizeof proto_hte.u.s.data)
    {
      for (h = hash ((char *) string) % hashsize;
	   h < HASH_SPECIAL;
	   h = rh->next)
	{
	  rh = H_TO_HTE (h);
	  /* should do if (rh->next == HASH_EMPTY) continue;
			 * here, but since we know that the rest of the
			 * entry will be zero, it can't match anyway, and
			 * we get out of the loop next time anyway */
	  if (!rh->u.l.short_flag)
	    continue;
	  hp = rh->u.s.data;
	  p = string;
	  for (n = len; n > 0; --n)
	    if (*p++ != *hp++)
	      goto fail1;
	  if (len < sizeof proto_hte.u.s.data && *hp != 0)
	    goto fail1;
	  return (h);
	fail1:;
	}
      return (0);
    }
  else
    {
      for (h = hash ((char *) string) % hashsize;
	   h < HASH_SPECIAL;
	   h = rh->next)
	{
	  rh = H_TO_HTE (h);
	  if (rh->u.l.short_flag)
	    continue;
	  if ((int) rh->u.l.len != len)
	    continue;
	  hp = rh->u.l.data;
	  p = string;
	  for (n = sizeof proto_hte.u.l.data; n > 0; --n)
	    if (*p++ != *hp++)
	      goto fail2;
	  hp = INDEX_TO_STRING (rh->u.l.sindex);
	  for (n = len - sizeof proto_hte.u.l.data; n > 0; --n)
	    if (*p++ != *hp++)
	      goto fail2;
	  return (h);
	fail2:;
	}
      return (0);
    }
}

unsigned short
get_flags (hindex)
     hash_index hindex;
{
  return (*H_TO_FLAGS (hindex));
}

unsigned char *freqletters;


void
hash_write (f)
  FILE *f;
{
  struct hash_header hhdr;
  extern int lexletters;
  int i;
  unsigned char *p;
#ifdef SMALL
  unsigned short h;
  unsigned short x;
  struct hash_table_entry hte;
  unsigned short flags;
#endif

  hhdr.magic = HMAGIC;
  hhdr.version = VERSION;
  hhdr.lexletters = lexletters;
  hhdr.lexsize = lexsize ();
  hhdr.hashsize = hashsize;
  hhdr.stblsize = stbl_index;

  for (i = 0; i < 32; i++)
    hhdr.used[i] = 0;

  for (p = freqletters; *p; p++)
    set_bitvec (hhdr.used, *p);

  if (fwrite ((char *) &hhdr, (int) sizeof hhdr, 1, f) != 1)
    goto bad;

  lexdump (f);

  if (ftell (f) != sizeof hhdr + hhdr.lexsize)
    {
      (void) fprintf (stderr, "lexdump used wrong number of bytes\n");
      exit (1);
    }

#ifdef SMALL
  for (h = 0; h < hashsize; h++)
    {
      hash_retrieve (h, &hte);
      if (fwrite ((char *) &hte, (int) sizeof hte, 1, f) != 1)
	goto bad;
    }
  for (h = 0; h < hashsize; h++)
    {
      flags = get_flags (h);
      if (fwrite ((char *) &flags, (int) sizeof (flags), 1, f) != 1)
	goto bad;
    }
  h = stbl_index;
  i = 0;
  while (h != 0)
    {
      x = min (h, BYTES_PER_TABLE);
      if (fwrite ((char *) stbls[i], (int) x, 1, f) != 1)
	goto bad;
      h -= x;
      i++;
    }
#else
  if (fwrite ((char *) htbl, (int) sizeof htbl[0], (int) hashsize, f) !=
      (int) hashsize)
    goto bad;
  if (fwrite ((char *) ftbl, (int) sizeof ftbl[0], (int) hashsize, f) !=
      (int) hashsize)
    goto bad;
  if (fwrite ((char *) stbl, 1, (int) stbl_index, f) != (int) stbl_index)
    goto bad;
#endif
  return;
bad:
  (void) fprintf (stderr, "error writing hash table\n");
  exit (1);
  /* NOTREACHED */
}


void
hash_awrite (out)
  FILE *out;
{
  struct hash_table_entry hte;
  hash_index h;
  char *entry;

  for (h = 1; h < hashsize; h++)
    {
      hash_retrieve (h, &hte);
      if (hte.next < HASH_SPECIAL ||
	  hte.next == HASH_END)
	{
	  entry = htetostr (&hte, h);
	  if (*entry && *entry != '/')
	    {
	      fputs (entry, out);
	      putc ('\n', out);
	    }
	}
    }
}


void
hash_ewrite (out)
  FILE *out;
{
  struct hash_table_entry hte;
  hash_index h;
  char *entry;
  int n, i;

  for (h = 1; h < hashsize; h++)
    {
      hash_retrieve (h, &hte);
      if (hte.next < HASH_SPECIAL ||
	  hte.next == HASH_END)
	{
	  entry = htetostr (&hte, h);
	  n = expand (entry);
	  for (i = 0; i < n; i++)
	    {
	      if (words[i] == (char *) NULL)
		break;
	      (void) fputs (words[i], out);
	      (void) putc ('\n', out);
	    }
	}
    }
}

char *
htetostr (htep, h)
  struct hash_table_entry *htep;
  hash_index h;
{
  comp_char *p;
  int i;

  int len;
  unsigned short bit, f;
  static char hstr[100];
  char *outp, *q;

  outp = hstr;

  if (htep->u.l.short_flag)
    {
      for (p = htep->u.s.data, i = 0;
	   *p && i < sizeof proto_hte.u.s.data;
	   p++, i++)
	{
	  q = lexdecode[*p];
	  while (*q)
	    *outp++ = *q++;
	}
    }
  else if (*(p = htep->u.l.data) && (len = htep->u.l.len))
    {
      for (i = 0;
	   i < sizeof proto_hte.u.l.data;
	   p++, i++)
	{
	  q = lexdecode[*p];
	  while (*q)
	    *outp++ = *q++;
	}
      len -= sizeof proto_hte.u.l.data;
      p = INDEX_TO_STRING (htep->u.l.sindex);
      for (i = 0; i < len; i++)
	{
	  q = lexdecode[*p];
	  while (*q)
	    *outp++ = *q++;
	  p++;
	}
    }
  f = get_flags (h);
  for (i = 0, bit = 1; i < 16; i++, bit <<= 1)
    {
      if (f & bit)
	{
	  *outp++ = '/';
	  *outp++ = flag_char (i);
	}
    }
  *outp = 0;
  return (hstr);
}

void
set_bitvec (bitvec, n)
  unsigned char *bitvec;
  unsigned short n;
{
  static unsigned char bittbl[] =
  {1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80};

  bitvec[n >> 3] |= bittbl[n & 7];
}
