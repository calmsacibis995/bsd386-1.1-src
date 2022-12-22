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
#include <signal.h>
#include "ispell.h"
#include "hash.h"

int reading_interactive_command;
jmp_buf command_loop;

extern struct sp_corrections corrections;

#ifdef __STDC__
static void copyout (char **, int, FILE *);
#else
static void copyout ();
#endif

extern int lflag;
extern int uflag;
extern int iflag;
extern FILE *sortf;
extern int Sflag;
extern int intr_typed;

char tempfile[100];

#define NOPARITY 0x7f

static void
giveihelp ()
{
  erase ();
  printf ("You have interrupted ISPELL.  Commands are:\r\n");
  printf ("\r\n");
  printf ("SPACE    Continue scanning the current file.\r\n");
  printf ("Q        Write changes so far, and ignore misspellings in\r\n");
  printf ("             the rest of the file.\r\n");
  printf ("X        Abandon changes to this file.\r\n");
  printf ("!       Shell escape.\r\n");
  printf ("^L      Redraw screen.\r\n");
  printf ("\r\n\r\n");
  printf ("-- Type space to continue --");
  fflush (stdout);
  getchar ();
}

static void
givehelp ()
{
  erase ();
  (void) printf ("Whenever a word is found that is not in the dictionary,\r\n");
  (void) printf ("it is printed on the first line of the screen.  If the dictionary\r\n");
  (void) printf ("contains any similar words, they are listed with a single digit\r\n");
  (void) printf ("next to each one.  You have the option of replacing the word\r\n");
  (void) printf ("completely, or choosing one of the suggested words.\r\n");
  (void) printf ("\r\n");
  (void) printf ("Commands are:\r\n\r\n");
  (void) printf ("R       Replace the misspelled word completely.\r\n");
  (void) printf ("Space   Accept the word this time only\r\n");
  (void) printf ("A       Accept the word for the rest of this file.\r\n");
  (void) printf ("I       Accept the word, and put it in your private dictionary.\r\n");
  (void) printf ("0-9     Replace with one of the suggested words.\r\n");
  (void) printf ("<NL>    Recompute near misses.  Use this if you interrupted\r\n");
  (void) printf ("             the near miss generator, and you want it to\r\n");
  (void) printf ("             again on this word.\r\n");
  (void) printf ("Q       Write the rest of this file, ignoring misspellings,\r\n");
  (void) printf ("            and start next file.\r\n");
  (void) printf ("X       Exit immediately.  Asks for conformation.\r\n");
  (void) printf ("            Leaves file unchanged.\r\n");
  (void) printf ("!       Shell escape.\r\n");
  (void) printf ("^L      Redraw screen.\r\n");
  (void) printf ("\r\n\r\n");
  (void) printf ("-- Type space to continue --");
  (void) fflush (stdout);
  (void) getchar ();
}


char *getline ();

char firstbuf[BUFSIZ], secondbuf[BUFSIZ];
char gtoken[BUFSIZ + 10];

int quit;

char *currentfile = NULL;
static long filesize;
static FILE *filestream;

int changed;

void
dofile (filename)
  char *filename;
{
  int c;
  FILE *in, *out;
  void (*oldf) (NOARGS);

  currentfile = filename;

  if ((in = fopen (filename, "r")) == NULL)
    {
      (void) fprintf (stderr, "Can't open %s\r\n", filename);
      return;
    }

  changed = 0;

  filesize = lseek (fileno (in), 0l, 2);
  (void) lseek (fileno (in), 0l, 0);

  if (access (filename, 2) < 0)
    {
      (void) fprintf (stderr, "Can't write to %s\r\n", filename);
      return;
    }

  (void) strcpy (tempfile, "/tmp/ispellXXXXXX");
  (void) mktemp (tempfile);
  if ((out = fopen (tempfile, "w")) == NULL)
    {
      (void) fprintf (stderr, "Can't create %s\r\n", tempfile);
      return;
    }

  quit = 0;

  checkfile (in, out, (long) 0);

  (void) fclose (in);
  (void) fclose (out);

  if (changed == 0)
    {
      (void) unlink (tempfile);
      tempfile[0] = 0;
      return;
    }

  if ((in = fopen (tempfile, "r")) == NULL)
    {
      (void) fprintf (stderr, "tempoary file disappeared (%s)\r\n", tempfile);
      (void) sleep (2);
      return;
    }

  oldf = (void (*)(NOARGS)) signal (SIGINT, SIG_IGN);

  if ((out = fopen (filename, "w")) == NULL)
    {
      (void) unlink (tempfile);
      tempfile[0] = 0;
      (void) fprintf (stderr, "can't create %s\r\n", filename);
      (void) sleep (2);
      return;
    }

  while ((c = getc (in)) != EOF)
    putc (c, out);

  fclose (in);
  (void) fclose (out);

  (void) unlink (tempfile);
  tempfile[0] = 0;

  signal (SIGINT, (RETSIGTYPE (*)()) oldf);
}

