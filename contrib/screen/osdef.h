/*

* This file is automagically created from osdef.sh -- DO NOT EDIT

*/

/* Copyright (c) 1993
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ****************************************************************
 * $Id: osdef.h,v 1.1 1994/01/29 17:20:22 polk Exp $ FAU
 */

/****************************************************************
 * Thanks to Christos S. Zoulas (christos@ee.cornell.edu) who 
 * mangled the screen source through 'gcc -Wall'.
 ****************************************************************
 */

#ifdef SYSV
#else
#endif

#ifndef USEBCOPY
# ifdef USEMEMCPY
# else
#  ifdef USEMEMMOVE
#  else
#  endif
# endif
#else
#endif

#ifdef BSDWAIT
struct rusage;
union wait;
#else
#endif


#ifndef NOREUID
# ifdef hpux
extern int   setresuid __P((int, int, int));
extern int   setresgid __P((int, int, int));
# else
# endif
#endif


extern int   tgetent __P((char *, char *));
extern int   tgetnum __P((char *));
extern int   tgetflag __P((char *));
extern void  tputs __P((char *, int, void (*)(int)));
extern char *tgoto __P((char *, int, int));

#ifdef POSIX
#endif







#ifdef NAMEDPIPE
#else
struct sockaddr;
#endif

#if defined(UTMPOK) && defined(GETUTENT)
extern void  setutent __P((void));
#endif

#if defined(sequent) || defined(_SEQUENT_)
extern int   getpseudotty __P((char **, char **));
#ifdef _SEQUENT_
extern int   fvhangup __P((char *));
#endif
#endif

#ifdef USEVARARGS
#endif
struct timeval;


# if defined(GETTTYENT) && !defined(GETUTENT) && !defined(UTNOKEEP)
struct ttyent;
# endif
