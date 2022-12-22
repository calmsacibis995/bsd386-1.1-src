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
#include <signal.h>
#include <setjmp.h>

#if defined (HAVE_TERMIO_H)
#include <termio.h>
#else
#if defined (HAVE_TERMIOS_H)
#include <termios.h>
#else
/* Assume BSD if all else fails */
#include <sgtty.h>
#endif /* not HAVE_TERMIOS_H */
#endif /* not HAVE_TERMIO_H */

#include <signal.h>
#include "ispell.h"


extern int reading_interactive_command;
extern jmp_buf command_loop;

int erasechar, killchar;

/* termcap variables */
static char *BC, *cd, *cl, *so, *se;
#ifndef VENIX
static char *cm, *ho, termcap[1024], termstr[1024];
#endif
static int li;

void
gettermcap ()
{
#ifdef NO_TERMCAP
  /* IBM pc ansi escape codes */
  BC = "\b";
  cl = "\033[0;0H\033[0J";
  cd = "\033[0J";
  so = "\033[47;30m";
  se = "\033[37;40m";
  li = 25;
#else
  char *termptr;
  char *tgetstr ();

  tgetent (termcap, getenv ("TERM"));
  termptr = termstr;
  BC = tgetstr ("bc", &termptr);/* backspace */
  if (BC == NULL)
    BC = "\b";
  cd = tgetstr ("cd", &termptr);/* clear to end of screen */
  cl = tgetstr ("cl", &termptr);/* clear screen */
  cm = tgetstr ("cm", &termptr);/* cursor motion */
  ho = tgetstr ("ho", &termptr);/* home */
  so = tgetstr ("so", &termptr);/* inverse video on */
  se = tgetstr ("se", &termptr);/* inverse video off */
  li = tgetnum ("li");		/* number of lines on screen */
#endif
}

#ifdef NO_TERMCAP
/* ARGSUSED */
void
tputs (str, lines, put)
     char *str;
     int (*put) ();
{
  while (*str)
    (*put) (*str++);
}

#endif

void
putch (c)
  int c;
{
  putchar (c);
}

void
termflush ()
{
  (void) fflush (stdout);
}

void
erase ()
{
  if (cl)
    tputs (cl, li, putch);
  else
    {
      move (0, 0);
      tputs (cd, li, putch);
    }
}

void
move (row, col)
  int row, col;
{
#ifdef NO_TERMCAP
  (void) printf ("\033[%d;%dH", row, col);
#else
  char *tgoto ();
  tputs (tgoto (cm, col, row), 100, putch);
#endif
}


void
inverse ()
{
  tputs (so, 10, putch);
}

void
normal ()
{
  tputs (se, 10, putch);
}

void
backup ()
{
  tputs (BC, 1, putch);
}

static int termchanged = 0;

#if !defined (HAVE_TERMIOS_H) && !defined (HAVE_TERMIO_H)
static struct sgttyb sbuf, osbuf;

void
terminit ()
{
  int tpgrp;
  int onstop ();

retry:
  sigsetmask (1 << SIGTSTP | 1 << SIGTTIN | 1 << SIGTTOU);
  /* apparently tpgrp was a short on 4.1, but is now a long -
	 * set the high bits to zero in case the ioctl doesn't write them.
	 */
  tpgrp = 0;
  if (ioctl (0, TIOCGPGRP, &tpgrp) != 0)
    {
      (void) fprintf (stderr, "must run from tty in interactive mode\n");
      exit (1);
    }
  if (tpgrp != getpgrp (0))
    {				/* not in foreground */
      (void) sigsetmask (1 << SIGTSTP | 1 << SIGTTIN);
      (void) signal (SIGTTOU, SIG_DFL);
      (void) kill (0, SIGTTOU);
      /* job stops here waiting for SIGCONT */
      goto retry;
    }

  (void) ioctl (0, TIOCGETP, &osbuf);
  termchanged = 1;

  sbuf = osbuf;
  sbuf.sg_flags &= ~ECHO;
  sbuf.sg_flags |= CBREAK;
  ioctl (0, TIOCSETP, &sbuf);

  erasechar = sbuf.sg_erase;
  killchar = sbuf.sg_kill;

  (void) signal (SIGTTIN, onstop);
  (void) signal (SIGTTOU, onstop);
  (void) signal (SIGTSTP, onstop);
  (void) sigsetmask (0);
  gettermcap ();
}

void
termuninit ()
{
  if (termchanged)
    {
      (void) ioctl (0, TIOCSETP, (char *) &osbuf);
      termchanged = 0;
    }
}