char *currentchar;
FILE *out;


void
checkfile (in, outx, end)
     FILE *in, *outx;
     long end;
{
  char *p;
  int maybe_troff;
  char *cstart, *cend;
  int i;
  long lineoffset;

  out = outx;

checkstart:
  filestream = in;
  secondbuf[0] = 0;

  maybe_troff = 0;
  while (1)
    {
      (void) strcpy (firstbuf, secondbuf);
      if (quit)
	{
	  if (out == NULL)
	    return;
	  while (fgets (secondbuf, sizeof secondbuf, in)
		 != NULL)
	    (void) fputs (secondbuf, out);
	  break;
	}

      if (Sflag)
	{
	  lineoffset = ftell (in);
	  if (end && lineoffset >= end)
	    break;
	}
      if (fgets (secondbuf, sizeof secondbuf, in) == NULL)
	break;

      /* uflag is on when emulating traditional spell
		 * if so, then follow troff commands '.so' and '.nx'
		 */
      if (uflag && !iflag && secondbuf[0] == '.')
	{
	  int soflag = 0, nxflag = 0;
	  FILE *so;
	  char *n, *end;
	  if (strncmp (secondbuf, ".so", 3) == 0)
	    soflag = 1;
	  if (strncmp (secondbuf, ".nx", 3) == 0)
	    nxflag = 1;
	  if (soflag || nxflag)
	    {
	      n = secondbuf + 3;
	      while (isspace (*n))
		n++;
	      end = n;
	      while (*end && !isspace (*end))
		end++;
	      *end = 0;
	      if (strncmp (n, "/usr/lib", 8) == 0)
		continue;
	      if (nxflag)
		{
		  if (freopen (n, "r", in) == NULL)
		    {
		      (void) fprintf (stderr, "can't open %s\n",
				      n);
		      return;
		    }
		  goto checkstart;
		}
	      if ((so = fopen (n, "r")) == NULL)
		{
		  (void) fprintf (stderr,
				  "can't open %s\n", n);
		}
	      else
		{
		  checkfile (so, (FILE *) NULL, 0);
		  (void) fclose (so);
		}
	      continue;
	    }
	}

      currentchar = secondbuf;

      p = secondbuf + strlen (secondbuf) - 1;
      if (*p == '\n')
	*p = 0;

      while (1)
	{
	  if (skip_to_next_word (out) == 0)
	    break;
	  cstart = currentchar;
	  cend = cstart;
	  if (intr_typed)
	    {
	      if (lflag || Sflag)
		return;

	      gtoken[0] = 0;
	      (void) correct (gtoken, sizeof gtoken,
			      cstart, cend, &currentchar);
	      if (quit)
		{
		  (void) fputs (currentchar, out);
		  break;
		}
	      intr_typed = 0;
	    }

	  /* first letter is a lexletter, therefore,
			 * loop will execute at least once
			 */
	  p = gtoken;
	  i = 0;
	  while (islexletter (*cend) && i < MAX_WORD_LEN - 5)
	    {
	      *p++ = *cend++;
	      i++;
	    }
	  /* flush quote if at end of word */
	  if (p[-1] == '\'')
	    {
	      p--;
	      cend--;
	    }
	  *p = 0;

	  currentchar = cend;

	  if (good (gtoken, strlen (gtoken), 0))
	    {
	      if (out)
		(void) fputs (gtoken, out);
	      continue;
	    }
	  interaction_flag = 1;
	  /* word is bad */
	  if (uflag)
	    {
	      /* traditional spell compatability */
	      (void) fputs (gtoken, sortf);
	      (void) putc ('\n', sortf);
	      /* don't report this word again by doing 'A' */
	      (void) p_enter (gtoken, 2, 0);
	      continue;
	    }
	  if (lflag)
	    {
	      /* make a simple list of bad words */
	      (void) puts (gtoken);
	      (void) p_enter (gtoken, 1, 0);
	      continue;
	    }
	  if (Sflag)
	    {
	      /* program interface */
	      (void) printf ("%ld\n", lineoffset + (cstart - secondbuf));
	      continue;
	    }
	  /* interactive mode */
	  if (correct (gtoken, sizeof gtoken,
		       cstart, cend, &currentchar))
	    {
	      /* user accepted, or choose a near miss */
	      (void) fputs (gtoken, out);
	      if (quit)
		{
		  (void) fputs (currentchar, out);
		  break;
		}
	    }
	  else
	    {
	      /* user typed replacement, rescan */
	      currentchar = cstart;
	    }
	}
      if (out)
	(void) putc ('\n', out);
    }
}

