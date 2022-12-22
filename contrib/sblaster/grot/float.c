/*======================================================================
   float: convert between floating point and unsigned characters
   [ This file is a part of SBlast-BSD-1.5 ]

   Steve Haehnichen <shaehnic@ucsd.edu>
  
   float.c,v 1.2 1992/09/14 03:29:58 steve Exp

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
#include "getopt.h"

#define BUFFER_SIZE	0x2000

#define OPTION_USAGE \
  "+help                Display usage information\n" \
  "+direct              Send converted data directly to /dev/sb_dsp\n" \
  "+tofloat             Convert to floating point from unsigned bytes\n" \
  "+fromfloat           Convert from floating point to unsigned bytes\n" \
  "+output <filename>   Write results to filename\n"\
  "+input <filename>    Read input from filename \n"
  
const struct option long_options[] =
{
  {"tofloat", 		0, NULL, 't'},
  {"fromfloat", 	0, NULL, 'f'},
  {"direct", 		0, NULL, 'd'},
  {"help", 		0, NULL, 'H'},
  {"output", 		1, NULL, 'o'},
  {"input", 		1, NULL, 'i'},
  {NULL, 		0, NULL, 0}
};

char dsp_dev_name[] = "/dev/sb_dsp";

void main (int argc, char **argv)
{
  float 	floats[BUFFER_SIZE];
  unsigned char chars[BUFFER_SIZE];
  int i, got;
  int tofloat = FALSE;
  int fromfloat = FALSE;
  int in_fd, out_fd;
  char *in_name, *out_name;
  int		opt, error_flag;
  extern int 	optind;
  extern char	*optarg;

  in_name = out_name = NULL;
  in_fd = 0;
  out_fd = 1;
  
  error_flag = 0;
  while ((opt = getopt_long_only (argc, argv, "", long_options, NULL)) != EOF)
    {
      switch (opt)
	{
	case 'd':
	  out_name = dsp_dev_name;
	  break;
	case 't':
	  tofloat = TRUE;
	  break;
	case 'f':		/* quiet */
	  fromfloat = TRUE;
	  break;
	case 'i':
	  in_name = optarg;
	  break;
	case 'o':
	  out_name = optarg;
	  break;
	case 'H':
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

  if (!fromfloat && !tofloat)
    fprintf (stderr, "Convert *from* or *to* floating point?\n"), exit(1);

  if (tofloat)
    while (1)
      {
	got = read (in_fd, chars, BUFFER_SIZE);
	if (got == 0)
	  break;
	
	for (i = 0; i < got; i++)
	  floats[i] = (float)((int) chars[i] - 0x80) / 128.0;
	write (out_fd, floats, sizeof(float) * got);
      }
  else				/* fromfloat */
    while (1)
      {
	got = read (in_fd, floats, BUFFER_SIZE * sizeof(float))
	  / sizeof (float);
	if (got == 0)
	  break;
	
	for (i = 0; i < got; i++)
	  chars[i] = (int)((floats[i] * 128.0) + 0x80);
	write (out_fd, chars, got);
      }
}
