/*======================================================================
   record: Sound Blaster DSP recording program
   [ This file is a part of SBlast-BSD-1.5 ]

   Steve Haehnichen <shaehnic@ucsd.edu>
  
   record.c,v 1.2 1992/09/14 03:30:08 steve Exp

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
#include <math.h>
#include <stdlib.h>
#include <sys/file.h>
#include <i386/isa/sblast.h>
#include "getopt.h"

#define TRUE 1
#define FALSE 0

#define OPTION_USAGE \
  "+help            Display usage information\n" \
  "+quiet           Print no output other than errors\n" \
  "+speed <freq>    Set DSP speed in Hz (3906-44100)\n" \
  "+time <seconds>  Record from DSP for <seconds>\n" \
  "+bytes <count>   Record <count> bytes from DSP\n" \
  "+stereo          Set stereo mode\n" \
  "+mono            Set mono mode\n" \
  "+channels 1|2    Set number of channels explicitly\n"

#define PRINT(x)	do { if (!quiet) printf x; } while (0)
#define IOCTL(cmd, arg)	\
  do { \
    if (ioctl (dsp, DSP_IOCTL_##cmd, arg) == -1) \
        perror ("ioctl"); \
  } while (0)

int 	quiet = 0;		/* Global flag for no stdout */
int	speed, stereo;

const struct option long_options[] =
{
  {"help", 		0, NULL, 'H'},
  {"stereo",		0, NULL, 'T'},
  {"mono", 		0, NULL, 'M'},
  {"quiet", 		0, NULL, 'Q'},
  {"speed",		1, NULL, 's'},
  {"time",		1, NULL, 't'},
  {"bytes",		1, NULL, 'b'},
  {"channels",		1, NULL, 'c'},
  {"output", 		1, NULL, 'o'},
  {NULL, 0, NULL, 0}
};

#define PRINT(x)	do { if (!quiet) printf x; } while (0)
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))
#define BUFFER_SIZE	0x2000

int dsp;			/* DSP device fd */

void 
main (int argc , char *argv[])
{
  int opt, error_flag;
  unsigned char buf[BUFFER_SIZE];
  char *out_name = NULL;
  int out_fd = 1;
  double secs = 0.0;
  long bytes = 0;
  int got;
  int bufsize;

  error_flag = 0;
  dsp = open ("/dev/sb_dsp", O_RDONLY, 0);
  if (dsp < 0)
    perror ("open"), exit(1);

  error_flag = 0;
  while ((opt = getopt_long_only (argc, argv, "", long_options, NULL)) != EOF)
    {
      switch (opt)
	{
	case 'H':
	  fprintf (stderr, "Usage: %s\n" OPTION_USAGE, argv[0]);
	  break;
	case 'Q':		/* quiet */
	  quiet = 1;
	  break;
	case 'M':
	  PRINT (("DSP set to Mono.\n"));
	  stereo = 0;
	  IOCTL (STEREO, &stereo);
	  break;
	case 'T':
	  PRINT (("DSP set to Stereo.\n"));
	  stereo = 1;
	  IOCTL (STEREO, &stereo);
	  break;
	case 't':
	  secs = atof (optarg);
	  if (secs < 0.0)
	    error_flag++;
	  break;
	case 'b':
	  bytes = atol (optarg);
	  break;
	case 'o':
	  out_name = optarg;
	  break;
	case 's':
	  speed = atoi (optarg);
	  IOCTL (SPEED, &speed);
	  PRINT (("DSP speed: %d\n", speed));
	  break;
	case 'c':
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

  if (argv[optind])		/* non-option arguments are time and output */
    {
      secs = atof (argv[optind]);
      if (argv[optind + 1])
	  out_name = argv[optind + 1];
    }

  if (secs != 0.0)
    {
      speed = -1;		/* Read current DSP speed */
      IOCTL (SPEED, &speed);
      bytes = (long)((double)speed * secs); /* Should stereo double this? */
      PRINT (("Sampling %d bytes...\n", bytes));
    }
  
  if (bytes == 0)
    {
      fprintf (stderr, "%s: You must specify a length with +time or +bytes.\n",
	       argv[0]);
      exit(1);
    }
      
  if (out_name)
    if ((out_fd = open (out_name, O_TRUNC | O_WRONLY | O_CREAT, 0666)) < 0)
      perror ("Unable to open output file"), exit (1);

  bufsize = max (DSP_MIN_BUFSIZE, min (DSP_MAX_BUFSIZE, bytes));
  IOCTL (BUFSIZE, &bufsize);
  
  while (bytes)
    {
      got = read (dsp, buf, min(BUFFER_SIZE, bytes));
      if (got == -1)
	perror ("read"), exit(1);
      if (write (out_fd, buf, got) == -1)
	perror ("write"), exit(1);
      bytes -= got;
    }
}
