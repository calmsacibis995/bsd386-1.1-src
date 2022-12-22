/*======================================================================
   filt: Pass MIDI bytes from IN to OUT, with some filtering
   [ This file is a part of SBlast-BSD-1.5 ]

   Steve Haehnichen <shaehnic@ucsd.edu>

   filt.c,v 1.2 1992/09/14 03:29:57 steve Exp

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
#define CYCLE		4

void
main (int argc, char **argv)
{
  int midi_dev;
  unsigned char *data, *status, *data1, *data2;
  int got, breath;

  breath = 1;
  data = (unsigned char *) malloc (4);
  status = data;
  data1  = data + 1;
  data2  = data + 2;

  midi_dev = open ("/dev/sb_midi", O_RDWR, 0);
  if (midi_dev < 0)
      perror ("midi_dev"), exit(1);

  while (1)
    {
      got = read (midi_dev, status, 1);
      if (got <= 0)
	  perror ("read()");

      switch (*status & 0xf0)
	{
	case 0x80:		/* Note off */
	  printf ("Note off\n");
	  read (midi_dev, data1, 1); /* pitch */
	  read (midi_dev, data2, 1); /* vel */
	  write (midi_dev, data, 3);
	  break;
	case 0x90:		/* Note on */
	  read (midi_dev, data1, 1); /* pitch */
	  read (midi_dev, data2, 1); /* vel */
	  write (midi_dev, data, 3);
	  break;
	case 0xB0:		/* Control/Mode change */
	  read (midi_dev, data1, 1); /* controller */
	  read (midi_dev, data2, 1); /* value */
	  write (midi_dev, data, 3);
	  break;
	case 0xC0:		/* Program change */
	  read (midi_dev, data1, 1); /* new program */
	  write (midi_dev, data, 2);
	  break;
	case 0xD0:		/* Aftertouch */
	  read (midi_dev, data1, 1); /* value */
	  write (midi_dev, data, 2);
	  break;
	  
	default:
/*	  printf ("Unknown status: 0x%2x\n", *status); */
	  write (midi_dev, status, 1); /* Pass through */
	  break;
	}
    }				/* while(1) */
}
