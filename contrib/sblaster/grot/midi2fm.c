/*======================================================================
   midi2fm: Play FM notes from MIDI input.
   [ This file is a part of SBlast-BSD-1.5 ]

   Steve Haehnichen <shaehnic@ucsd.edu>

   midi2fm.c,v 1.2 1992/09/14 03:30:00 steve Exp

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

#define VOICES		3
#define CHAN		FM_BOTH
#define RAND(bits) 	(random() & ((1 << (bits)) - 1))
#define AT_SAMP		10

void load_instruments (void);
void play_all (int pitch, int velocity);
void all_off ();

struct sb_fm_note op0;
struct sb_fm_note n[VOICES];
struct sb_fm_params p;
int fm_dev, midi_dev;

int note_table[] = {
  343,  363,  385,  408,  432,  458,  485,  514,  544,  577,  611,  647 };
char *note_names[] = {
  "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};

void
main (int argc, char **argv)
{
  int i;
  unsigned char status, data1, data2;
  int notes_on = 0;
  int at_cnt = 0;

  fm_dev = open ("/dev/sb_fm", O_WRONLY, 0);
  if (fm_dev < 0)
    perror ("fm_dev"), exit(1);
  
  midi_dev = open ("/dev/sb_midi", O_RDONLY, 0);
  if (midi_dev < 0)
    perror ("midi_dev"), exit(1);
  
  if (ioctl (fm_dev, FM_IOCTL_RESET, 0) == -1)
    perror ("FM reset"), exit(1);
  
  srandom (time(0));

  load_instruments ();
  
  while (1)
    {
      read (midi_dev, &status, 1);
      if (!(status & 0x80))
	{
	  printf ("Running status ignored.\n");
	  continue;
	}
      
      switch (status & 0xf0)
	{
	case 0x80:		/* Note off */
	  printf ("Note off\n");
	  read (midi_dev, &data1, 1); /* pitch */
	  read (midi_dev, &data2, 1); /* vel */
	  all_off ();
	  break;
	case 0x90:		/* Note on */
	  read (midi_dev, &data1, 1); /* pitch */
	  read (midi_dev, &data2, 1); /* velocity */
	  if (data2 == 0)
	    {
	      if (--notes_on == 0)
		all_off();
	    }
	  else
	    {
	      play_all (data1, data2);
	      notes_on++;
	    }
	  break;
	case 0xB0:		/* Pitch-bender */
	  printf ("Pitch bend data ignored.\n");
	  read (midi_dev, &data1, 1);
	  read (midi_dev, &data2, 1);
	  break;
	case 0xC0:
	  read (midi_dev, &data1, 1); /* program change */
	  notes_on = 0;
	  load_instruments();
	  all_off();
	  break;
	case 0xD0:		/* Aftertouch */
	  read (midi_dev, &data1, 1);
/*	  if (++at_cnt < AT_SAMP) */
	    break;

	  at_cnt = 0;
/*	  printf ("AT: %3d\r", data1); */
	  fflush (stdout);
	  for (i = 0; i < VOICES; i++)
	    {
	      n[i].volume = data1 >> 1;
	      if (ioctl (fm_dev, FM_IOCTL_PLAY_NOTE, &n[i]) < 0)
		perror ("play_note");
	    }			/* for each voice */
	  break;
	  
	default:
	  printf ("Ignoring status: 0x%2x\n", status);
	  break;
	}
    }				/* while(1) */
}

void
all_off (void)
{
  int i;
/*  printf ("All off.\n"); */
  for (i = 0; i < VOICES; i++)
    {
      n[i].key_on = 0;
      if (ioctl (fm_dev, FM_IOCTL_PLAY_NOTE, &n[i]) < 0)
	perror ("play_note");
    }
}
      
void
play_all (int pitch, int amp)
{
  int i;
  unsigned int fnum;
  BYTE oct, volume;

/* pitch += 7; */
  
  fnum = note_table[pitch % 12];
  oct  = (pitch / 12) & 0x07;
  oct += -2;
  volume = amp >> 1;
  printf ("Playing note - %d %s  vol: %d\n",
	  oct, note_names[pitch % 12], volume);

  for (i = 0; i < VOICES; i++)
    {
      n[i].fnum   = fnum;
      n[i].octave = oct;
      n[i].volume = volume;

      n[i].key_on = 1;
      if (ioctl (fm_dev, FM_IOCTL_PLAY_NOTE, &n[i]) < 0)
	perror ("play_note");
    }				/* for each voice */
}

void
load_instruments ()
{
  int i;

  printf ("Loading instruments.\n");
  ioctl (fm_dev, FM_IOCTL_RESET, 0);
  p.channel = CHAN;
  p.am_depth = 1;
  p.vib_depth = 1;
  p.rhythm = 0;
  p.kbd_split = 0;
  p.wave_ctl = 1;
  p.speech = 0;
  if (ioctl (fm_dev, FM_IOCTL_SET_PARAMS, &p) < 0)
    perror ("set_params"); 
      
  for (i = 0; i < VOICES; i++)
    {
      n[i].operator = 1;

      n[i].voice = i;
      n[i].channel = CHAN;

      n[i].am = RAND(1);
      n[i].vibrato = RAND(1);
      n[i].do_sustain = 0;
      n[i].harmonic = RAND(2);
      n[i].kbd_scale = 0;
      n[i].scale_level = 0;
      n[i].attack = RAND(4) | 1;
      n[i].decay = RAND(4);
      n[i].sustain = RAND(4);
      n[i].release = RAND(4);
      n[i].feedback = RAND(3);
      n[i].waveform = RAND(2);
      n[i].indep_op = 0;

      op0 = n[i];
      /* These are all per-operator settings: */
      op0.operator = 0;

      op0.waveform = RAND(2);
      op0.volume = RAND(6);
      op0.scale_level = RAND(2);
      op0.am = RAND(1);
      op0.vibrato = RAND(1);
      op0.do_sustain = RAND(1);
      op0.kbd_scale = RAND(1);
      op0.harmonic = RAND(4);
      op0.attack = RAND(4) | 0x01;
      op0.decay = RAND(4);
      op0.sustain = RAND(4); 
      op0.release = RAND(4);
      op0.key_on = 0;
      if (ioctl (fm_dev, FM_IOCTL_PLAY_NOTE, &op0) < 0)
	perror ("play_note");
/*      printf ("Using vol %d, harm %d\n", op0.volume, op0.harmonic); */
    }
}      
