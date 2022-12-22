/*======================================================================
   filt: Pass MIDI bytes from IN to OUT, with some filtering
   [ This file is a part of SBlast-BSD-1.2b ]

   Steve Haehnichen <shaehnic@ucsd.edu>   April 1, 1992.

   my_filt.c,v 1.1.1.1 1992/07/13 02:47:14 steve Exp

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
  int notes_on = 0;
  int inst = 0;
  unsigned char *data, *status, *data1, *data2;
  int got, breath, i, count = 0;
  unsigned char cur_note;

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
/*	  if (breath & *data2)
	      *data2 = 127; */
	  write (midi_dev, data, 3);
	  break;
	case 0xB0:		/* Control/Mode change */
	  read (midi_dev, data1, 1);
	  read (midi_dev, data2, 2);
	  break;
	  if (*data2 == 0)
	      breath = 1;
	  else
	    {
	      breath = 0;
	      *status = 0xB0;
	      *data1 = 0x07;
	      *data2 = 64;
	      write (midi_dev, data, 3);
	    }
	  break;
	case 0xC0:		/* Program change */
	  read (midi_dev, data1, 1);
	  if (++inst > 48)
	      inst = 0;
	  *data1 = inst & 0xff;
	  write (midi_dev, data, 2);
	  break;
	case 0xD0:		/* Aftertouch */
	  read (midi_dev, data1, 1);
	  break;
	  if (!breath)
	      break;
	  count++;
	  if (count > CYCLE)
	    {
	      count = 0;
	      *status = 0xB0;
	      *data2 = *data1;
	      *data1 = 0x07;
	      write (midi_dev, data, 3);
	    }
	  break;
	  
	default:
	  printf ("Unknown status: 0x%2x\n", *status);
	  write (midi_dev, status, 1);
	  break;
	}
    }				/* while(1) */
}
