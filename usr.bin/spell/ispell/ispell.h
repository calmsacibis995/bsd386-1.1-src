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

#define VERSION_STRING version
extern char version[];

#ifdef NOARGS
#undef NOARGS
#endif

#ifdef __STDC__
#define NOARGS void
#else
#define NOARGS
#endif

#define MAXPOS 10
#define MAX_WORD_LEN 40

typedef char posbuf[MAXPOS][MAX_WORD_LEN];

struct sp_corrections
{
  int nwords;
  int mandantory;
  posbuf posbuf;
};

enum formatter
{
  formatter_generic, formatter_troff, formatter_tex
};

extern enum formatter formatter;
extern char **lexdecode;
extern char near_miss_letters[];
extern int nnear_miss_letters;
extern char near_map[];
extern int interaction_flag;

#if defined(USG) || defined (STDC_HEADERS)
#include <string.h>
#define index strchr
#define rindex strrchr
#define bcopy(s, d, n)  (memcpy ((d), (s), (n)))
#define bcmp(s1, s2, n)  (memcmp ((s1), (s2), (n)))
#define bzero(d, n)  (memset ((d), 0, (n)))
#else
#include <strings.h>
#endif

#ifdef __STDC__

void *xmalloc (int);
void *xcalloc (int, int);

void usage (void);
RETSIGTYPE intr (void);
void done (void);
void submode (void);
int subcmd (char *);

int cmd_insert (char *);
int cmd_accept (char *);
int cmd_delete (char *);
int cmd_dump (char *);
int cmd_reload (char *);
int cmd_file (char *);
int cmd_tex (char *);
int cmd_troff (char *);
int cmd_generic (char *);

void signon (void);

void dofile (char *);
void terminit (void);
void termuninit (void);
int p_load (char *, int);
int p_dump (char *);
int p_enter (char *, int, int);
int p_delete (char *);
int p_reload (void);
int p_lookup (char *, int);
void prhashchain (void);
void hash_write (FILE *);
void hash_awrite (FILE *);
void hash_ewrite (FILE *);
void askmode (int);
void spellmode (int, char **, int);
void checkfile (FILE *, FILE *, long);
int makepossibilities (char *);
int good (char *, int, int);
void downcase (char *, char *);
void fixcase (char *word, struct sp_corrections *c);
void addchars (char *);
void checkfile (FILE *, FILE *, long);
int skip_to_next_word (FILE *);
int correct (char*, unsigned int, char *, char *, char **);
int dochild (void (*) (NOARGS));
int skip_to_next_word_troff (void);
int skip_to_next_word_tex (FILE *);
int skip_to_next_word_generic (void);
void lexalloc (void);
int lexword (char *, int, unsigned char *);
unsigned short nextprime (unsigned short);

void inverse (void);
void normal (void);
void move (int, int);
void stop (void);
void termbeep (void);
void shellescape (char *);
void inserttoken (char *, char *, char *, char *, char **);
void termflush (void);
void dolook_interactive (char *);
void backup (void);
void erase (void);
void termreinit (void);

#else /* ! __STDC__ */

extern void *xmalloc ();
extern void *xcalloc ();

extern void dofile ();
extern void terminit ();
extern void termuninit ();
extern void prhashchain ();
extern void hash_write ();
extern void hash_awrite ();
extern void hash_ewrite ();
extern void askmode ();
extern void spellmode ();
extern void checkfile ();
extern void downcase ();
extern void fixcase ();
extern void addchars ();
extern void checkfile ();
extern void lexalloc ();

extern void inverse ();
extern void normal ();
extern void move ();
extern void stop ();
extern void termbeep ();
extern void shellescape ();
extern void inserttoken ();
extern void termflush ();
extern void dolook_interactive ();
extern void backup ();
extern void erase ();
extern void termreinit ();
extern void submode ();

#endif /* ! __STDC__ */