/* return 0 if no more words on this line
 * else return 1
 */
int
skip_to_next_word (out)
  FILE *out;
{
  switch (formatter)
    {
    case formatter_troff:
      return (skip_to_next_word_troff ());
    case formatter_tex:
      return (skip_to_next_word_tex (out));
    case formatter_generic:
    default:
      return (skip_to_next_word_generic ());
    }
}

int
skip_to_next_word_generic ()
{
  if (currentchar == secondbuf)
    {
      /* at beginning of line */
      if (*currentchar == '.')
	{
	  formatter = formatter_troff;
	  return (skip_to_next_word_troff ());
	}
    }

  while (*currentchar)
    {
      /* skip any leading apostrophes */
      if (islexletter (*currentchar) && *currentchar != '\'')
	return (1);
      copyout (&currentchar, 1, out);
    }
  return (0);
}

int
skip_to_next_word_troff ()
{
  if (currentchar == secondbuf)
    {
      /* at beginning of line */
      /* if this is a formatter command, skip the line */
      if (*currentchar == '.')
	{
	  if (out)
	    {
	      while (*currentchar)
		(void) putc (*currentchar++, out);
	    }
	  return (0);
	}
    }

  while (*currentchar)
    {
      if (islexletter (*currentchar) && *currentchar != '\'')
	return (1);

      if (*currentchar != '\\')
	{
	  copyout (&currentchar, 1, out);
	  continue;
	}

      if (currentchar[1] == 'f')
	{
	  /* \fB or \f(XY */
	  if (currentchar[2] == '(')
	    copyout (&currentchar, 5, out);
	  else
	    copyout (&currentchar, 3, out);
	}
      else if (currentchar[1] == 's')
	{
	  /* \s5 \s+2 \s-2 \s30
			 * skip one following character
			 * there may be more digits following, but
			 * they will get skipped since they are not
			 * lexletters
			 */
	  copyout (&currentchar, 3, out);
	}
      else if (currentchar[1] == '(')
	{
	  /* extended character set \(XY */
	  copyout (&currentchar, 3, out);
	}
      else if (currentchar[1] == '*')
	{
	  /* string interpolation \*X \*(XY */
	  if (currentchar[2] == '(')
	    copyout (&currentchar, 5, out);
	  else
	    copyout (&currentchar, 3, out);
	}
      else
	{
	  copyout (&currentchar, 1, out);
	}
    }

  return (0);
}

