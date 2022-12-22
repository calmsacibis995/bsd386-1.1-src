/*======================================================================
   mixer:  A semi-clone of CL's SBP-SET program, for BSD Unix.
   [ This file is a part of SBlast-BSD-1.5 ]

   Steve Haehnichen <shaehnic@ucsd.edu>

   mixer.c,v 1.2 1992/09/14 03:30:03 steve Exp

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
#include <string.h>
#include <sys/file.h>
#include <i386/isa/sblast.h>
#include "getopt.h"

#define OPTION_USAGE \
  "(Options may be preceeded by - or + and can be uniquely abbreviated.)\n" \
  "+help                    Display usage information\n" \
  "+quiet                   Print no output other than errors\n" \
  "+reset                   Reset mixer\n" \
  "+show                    Show current settings\n" \
  "+master 0-15             Master volume control\n" \
  "+master-left 0-15        Master volume control (left only)\n" \
  "+master-right 0-15       Master volume control (right only)\n" \
  "+line 0-15               Line-in volume control\n" \
  "+line-left 0-15          Line-in volume control (left only)\n" \
  "+line-right 0-15         Line-in volume control (right only)\n" \
  "+voice 0-15              Voice volume control (also +dsp)\n" \
  "+voice-left 0-15         Voice volume control (left only)\n" \
  "+voice-right 0-15        Voice volume control (right only)\n" \
  "+fm 0-15                 FM volume control\n" \
  "+fm-left 0-15            FM volume control (left only)\n" \
  "+fm-right 0-15           FM volume control (right only)\n" \
  "+cd 0-15                 CD volume control\n" \
  "+cd-left 0-15            CD volume control (left only)\n" \
  "+cd-right 0-15           CD volume control (right only)\n" \
  "+microphone 0-7          Microphone mixing level\n" \
  "+filter-level LOW/HIGH   Recording frequency\n" \
  "+source MIC/CD/LINE      Recording source (also +src)\n" \
  "+input-filter ON/OFF     Filter input signal\n" \
  "+output-filter ON/OFF    Filter output signal\n"

#define PRINT(x)	do { if (!quiet) printf x; } while (0)
int 	quiet = 0;		/* Global flag for no stdout */
enum { ANFI, DNFI };

/*
 * Handy hairy macro for sending an ioctl to the Mixer with error
 * checking.  It also reads in the current settings if there was
 * an error, so you are guaranteed to have good data then.
 */