void
termreinit ()
{
  if (termchanged == 0)
    {
      (void) ioctl (0, TIOCSETP, (char *) &sbuf);
      termchanged = 1;
    }
}

#endif /* !HAVE_TERMIO_H && !HAVE_TERMIOS_H */

#if defined (HAVE_TERMIO_H) || defined (HAVE_TERMIOS_H)

#if defined (HAVE_TERMIO_H)

struct termio termio, otermio;
int
tcgetattr (fd, tp)
     int fd;
     struct termio *tp;
{
  return (ioctl (fd, TCGETA, (char *) tp));
}

int
tcsetattr (fd, how, tp)
     int fd;
     int how;
     struct termio *tp;
{
  return (ioctl (fd, how, tp));
}
#define TCSANOW TCSETA

#endif /* not HAVE_TERMIO_H */

#if defined (HAVE_TERMIOS_H)

struct termios termio, otermio;

#endif /* not HAVE_TERMIO_H */

void
terminit ()
{
  if (tcgetattr (0, &otermio) < 0)
    {
      (void) fprintf (stderr, "must run from tty in interactive mode\n");
      exit (1);
    }

  termchanged = 1;

  termio = otermio;
  termio.c_lflag &= ~(ICANON | ECHO);
  termio.c_cc[VMIN] = 1;
  termio.c_cc[VTIME] = 0;
  erasechar = termio.c_cc[VERASE];
  killchar = termio.c_cc[VKILL];

  (void) tcsetattr (0, TCSANOW, &termio);

  gettermcap ();
}


void
termuninit ()
{
  if (termchanged)
    {
      (void) tcsetattr (0, TCSANOW, &otermio);
      termchanged = 0;
    }
}

void
termreinit ()
{
  if (termchanged == 0)
    {
      (void) tcsetattr (0, TCSANOW, &termio);
      termchanged = 1;
    }
}

#endif /* USG */

#ifdef MSDOS
void terminit () { ; }
void termuninit () { ; }
void termreinit () { ; }
#endif

#ifdef SIGTTIN
void
onstop (signo)
  int signo;
{
  (void) signal (signo, SIG_DFL);
  termuninit ();
  sigsetmask (0);
  (void) killpg (getpgrp (0), signo);
  /* stop here */
  signal (signo, onstop);
  termreinit ();
  if (reading_interactive_command)
    longjmp (command_loop, 1);
}

void
stop ()
{
  onstop (SIGTSTP);
}

#else
void
stop ()
{
  shellescape ((char *) NULL);
}

#endif

static char *shellcmd, *shellsh;;

void
shellfun ()
{
  if (shellcmd == (char *) 0)
    execlp (shellsh, shellsh, (char *) 0);
  else
    execlp (shellsh, shellsh, "-c", shellcmd, (char *) 0);
}


void
shellescape (buf)
     char *buf;
{
  shellsh = (char *) getenv ("SHELL");
  if (shellsh == NULL)
    shellsh = "/bin/sh";
  shellcmd = buf;
  (void) dochild (shellfun);
  (void) printf ("\n-- Type space to continue --");
  getchar ();
}

int
dochild (fun)
     void (*fun) (NOARGS);
{
  int pid, w;
  int status, val = 1;
  RETSIGTYPE (*oldf) (NOARGS);
  termuninit ();

  oldf = (RETSIGTYPE (*)()) signal (SIGINT, SIG_IGN);
  (void) signal (SIGQUIT, SIG_IGN);
#ifdef SIGTTIN
  (void) signal (SIGTTIN, SIG_DFL);
  (void) signal (SIGTTOU, SIG_DFL);
  (void) signal (SIGTSTP, SIG_DFL);
#endif

  pid = fork ();
  if (pid < 0)
    {
      perror ("fork");
      goto ret;
    }
  if (pid == 0)
    {
      (void) signal (SIGINT, SIG_DFL);
      (void) signal (SIGQUIT, SIG_DFL);
      (*fun) ();
      _exit (1);
    }
  while ((w = wait (&status)) >= 0)
    if (w == pid)
      break;

  val = (status >> 8) & 0xff;
ret:
#ifdef SIGTTIN
  signal (SIGTTIN, onstop);
  signal (SIGTTOU, onstop);
  signal (SIGTSTP, onstop);
#endif
  signal (SIGINT, (RETSIGTYPE (*)()) oldf);
  signal (SIGQUIT, SIG_DFL);

  termreinit ();

  return (val);
}

void
termbeep ()
{
  (void) putchar (7);
  (void) fflush (stdout);
}
