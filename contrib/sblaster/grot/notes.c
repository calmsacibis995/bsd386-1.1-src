/*======================================================================
   notes: Play random FM notes.
   [ This file is a part of SBlast-BSD-1.5 ]

   Steve Haehnichen <shaehnic@ucsd.edu>

   notes.c,v 1.2 1992/09/14 03:30:04 steve Exp

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

#define VOICES		9
#define RAND(bits) 	(random() & ((1 << (bits)) - 1))

void
main (int argc, char **argv)
{
  int fmdev;
  struct sb_fm_note n;
  struct sb_fm_params p;

  fmdev = open ("/dev/sb_fm", O_RDWR, 0);
  if (fmdev < 0)
    perror ("fmdev");
  
  if (ioctl (fmdev, FM_IOCTL_RESET, 0) == -1)
    perror ("reset");
  
  srandom (time(0));

  while (1)
    {
      /* cycle through operators 0 and 1 and L & R channels */
      p.channel = !p.channel;
      n.channel = !n.channel;
      if (p.channel)
	n.operator = !n.operator;

      p.am_depth = RAND(1);
      p.vib_depth = RAND(1);
      p.rhythm = RAND(1);
      p.bass = RAND(1);
      p.snare = RAND(1);
      p.tomtom = RAND(1);
      p.cymbal = RAND(1);
      p.hihat = RAND(1);
      p.kbd_split = RAND(1);
      p.wave_ctl = 1;
      p.speech = 0;
      if (ioctl (fmdev, FM_IOCTL_SET_PARAMS, &p) < 0)
	perror ("set_params"); 
      
      for (n.voice = 0; n.voice < VOICES; n.voice++)
	{
	  n.am = RAND(1);
	  n.vibrato = RAND(1);
	  n.do_sustain = RAND(1);
	  n.kbd_scale = RAND(1);
	  n.harmonic = RAND(4);
	  n.scale_level = RAND(2);
	  n.attack = RAND(4) | 0x01;
	  n.decay = RAND(4);
	  n.sustain = RAND(4); 
	  n.release = RAND(4);
 	  n.fnum = RAND(10); 
	  n.octave = RAND(3);
	  n.feedback = RAND(3);
	  n.indep_op = 0;
	  n.waveform = RAND(2);

 	  n.volume = 0x3f;

/*	  printf ("op: %d  voice: %d  channel: %d\n",
		  n.operator, n.voice, n.channel); */
	  n.key_on = 0;
	  if (ioctl (fmdev, FM_IOCTL_PLAY_NOTE, &n) < 0)
	    perror ("play_note");
	  n.key_on = 1;
	  if (ioctl (fmdev, FM_IOCTL_PLAY_NOTE, &n) < 0)
	    perror ("play_note");
/*	  usleep (RAND(1) ? 170000 : 100000); */
	}			/* for each voice */
      printf ("."); fflush(stdout);
    }				/* while(1) */
}
