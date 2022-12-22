/*======================================================================
   playmidi: Read a standard MIDI file and send it to SB MIDI OUT
   [ This file is a part of SBlast-BSD-1.5 ]

   Steve Haehnichen <shaehnic@ucsd.edu>

   playmidi.c,v 1.2 1992/09/14 03:30:06 steve Exp

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

   Large portions of this program were copied verbatim from Michael
   Czeiszperger <mike@pan.com> and Tim Thompson's 1989 MIDI library,
   specifically the program mftext.  The programs appear to be
   uncopyrighted and redistributable without restriction, but any
   such conditions, should they exist, superceed my own.
   See the midilib package for the original source code.
======================================================================*/

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <fcntl.h>
#include <i386/isa/sblast.h>
#include <sys/time.h>
#include "midifile.h"
#include "getopt.h"

/*#define DPRINTF(x)		printf x */
#define DPRINTF
#define BUFSIZE		256
#define CYCLE		20
#define MAXEVENTS	100000	/* TACKY! */

static char     midi_dev_name[]= "/dev/sb_midi";
static FILE    *in_file;
static int      midi_fd;
long            tempo = 500000;	/* the default tempo is 120 beats/minute */
int             division;	/* from the file header */
int             adjust;
int		force_chan = -1;

#define OPTION_USAGE \
  "+help               Display usage information\n" \
  "+output <filename>  Write output to <filename> instead of /dev/sb_midi\n"\
  "+input <filename>   Read input from <filename> instead of stdin\n"\
  "+adjust <1-10000>   Tempo adjustment.  smaller = faster, 1000 = normal\n"\
  "+chan <1-16>        Force all notes to one specified MIDI channel"

const struct option long_options[]=
{
  {"adjust", 1, NULL, 'a'},
  {"output", 1, NULL, 'o'},
  {"input", 1, NULL, 'i'},
  {"chan", 1, NULL, 'c'},
  {"help", 0, NULL, 'H'},
  {NULL, 0, NULL, 0}
};

struct _event
{
  unsigned long currtime;
  char data[3];
  int bytes;
};
typedef struct _event event;

event array[MAXEVENTS];		/* This will be dynamically alloc'd RSN */

static char    *ttype[]=
{
  NULL,
  "Text Event",			/* type=0x01 */
  "Copyright Notice",		/* type=0x02 */
  "Sequence/Track Name",
  "Instrument Name",		/* ...       */
  "Lyric",
  "Marker",
  "Cue Point",			/* type=0x07 */
  "Unrecognized"
};

static next_event = 0;
void
add_event (int count, sbBYTE data1, sbBYTE data2, sbBYTE data3)
{
  event new_event;

  new_event.currtime = Mf_currtime;
  new_event.bytes = count;
  new_event.data[0] = data1;
  new_event.data[1] = data2;  
  new_event.data[2] = data3;
  array[next_event++] = new_event;
}

void
send (unsigned char data)
{
  write (midi_fd, &data, 1);
}

void
wait_until (unsigned long currtime)
{
  static unsigned long last_event = 0;
  unsigned long   sep;
  unsigned long   usecs;
  struct timeval  tv;

  DPRINTF (("Waiting until %d..\n", currtime));
  if (last_event == 0)
    {
      last_event = currtime;
      return;
    }
  
  sep = (currtime - last_event);
  last_event = currtime;
  if (sep <= 0)
    return;

  if (division > 0)
    usecs = ((sep * tempo) / division) * adjust / 1000;
  else
    usecs = ((sep / (upperbyte (division) * lowerbyte (division)))
	     * adjust / 1000);
  tv.tv_sec = usecs / 1000000;
  tv.tv_usec = usecs % 1000000;
  DPRINTF (("Delay: usecs = %d, tv_sec = %d, tv_usec = %d\n",
	    usecs, tv.tv_sec, tv.tv_usec));
  DPRINTF (("Adjust: %d\n", adjust));
  fflush (stderr);
  select (0, 0, 0, 0, &tv);
}

int
do_trackstart (void)
{
  printf ("Track start\n");
}

int
do_trackend (void)
{
  printf ("Track end\n");
}

int
do_pressure (int chan, int pitch, int press)
{
  printf ("Pressure, chan=%d pitch=%d press=%d\n", chan + 1, pitch, press);
}

int
do_parameter (int chan, int control, int value)
{
  DPRINTF(("Parameter, chan=%d c1=%d c2=%d\n", chan + 1, control, value));
  if (force_chan == -1)
    add_event (3, control_change | chan, control, value);
  else
    add_event (3, control_change | force_chan, control, value);
}

int
do_pitchbend (int chan, int msb, int lsb)
{
  printf ("Pitchbend, chan=%d msb=%d lsb=%d\n", chan + 1, msb, lsb);
}

int
do_program (int chan, int program)
{
  printf ("Program, chan=%d program=%d\n", chan + 1, program);
}

int
do_chanpressure (int chan, int press)
{
  printf ("Channel pressure, chan=%d pressure=%d\n", chan + 1, press);
}

int
do_sysex (int leng, char *mess)
{
  printf ("Sysex, leng=%d\n", leng);
}

int
do_metamisc (int type, int leng, char *mess)
{
  printf ("Meta event, unrecognized, type=0x%02x leng=%d\n", type, leng);
}

int
do_metaspecial (int type, int leng, char *mess)
{
  printf ("Meta event, sequencer-specific, type=0x%02x leng=%d\n", type, leng);
}

