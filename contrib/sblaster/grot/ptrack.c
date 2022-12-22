/*======================================================================
   chew: convert between signed and unsigned bytes
   [ This file is a part of SBlast-BSD-1.5 ]

   Steve Haehnichen <shaehnic@ucsd.edu>
  
   ptrack.c,v 1.2 1992/09/14 03:30:07 steve Exp

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
#include <sys/file.h>
#include <c.h>
#include <i386at/sblast.h>
#include <sys/ioctl.h>
#include "getopt.h"

#define BUFFER_SIZE	20000

#define OPTION_USAGE \
  "+help               Display usage information\n"\
  "+left               Extract left channel only (even)\n"\
  "+right              Extract right channel only (odd)\n"\
  "+direct             Send output directly to /dev/sb_dsp\n"\
  "+output <filename>  Write output to filename\n"\
  "+input <filename>   Read input from filename\n"
  
const struct option long_options[] =
{
  {"help", 		0, NULL, 'H'},
  {"make", 		1, NULL, 'm'},
  {"break", 		1, NULL, 'b'},
  {"direct", 		0, NULL, 'd'},
  {"output", 		1, NULL, 'o'},
  {"input", 		1, NULL, 'i'},
  {"srate", 		1, NULL, 's'},
  {NULL, 		0, NULL, 0}
};

char dsp_dev_name[] = "/dev/sb_dsp";
#define DEF_WINSIZE	500
#define DEF_SRATE	8000

void main (int argc, char **argv)
{
  unsigned char *buf, *p, *q;
  int i, got;
  int in_fd, out_fd;
  char *in_name, *out_name;
  int		opt, error_flag;
  extern int 	optind;
  extern char	*optarg;
  int	winsize = DEF_WINSIZE;
  int diff, sum;
  int srate = DEF_SRATE;
  int overrun_errno = 0;

  in_name = out_name = NULL;
  in_fd = 0;
  out_fd = 1;
  
  error_flag = 0;
  while ((opt = getopt_long_only (argc, argv, "", long_options, NULL)) != EOF)
    {
      switch (opt)
	{
	case 'd':		/* direct */
	  out_name = dsp_dev_name;
	  break;
	  /*	case 'm':
		make = atoi (optarg);
		break; 
		case 'b':
		space = atoi (optarg);
		break;*/
	case 'i':		/* input */
	  in_name = optarg;
	  break;
	case 's':		/* srate */
	  srate = atoi (optarg);
	  break;
	case 'o':		/* output */
	  out_name = optarg;
	  break;
	case 'H':		/* help */
	default:
	  error_flag++;
	  break;
	}
    }

  if (error_flag)
    {
      fprintf (stderr, "Usage: %s [options]\n" OPTION_USAGE, argv[0]);
      exit(1);
    }
  
  if (argv[optind])		/* non-option arguments are filenames */
    {
      in_name = argv[optind];
      if (argv[optind + 1])
	out_name = argv[optind + 1];
    }

  if (in_name)
    if ((in_fd = open (in_name, O_RDONLY, 0)) < 0)
      perror ("Unable to open input file"), exit (1);

  if (out_name)
    if ((out_fd = open (out_name, O_TRUNC | O_WRONLY | O_CREAT, 0666)) < 0)
      perror ("Unable to open output file"), exit (1);

  if (ioctl (in_fd, DSP_IOCTL_BUFSIZE, &winsize) == -1)
    perror ("ioctl"), exit (1);
  if (ioctl (in_fd, DSP_IOCTL_OVERRUN_ERRNO, &overrun_errno) == -1)
    perror ("ioctl"), exit (1);
  buf = (char *) malloc (winsize * 2);
  q = buf + winsize;

#define STEP 1
#define ERROR_PER 2
  while (got = read (in_fd, buf, winsize * 2))
    {
      if (got < winsize * 2)
	break;
      
      p = q;
      p -= 2 * STEP;
      while (p > buf)
	{
	  sum = 0;
	  p -= STEP;
	  for (i = 0; i < winsize; i++)
	    {
	      diff = q[i] - p[i];
	      if (diff < 0)
		sum -= diff;
	      else
		sum += diff;
	    }
	  /* printf ("Frame %3d: %d\n", p - buf, sum); */
	  if (sum < ERROR_PER * winsize)
	    {
	      printf ("** LEN: %3d  %7.2f Hz  Err: %d\n",
		      q - p, (float)srate / (q - p), sum);
	      break;
	    }
	}
    }
}