/* return 1 if everyting ok, 0 to recheck some */
int
correct (token, toksiz, start, end, pcurrentchar)
  char *token;
  unsigned toksiz;
  char *start, *end;
  char **pcurrentchar;
{
  int c;
  int i;
  char *p;
  int len;
  int needposs;
  int intcnt = 0;
  int mpinterrupted = 0;


  /* zero length token just means to run the command loop */
  len = strlen (token);

  needposs = 1;
  corrections.nwords = 0;
redraw:
  (void) setjmp (command_loop);

  erase ();
  if (len)
    {
      (void) printf ("    %s", token);
    }
  else
    {
      inverse ();
      (void) printf ("(INTERRUPT)");
      normal ();
      needposs = 0;
    }

  if (currentfile)
    (void) printf ("              File: %s (%ld%%)", currentfile,
		   ftell (filestream) * 100 / filesize);
  (void) printf ("\r\n\r\n");

  if (needposs)
    {
      mpinterrupted = makepossibilities (token);
      needposs = 0;
    }
  for (i = 0; i < corrections.nwords; i++)
    {
      if (corrections.posbuf[i][0] == 0)
	break;
      (void) printf ("%d: %s\r\n", i, corrections.posbuf[i]);
    }
  if (mpinterrupted)
    (void) printf ("(near miss generator interrupted; type return to rerun)\r\n");

  move (15, 0);
  (void) printf ("%s\r\n", firstbuf);

  for (p = secondbuf; p != start; p++)
    (void) putchar (*p);
  inverse ();
  for (i = len; i > 0; i--)
    (void) putchar (*p++);
  normal ();
  while (*p)
    (void) putchar (*p++);
  (void) printf ("\r\n");

  while (1)
    {
      (void) fflush (stdout);
      intr_typed = 0;
      reading_interactive_command = 1;
      c = getchar ();
      reading_interactive_command = 0;
      if (c == -1)
	{
	  intcnt++;
	  if (intcnt == 10)
	    {
	      quit = 1;
	      erase ();
	      return (1);
	    }
	  goto redraw;
	}
      intcnt = 0;
      c &= NOPARITY;
      switch (c)
	{
	case '\r':
	case '\n':
	  if (len)
	    needposs = 1;
	  goto redraw;
	case 'Z' & 037:
	case 'Y' & 037:
	case 'Z':
	  stop ();
	  goto redraw;
	case ' ':		/* accept this time only */
	  erase ();
	  return 1;
	case 'x':
	case 'X':		/* exit without writing */
	  (void) printf ("Exit without writing changes? ");
	  (void) fflush (stdout);
	  c = (getchar () & NOPARITY);
	  if (c == 'y' || c == 'Y')
	    {
	      erase ();
	      done ();
	    }
	  termbeep ();
	  goto redraw;
	case 'i':
	case 'I':		/* put in private dictionary */
	  if (len == 0)
	    break;
	  if (len < MAX_WORD_LEN - 5)
	    (void) p_enter (token, 1, 1);
	  erase ();
	  return 1;
	case 'a':
	case 'A':		/* don't report again this session */
	  if (len == 0)
	    break;
	  if (len < MAX_WORD_LEN - 5)
	    (void) p_enter (token, 1, 0);
	  erase ();
	  return 1;
	case 'L' & 037:	/* redraw */
	  goto redraw;
	case '?':		/* help */
	  if (len == 0)
	    giveihelp ();
	  else
	    givehelp ();
	  goto redraw;
	case '!':
	  {
	    char buf[200];
	    move (18, 0);
	    (void) putchar ('!');
	    if (getline (buf, sizeof buf) == NULL)
	      goto redraw;
	    (void) printf ("\r\n");
	    shellescape (buf);
	    goto redraw;
	  }
	case 'r':
	case 'R':
	  if (len == 0)
	    break;
	  move (18, 0);
	  (void) printf ("Replace with: ");
	  if (getline (token, toksiz) == NULL)
	    goto redraw;
	  inserttoken (secondbuf, start, end, token, (char **) NULL);
	  erase ();
	  return 0;		/* 0 means rescan */
	case 'l':
	case 'L':
	  move (18, 0);
	  (void) printf ("Lookup (regular expression): ");
	  if (getline (token, toksiz) == NULL)
	    goto redraw;
	  move (19, 0);
	  termflush ();
	  dolook_interactive (token);
	  goto redraw;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  if (corrections.posbuf[c - '0'][0] != 0)
	    {
	      (void) strcpy (token, corrections.posbuf[c - '0']);
	      inserttoken (secondbuf, start, end, token,
			   pcurrentchar);
	      erase ();
	      return 1;
	    }
	  break;
	case 'q':
	case 'Q':
	  quit = 1;
	  erase ();
	  return 1;
	default:
	  break;
	}
      termbeep ();
    }
}

/*
 * buf is line
 * replace between start and end with token
 */
void
inserttoken (buf, start, end, token, pcurrentchar)
  char *buf, *start, *end, *token;
  char **pcurrentchar;
{
  char copy[BUFSIZ];
  char *p, *q;

  changed = 1;

  /* copy up to start */
  for (p = copy, q = buf; q != start; p++, q++)
    *p = *q;
  /* put in token */
  while (*token)
    *p++ = *token++;
  /* remember place in buf */
  if (pcurrentchar)
    *pcurrentchar = buf + (p - copy);
  /* skip over old stuff */
  q = end;
  /* copy the tail of the line */
  while (*q)
    *p++ = *q++;
  *p = 0;
  (void) strcpy (buf, copy);
}

