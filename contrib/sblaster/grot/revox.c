/*======================================================================
   revox: reverse a soundfile
   [ This file is a part of SBlast-BSD-1.5 ]

   Steve Haehnichen <shaehnic@ucsd.edu>
  
   revox.c,v 1.2 1992/09/14 03:30:10 steve Exp

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

#define BUFFER_SIZE	0x4000

#define OPTION_USAGE \
  "[+direct] [+help] [+output filename] [+input filename]\n"
  
const struct option long_options[] =
{
  {"help", 		0, NULL, 'H'},
  {"direct", 		0, NULL, 'd'},
  {"output", 		1, NULL, 'o'},
  {"input", 		1, NULL, 'i'},
  {NULL, 		0, NULL, 0}
};

char dsp_dev_name[] = "/dev/sb_dsp";

int in_fd, out_fd;
unsigned char in[BUFFER_SIZE], out[BUFFER_SIZE];

inline void rev (int count)
{
  int i;
  
  if (read (in_fd, in, count) < count)
      fprintf (stderr, "Huh?  I could't get a full buffer.\n"), exit (1);
  for (i = 0; i < count; i++)
      out[i] = in[count - i - 1];
  write (out_fd, out, count);
}  

void main (int argc, char **argv)
{
  int i;
  off_t len;
  char *in_name, *out_name;
  int		opt, error_flag;
  extern int 	optind;
  extern char	*optarg;

  in_name = out_name = NULL;
  in_fd = 0;
  out_fd = 1;
  error_flag = 0;
  
  error_flag = 0;
  while ((opt = getopt_long_only (argc, argv, "", long_options, NULL)) != EOF)
    {
      switch (opt)
	{
	case 'H':
	  fprintf (stderr, "Usage: %s\n" OPTION_USAGE, argv[0]);
	  break;
	case 'd':
	  out_name = dsp_dev_name;
	  break;
	case 'i':
	  in_name = optarg;
	  break;
	case 'o':
	  out_name = optarg;
	  break;
	default:
	  error_flag = 1;
	}
    }

  if (error_flag)
    {
      fprintf (stderr, "%s: Use `-help' for usage information.\n", argv[0]);
      exit (2);
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

  len = lseek (in_fd, 0, L_XTND); /* Go to end of file */

  for (i = 1; i <= len / BUFFER_SIZE; i++)
    {
      lseek (in_fd, len - (i * BUFFER_SIZE), L_SET);
      rev (BUFFER_SIZE);
    }
  lseek (in_fd, 0, L_SET);
  rev (len % BUFFER_SIZE);
}
