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


/* initialize mapcase array
 
   this array has two jobs: to distingush between "letters" (characters
   that can be parts of words) and to convert upper case to lower case
   when that makes sense.  The functions in 'ctype' would be good enough,
   except that now we want to handle international characters */

#include <stdio.h>
#include <ctype.h>

short mapcase[256];

mapcaseinit ()
{
  static firsttime = 1;
  int i;

  if (firsttime == 0)
    return;

  firsttime = 0;

  /* map upper case to lower case */
  for (i = 'A'; i <= 'Z'; i++)
    mapcase[i] = i - 'A' + 'a';

  /* map lower case to itself */
  for (i = 'a'; i <= 'z'; i++)
    mapcase[i] = i;

  /* add other normal ascii characters that should be considered
	 * letters here.  You might want '-' or '_'
	 */
  mapcase['\''] = '\'';

  /* map all high bit set characters to themselves - they
	 * wont normally appear, and if they do, they probably represent
	 * characters with accent marks
	 */
  for (i = 0x80; i < 0x100; i++)
    mapcase[i] = i;

  /*
	 * now map upper case accented characters to
	 * lower case equivalents
	 */
  /* IBM-PC */
  mapcase[0x80] = 0x87;		/* c cedialla */
  mapcase[0x8e] = 0x84;		/* a umlaut */
  mapcase[0x8f] = 0x86;		/* a with circle */
  mapcase[0x90] = 0x82;		/* e acute */
  mapcase[0x92] = 0x91;		/* ae ligaturrre */
  mapcase[0x99] = 0x94;		/* o umlaut */
  mapcase[0x9a] = 0x81;		/* u umlaut */
  mapcase[0xa5] = 0xa4;		/* n tilde */
}