#define IOCTL(cmd, arg)	\
  do { \
    if (ioctl (mix, MIXER_IOCTL_##cmd, arg) == -1) \
      { \
        perror ("ioctl"); \
        ioctl (mix, MIXER_IOCTL_READ_LEVELS, &l); \
        ioctl (mix, MIXER_IOCTL_READ_PARAMS, &p); \
      } \
  } while (0)

const struct option long_options[] =
{
  {"help", 		0, NULL, 'H'},
  {"quiet", 		0, NULL, 'Q'},
  {"reset", 		0, NULL, 'R'},
  {"show", 		0, NULL, 'S'},
  {"m", 		1, NULL, 'm'},
  {"master", 		1, NULL, 'm'},
  {"master-left", 	1, NULL, 'l'},
  {"master-right", 	1, NULL, 'r'},
  {"l", 		1, NULL, 'n'},
  {"line", 		1, NULL, 'n'},
  {"line-left", 	1, NULL, '1'},
  {"line-right", 	1, NULL, '2'},
  {"fm", 		1, NULL, 'f'},
  {"fm-left", 		1, NULL, '3'},
  {"fm-right", 		1, NULL, '4'},
  {"dac", 		1, NULL, 'v'},
  {"dsp", 		1, NULL, 'v'},
  {"dsp-left",		1, NULL, '6'},
  {"dsp-right",		1, NULL, '7'},
  {"voice", 		1, NULL, 'v'},
  {"voice-left",	1, NULL, '6'},
  {"voice-right",	1, NULL, '7'},
  {"cd", 		1, NULL, 'c'},
  {"cd-left", 		1, NULL, 'd'},
  {"cd-right", 		1, NULL, 'e'},
  {"microphone", 	1, NULL, 'k'},
  {"filter-level",	1, NULL, '5'},
  {"source", 		1, NULL, 's'},
  {"src", 		1, NULL, 's'},
  {"input-filter", 	1, NULL, 'i'},
  {"output-filter", 	1, NULL, 'o'},
  {NULL, 0, NULL, 0}
};

struct sb_mixer_levels l;	/* Mixer level settings */
struct sb_mixer_params p;	/* Mixer parameter settings */
int mix;			/* Mixer device fd */

/*
 * Show a stereo volume level.
 * If Left & Right are different, print both.
 */
void show_stereo_vol (char *name, struct stereo_vol vol)
{
  if (vol.l == vol.r)
    PRINT (("%s volume: %d\n", name, vol.l));
  else
    {
      PRINT (("%s-left volume: %d\n", name, vol.l));
      PRINT (("%s-right volume: %d\n", name, vol.r));
    }
}

/*
 * Display all current Mixer settings.
 */
void
show_mixer_settings (void)
{
  PRINT (("Recording input filter: %s\n",
	  p.filter_input ? "ON" : "OFF"));
  PRINT (("Playback output filter: %s\n",
	  p.filter_output ? "ON" : "OFF"));
  PRINT (("Recording filter frequency: %s\n",
	  p.hifreq_filter ? "HI" : "LOW"));
  PRINT (("Recording source: "));
  switch (p.record_source)
    {
    case SRC_MIC:
      PRINT (("MIC\n")); break;
    case SRC_CD:
      PRINT (("CD\n")); break;
    case SRC_LINE:
      PRINT (("LINE\n")); break;
    default:
      PRINT (("Bad source! (%x)\n", p.record_source)); break;
    }
  PRINT (("Microphone volume: %d\n", l.mic));
  show_stereo_vol ("CD", l.cd);
  show_stereo_vol ("Master", l.master);
  show_stereo_vol ("Line", l.line);
  show_stereo_vol ("FM", l.fm);
  show_stereo_vol ("DSP Voice", l.voc);
}

/*
 * Set the mixer filter level, given the user's argument.
 */
void 
set_filter_level (char *arg)
{
  if (arg[0] == 'l' || arg[0] == 'L')
    {
      PRINT (("Low frequency recording\n"));
      p.hifreq_filter = 0;
    }
  else if (arg[0] == 'h' || arg[0] == 'H')
    {
      PRINT (("High frequency recording\n"));
      p.hifreq_filter = 1;
    }
  else
    {
      fprintf (stderr, "Frequency setting must be LOW or HI.\n");
      return;
    }
  IOCTL (SET_PARAMS, &p);
}

/*
 * Set the Recording signal source, given the users argument.
 */
void 
set_source (char *arg)
{
  switch (arg[0])
    {
    case 'm':
    case 'M':
      PRINT (("Recording source is Microphone\n"));
      p.record_source = SRC_MIC;
      break;
    case 'c':
    case 'C':
      PRINT (("Recording source is CD\n"));
      p.record_source = SRC_CD;
      break;
    case 'l':
    case 'L':
      PRINT (("Recording source is Line-in\n"));
      p.record_source = SRC_LINE;
      break;
    default:
      fprintf (stderr, "Recording source must be MIC, CD, or LINE\n");
      return;
    }
  IOCTL (SET_PARAMS, &p);
}

/*
 * Set a filter on or off, given the user's argument,
 * and which filter (ANFI or DNFI) to change.
 */
void 
set_filt (char *arg, int io)
{
  int toggle;
  
  if (arg[0] == '1' || arg[1] == 'n' || arg[1] == 'N') /* ON */
    toggle = 1;
  else if (arg[0] == '0' || arg[1] == 'f' || arg[1] == 'F') /* OFF */
    toggle = 0;
  else 
    {
      fprintf (stderr, "Filter setting must be ON (1) or OFF (0).\n");
      return; 
    }
  PRINT (("%s %s filter\n",
	  (io == ANFI) ? "Recording input" : "Voice output",
	  toggle ? "through" : "bypass"));
  if (io == ANFI)
    p.filter_input = toggle;	/* ANFI input filter */
  else
    p.filter_output = toggle;
  IOCTL (SET_PARAMS, &p);
}

/*
 * Main program actually does little more than option parsing.
 */
void 
main (int argc , char *argv[])
{
  int opt, error_flag;

  mix = open ("/dev/sb_mixer", O_RDWR, 0);
  if (mix < 0)
    perror ("open");

  if (ioctl (mix, MIXER_IOCTL_READ_LEVELS, &l) < 0
      || ioctl (mix, MIXER_IOCTL_READ_PARAMS, &p) < 0)
    perror ("mixer ioctl");
  
  error_flag = 0;
  while ((opt = getopt_long_only (argc, argv, "", long_options, NULL)) != EOF)
    {
      switch (opt)
	{
	case 'H':
	  fprintf (stderr, "Usage: %s [options]\n" OPTION_USAGE, argv[0]);
	  break;
	case 'Q':		/* quiet */
	  quiet = 1;
	  break;
	case 'R':		/* reset */
	  PRINT (("Resetting Mixer.\n"));
	  IOCTL (RESET, 0);
	  IOCTL (READ_LEVELS, &l);
	  IOCTL (READ_PARAMS, &p);
	  break;
	case 'S':
	  show_mixer_settings();
	  break;
	case 'm':		/* master */
	  l.master.l = l.master.r = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("Master volume: %d\n", l.master.l));
	  break;
	case 'l':		/* master-left */
	  l.master.l = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("Master left: %d\n", l.master.l));
	  break;
	case 'r':		/* master-right */
	  l.master.r = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("Master right: %d\n", l.master.r));
	  break;
	case 'n':		/* line */
	  l.line.l = l.line.r = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("Line-in volume: %d\n", l.line.l));
	  break;
	case '1':		/* line-left */
	  l.line.l = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("Line-in left: %d\n", l.line.l));
	  break;
	case '2':		/* line-right */
	  l.line.r = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("Line-in left: %d\n", l.line.r));
	  break;
	case 'f':		/* fm */
	  l.fm.r = l.fm.l = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("FM Volume: %d\n", l.fm.l));
	  break;
	case '3':		/* fm-left */
	  l.fm.l = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("FM left: %d\n", l.fm.l));
	  break;
	case '4':		/* fm-right */
	  l.fm.r = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("FM right: %d\n", l.fm.r));
	  break;
	case 'v':		/* voice */
	  l.voc.l = l.voc.r = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("Voice volume: %d\n", l.voc.l));
	  break;
	case '6':		/* voice-left */
	  l.voc.l = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("Voice left: %d\n", l.voc.l));
	  break;
	case '7':		/* voice-right */
	  l.voc.r = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("Voice right: %d\n", l.voc.r));
	  break;
	case 'c':		/* cd */
	  l.cd.r = l.cd.l = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("CD Volume: %d\n", l.cd.l));
	  break;
	case 'd':		/* cd-left */
	  l.cd.l = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("CD left: %d\n", l.cd.l));
	  break;
	case 'e':		/* cd-right */
	  l.cd.r = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("CD right: %d\n", l.cd.r));
	  break;
	case 'k':		/* microphone */
	  l.mic = atoi (optarg);
	  IOCTL (SET_LEVELS, &l);
	  PRINT (("Microphone volume: %d\n", l.mic));
	  break;
	case '5':		/* filter-level */
	  set_filter_level (optarg);
	  break;
	case 's':		/* source */
	  set_source (optarg);
	  break;
	case 'i':		/* input-filter */
	  set_filt (optarg, ANFI);
	  break;
	case 'o':		/* output-filter */
	  set_filt (optarg, DNFI);
	  break;
	default:
	  error_flag++;
	  break;
	}
    }

  if (error_flag || argv[optind])
    {
      fprintf (stderr, "%s: Use `-help' for usage information.\n", argv[0]);
      exit (2);
    }
  if (argc == 1)		/* "mixer" alone shows current settings */
    {
      show_mixer_settings();
      exit(0);
    }
}