char *
getline (s, siz)
  char *s;
  unsigned siz;
{
  char *p;
  int c;
  extern int erasechar, killchar;
  int pos;

  if (siz <= 1)
    return (NULL);
  siz--;
  p = s;
  pos = 0;

  while (1)
    {
      c = (getchar () & NOPARITY);
      if (c == '\\')
	{
	  if (pos == siz)
	    {
	      p--;
	      pos--;
	      backup ();
	    }
	  (void) putchar ('\\');
	  c = (getchar () & NOPARITY);
	  backup ();
	  (void) putchar (c);
	  *p++ = c;
	  pos++;
	}
      else if (c == ('G' & 037))
	{
	  return (NULL);
	}
      else if (c == '\n' || c == '\r')
	{
	  *p = 0;
	  return (s);
	}
      else if (c == erasechar)
	{
	  if (p != s)
	    {
	      p--;
	      backup ();
	      (void) putchar (' ');
	      backup ();
	    }
	}
      else if (c == killchar)
	{
	  while (p != s)
	    {
	      p--;
	      backup ();
	      (void) putchar (' ');
	      backup ();
	    }
	}
      else
	{
	  if (pos == siz)
	    {
	      p--;
	      pos--;
	      backup ();
	    }
	  *p++ = c;
	  (void) putchar (c);
	}
    }
}


void
askmode (verbose)
  int verbose;
{
  char buf[MAX_WORD_LEN];
  int i;
  int len;

  setbuf (stdin, (char *) NULL);
  setbuf (stdout, (char *) NULL);

  while (1)
    {
      if (verbose)
	{
	  (void) printf ("word: ");
	  (void) fflush (stdout);
	}

      if (fgets (buf, MAX_WORD_LEN - 5, stdin) == NULL)
	break;

      len = strlen (buf);
      if (len && buf[len - 1] == '\n')
	buf[--len] = 0;

      if (verbose && len == 0)
	continue;

      if (good (buf, strlen (buf), 0))
	{
	  /* used to print + if rootword */
	  if (verbose)
	    (void) printf ("ok\n");
	  else
	    (void) printf ("*\n");
	}
      else
	{
	  (void) makepossibilities (buf);
	  if (corrections.posbuf[0][0])
	    {
	      if (verbose)
		(void) printf ("how about: ");
	      else
		(void) printf ("& ");
	      for (i = 0; i < 10; i++)
		{
		  if (corrections.posbuf[i][0] == 0)
		    break;
		  (void) printf ("%s ",
				 corrections.posbuf[i]);
		}
	      (void) printf ("\n");
	    }
	  else
	    {
	      if (verbose)
		(void) printf ("not found\n");
	      else
		(void) printf ("#\n");
	    }
	}
    }
}

static void
copyout (cc, cnt, out)
  char **cc;
  int cnt;
  FILE *out;
{
  while (--cnt >= 0)
    {
      if (*(*cc) == 0)
	break;
      if (out)
	(void) putc (*(*cc), out);
      (*cc)++;
    }
}

static char *lookcmd, *lookarg1, *lookarg2;

/* this runs in a child with the terminal set back to normal
 * and with the parent ignoring signals
 */
void
lookfun (NOARGS)
{
  if (lookarg2 == NULL)
    (void) execlp (lookcmd, lookcmd, lookarg1, NULL);
  else
    (void) execlp (lookcmd, lookcmd, lookarg1, lookarg2, NULL);
}


void
dolook (str)
     char *str;
{
  char *p;
  int wild = 0;

  for (p = str; *p; p++)
    {
      if (strchr (".*[\\", *p) != NULL)
	wild = 1;
      if (isupper (*p))
	*p = tolower (*p);
    }

  if (wild == 0)
    {
      lookcmd = "look";
      lookarg1 = str;
      lookarg2 = NULL;
      if (!dochild (lookfun))
	return;
    }

  lookcmd = "egrep";
  lookarg1 = str;
  lookarg2 = "/usr/lib/ispell.words";
  dochild (lookfun);
}


void
dolook_interactive (str)
  char *str;
{

  dolook (str);
  (void) printf ("\n-- Type space to continue --");
  (void) getchar ();
}