int
do_metatext (int type, int leng, char *mess)
{
  int             unrecognized = (sizeof (ttype) / sizeof (char *))- 1;
  register int    n, c;
  register char  *p = mess;

  if (type < 1 || type > unrecognized)
    type = unrecognized;
  printf ("Meta Text, type=0x%02x (%s)  leng=%d\n", type, ttype[type], leng);
  printf ("     Text = <");
  for (n = 0; n < leng; n++)
    {
      c = *p++;
      printf ((isprint (c) || isspace (c)) ? "%c" : "\\0x%02x", c);
    }
  printf (">\n");
}

int
do_metaseq (int num)
{
  printf ("Meta event, sequence number = %d\n", num);
}

int
do_metaeot (void)
{
  printf ("Meta event, end of track\n");
}

int
do_keysig (int sf, int mi)
{
  printf ("Key signature, sharp/flats=%d  minor=%d\n", sf, mi);
}

int
do_timesig (int nn, int dd, int cc, int bb)
{
  int             denom = 1;

  while (dd-- > 0)
    denom *= 2;
  printf ("Time signature=%d/%d "
	  " MIDI-clocks/click=%d "
	  " 32nd-notes/24-MIDI-clocks=%d\n",
	  nn, denom, cc, bb);
}

int
do_smpte (int hr, int mn, int se, int fr, int ff)
{
  printf ("SMPTE, hour=%d minute=%d second=%d frame=%d fract-frame=%d\n",
	  hr, mn, se, fr, ff);
}

int
do_arbitrary (int leng, char *mess)
{
  printf ("Arbitrary bytes, leng=%d\n", leng);
}

int
midigetc (void)
{
  return (getc (in_file));
}

int
error (char    *s)
{
  fprintf (stderr, "Error: %s\n", s);
}

int
do_header (format, ntrks, ldivision)
{
  division = ldivision;
  fprintf (stderr,
	"Header format=%d ntrks=%d division=%d\n", format, ntrks, division);
}

int
do_tempo (int microsecs)
{
  tempo = microsecs;
  DPRINTF (("Tempo changed to %d\n", tempo));
}

int
do_noteon (int chan, int pitch, int vol)
{
  DPRINTF (("Note on, chan=%d pitch=%d vol=%d\n", chan + 1, pitch, vol));
  if (force_chan == -1)
    add_event (3, note_on | chan, pitch, vol);
  else
    add_event (3, note_on | force_chan, pitch, vol);
}

int
do_noteoff (int chan, int pitch, int vol)
{
  DPRINTF (("Note off, chan=%d pitch=%d vol=%d\n", chan + 1, pitch, vol));
  if (force_chan == -1)
    add_event (3, note_off | chan, pitch, vol);
  else
    add_event (3, note_off | force_chan, pitch, vol);
}

int compare (event *one, event *two)
{
  return (one->currtime - two->currtime); 
}

void
main (int argc, char **argv)
{
  int             opt, error_flag, i;
  extern int      optind;
  extern char    *optarg;
  char           *in_name, *out_name;

  adjust = 1000;
  in_name = out_name = NULL;
  error_flag = 0;
  while ((opt = getopt_long_only (argc, argv, "", long_options, NULL)) != EOF)
    {
      switch (opt)
	{
	case 'i':
	  in_name = optarg;
	  break;
	case 'o':
	  out_name = optarg;
	  break;
	case 'a':
	  adjust = atoi (optarg);
	  break;
	case 'c':
	  force_chan = atoi (optarg) - 1;
	  break;
	case 'H':		/* Help */
	default:
	  error_flag++;
	  break;
	}
    }

  if (argv[optind])		/* non-option arguments are filenames */
    {
      in_name = argv[optind];
      if (argv[optind + 1])
	out_name = argv[optind + 1];
    }

  if (in_name)
    {
      if ((in_file = fopen (in_name, "r")) == NULL)
	perror ("Unable to open input file"), exit (1);
    }
  else
    in_file = stdin;

  if (out_name == NULL)
    out_name = midi_dev_name;

  if ((midi_fd = open (out_name, O_TRUNC | O_WRONLY | O_CREAT, 0666)) < 0)
    perror ("Unable to open output file"), exit (1);

  Mf_getc = midigetc;
  Mf_header = do_header;
  Mf_noteon = do_noteon;
  Mf_noteoff = do_noteoff;
  Mf_tempo = do_tempo;
  Mf_error = error;
  Mf_trackstart = do_trackstart;
  Mf_trackend = do_trackend;
  Mf_pressure = do_pressure;
  Mf_parameter = do_parameter;
  Mf_pitchbend = do_pitchbend;
  Mf_program = do_program;
  Mf_chanpressure = do_chanpressure;
  Mf_sysex = do_sysex;
  Mf_metamisc = do_metamisc;
  Mf_seqnum = do_metaseq;
  Mf_eot = do_metaeot;
  Mf_timesig = do_timesig;
  Mf_smpte = do_smpte;
  Mf_keysig = do_keysig;
  Mf_seqspecific = do_metaspecial;
  Mf_text = do_metatext;
  Mf_arbitrary = do_arbitrary;

  if (mfread ()== 1)
    fprintf (stderr, "mfread() returned error!\n");

  printf ("%d events total.\n", next_event);
  qsort (array, next_event, sizeof (event), compare);

  for (i = 0; i < next_event; i++)
    {
      if (i % 20 == 0)
	printf ("Event %d\r", i), fflush (stdout);
      wait_until (array[i].currtime);
      write (midi_fd, array[i].data, array[i].bytes);
    }
  printf ("Done.          \n");
  exit (0);
}
