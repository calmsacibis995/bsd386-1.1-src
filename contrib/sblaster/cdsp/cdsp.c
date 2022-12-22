/*======================================================================
   cdsp: Sound Blaster DSP Control program.
   [ This file is a part of SBlast-BSD-1.5 ]

   Steve Haehnichen <shaehnic@ucsd.edu>
  
   $Id: cdsp.c,v 1.1.1.1 1994/01/03 23:14:23 polk Exp $

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
  "+help              Display usage information\n" \
  "+quiet             Print no output other than errors\n" \
  "+reset             Reset DSP\n" \
  "+bufsize <newsize> Set DSP buffer size\n" \
  "+speed <newspeed>  Set DSP speed in Hz (3906-47619)\n" \
  "+show              Show current DSP speed\n" \
  "+stereo            Set stereo mode\n" \
  "+mono              Set mono mode\n" \
  "+channels 1|2      Set number of channels explicitly\n"

/*
 * Macro to print only if the user doesn't demand silence.
 */
#define PRINT(x)	do { if (!quiet) printf x; } while (0)

/*
 * Send an ioctl to the DSP with error checking.
 */
#define IOCTL(cmd, arg)	\
  do { \
    if (ioctl (dsp, DSP_IOCTL_##cmd, arg) == -1) \
        perror ("ioctl"), exit(1); \
  } while (0)

const struct option long_options[] =
{
  {"help", 		0, NULL, 'H'},
  {"quiet", 		0, NULL, 'Q'},
  {"reset", 		0, NULL, 'R'},
  {"stereo",		0, NULL, 'T'},
  {"mono", 		0, NULL, 'M'},
  {"show", 		0, NULL, 'D'},
  {"speed",		1, NULL, 's'},
  {"bufsize",		1, NULL, 'b'},
  {"channels",		1, NULL, 'c'},
  {NULL, 0, NULL, 0}
};

void 
main (int argc , char *argv[])
{
  int newsize, speed, stereo;
  int opt, error_flag;
  int quiet = 0;
  int dsp;			/* DSP device fd */

  error_flag = 0;
  dsp = open ("/dev/sb_dsp", O_RDONLY, 0);
  if (dsp < 0)
    perror ("open"), exit(1);

  error_flag = 0;
  while ((opt = getopt_long_only (argc, argv, "", long_options, NULL)) != EOF)
    {
      switch (opt)
	{
	case 'H':		/* help */
	  fprintf (stderr, "Usage: %s [options]\n" OPTION_USAGE, argv[0]);
	  break;
	case 'Q':		/* quiet */
	  quiet = 1;
	  break;
	case 'R':		/* reset */
	  PRINT (("Resetting DSP.\n"));
	  IOCTL (RESET, 0);
	  break;
	case 'M':		/* mono */
	  PRINT (("DSP set to Mono.\n"));
	  stereo = 0;
	  IOCTL (STEREO, &stereo);
	  break;
	case 'T':		/* stereo */
	  PRINT (("DSP set to Stereo.\n"));
	  stereo = 1;
	  IOCTL (STEREO, &stereo);
	  break;
	case 'D':		/* show */
	  speed = -1;
	  IOCTL (SPEED, &speed);
	  PRINT (("DSP speed:   %d Hz\n", speed));
	  newsize = -1;
	  IOCTL (BUFSIZE, &newsize);
	  PRINT (("Buffer size: %d bytes\n", newsize));
	  break;
	case 's':		/* speed */
	  speed = atoi (optarg);
	  IOCTL (SPEED, &speed);
	  PRINT (("DSP speed: %d\n", speed));
	  break;
	case 'b':		/* bufsize */
	  newsize = atoi (optarg);
	  IOCTL (BUFSIZE, &newsize);
	  break;
	case 'c':		/* channels */
	  stereo = atoi (optarg) - 1;
	  if (stereo)
	    PRINT (("DSP set to Stereo.\n"));
	  else
	    PRINT (("DSP set to Mono.\n"));
	  IOCTL (STEREO, &stereo);
	  break;

	default:
	  error_flag++;
	  break;
	}
    }

  if (error_flag)
    {
      fprintf (stderr, "%s: Use `-help' for usage information.\n", argv[0]);
      exit (2);
    }

  if (argv[optind])		/* Argument with no flag is just speed */
    {
      speed = atoi (argv[optind]);
      IOCTL (SPEED, &speed);
      PRINT (("DSP speed: %d\n", speed));
      exit(0);
    }      

  if (argc == 1)		/* No arguments displays current speed */
    {
      speed = -1;
      IOCTL (SPEED, &speed);
      PRINT (("DSP speed:   %d Hz\n", speed));
      newsize = -1;
      IOCTL (BUFSIZE, &newsize);
      PRINT (("Buffer size: %d bytes\n", newsize));
      exit(0);
    }
}
