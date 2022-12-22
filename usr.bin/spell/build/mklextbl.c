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
#include "charset.h"

int lexletters;
unsigned char **lextable;
unsigned char *lexchar;
char **lexdecode;

extern char *freqletters;

void
mklextbl (name, outf)
  char *name;
  FILE *outf;
{
  FILE *f;
  int i, j;
  char buf[300], *p, *q;
  int code;

  if ((f = fopen (name, "r")) == NULL)
    {
      (void) fprintf (stderr, "can't open %s\n", name);
      exit (1);
    }

  if (fgets (buf, sizeof buf, f) == NULL)
    goto badfreq;

  if ((p = (char *) strchr (buf, ' ')) == NULL)
    goto badfreq;
  p++;
  if ((q = (char *) strchr (buf, '\n')) != NULL)
    *q = 0;

  if ((freqletters = (char *) xmalloc (strlen (p) + 1)) == NULL)
    {
      fprintf (stderr, "out of memory for freqletters\n");
      exit (1);
    }

  strcpy (freqletters, p);

  lexletters = strlen (freqletters) + 1;

  lexalloc ();

  code = 1;
  for (p = freqletters; *p && *p != '\n'; p++)
    {
      lexchar[*p & 0xff] = code;
      q = lexdecode[code];
      *q++ = *p;
      *q = 0;
      code++;
    }

  for (i = 0; i < 256; i++)
    {
      if (lexchar[charset[i].lowercase])
	lexchar[i] = lexchar[charset[i].lowercase];
    }

  while (code < 255 && fgets (buf, sizeof buf, f) != NULL)
    {
      p = buf;
      while (isspace (*p))
	p++;
      while (*p && !isspace (*p))
	p++;
      while (isspace (*p))
	p++;
      q = lexdecode[code];
      *q++ = *p++;
      *q++ = *p;
      *q = 0;
      code++;
    }
  (void) fclose (f);

  if (code != 255)
    {
      (void) fprintf (stderr,
		      "freqfile '%s' does not have enough entries\n", name);
      exit (1);
    }

  for (i = 0; i < 256; i++)
    {
      p = lexdecode[i];
      if (p[1])
	lextable[lexchar[p[0]]][lexchar[p[1]]] = i;
    }

  for (i = 0; i < lexletters; i++)
    {
      lextable[i][0] = 255;
      lextable[0][i] = 255;
    }

  if (outf == NULL)
    return;

  /* the following is old stuff when the lex table was written
	 * to a c file, and then compiled into other programs.  It's
	 * still here because it is sometimes helpful when debugging.
	 */
  (void) fprintf (outf, "char *lexdecode[256] = {");
  for (i = 0; i < 256; i++)
    {
      if (i % 8 == 0)
	(void) fprintf (outf, "\n/*%3d*/\t", i);
      switch (strlen (lexdecode[i]))
	{
	case 0:
	  (void) fprintf (outf, "  ");
	  break;
	case 1:
	  (void) fprintf (outf, " ");
	  break;
	}
      (void) fprintf (outf, "\"%s\", ", lexdecode[i]);
    }
  (void) fprintf (outf, "\n};\n");
  (void) fprintf (outf, "\n");

  (void) fprintf (outf, "unsigned char lexchar[256] = {");
  for (i = 0; i < 256; i++)
    {
      if (i % 8 == 0)
	(void) fprintf (outf, "\n\t");
      (void) fprintf (outf, "%3d, ", lexchar[i]);
    }
  (void) fprintf (outf, "\n};\n");
  (void) fprintf (outf, "\n");

  (void) fprintf (outf, "unsigned char lextable[%d][%d] = {\n",
		  lexletters, lexletters);
  for (i = 0; i < lexletters; i++)
    {
      (void) fprintf (outf, "\t{ ");
      for (j = 0; j < lexletters; j++)
	{
	  (void) fprintf (outf, "%3d, ", lextable[i][j]);
	  if (j == 10 || j == 21)
	    (void) fprintf (outf, "\n\t  ");
	}
      (void) fprintf (outf, "},\n");
    }
  (void) fprintf (outf, "};\n");

  (void) fclose (outf);
  return;
badfreq:
  fprintf (stderr, "bad freq table\n");
  exit (1);
}

#ifdef TEST
main (argc, argv)
     char **argv;
{
  mklextbl (argv[1], stdout);
}

#endif
