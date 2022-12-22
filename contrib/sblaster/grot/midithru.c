/*======================================================================
   midithru: Pass MIDI bytes from IN to OUT
   [ This file is a part of SBlast-BSD-1.5 ]

   Steve Haehnichen <shaehnic@ucsd.edu>

   midithru.c,v 1.2 1992/09/14 03:30:01 steve Exp

   Copyright (C) 1991 Steve Haehnichen.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

======================================================================*/

#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include <i386at/sblast.h>
#include "time.h"

#define BUFSIZE		256

void
main (int argc, char **argv)
{
  int midi_dev;
  char midi_buf[BUFSIZE];
  int got;

  midi_dev = open ("/dev/sb_midi", O_RDWR, 0);
  if (midi_dev < 0)
    perror ("midi_dev"), exit(1);
  
  while (1)
    {
      got = read (midi_dev, midi_buf, BUFSIZE);
      if (got <= 0)
	perror ("read()");
      if (write (midi_dev, midi_buf, got) <= 0)
	perror ("write()");
    }				/* while(1) */
}
