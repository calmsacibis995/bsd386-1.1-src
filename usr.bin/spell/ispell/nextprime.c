/* Copyright (C) 1990, 1993 Free Software Foundation, Inc.

   This file is part of GNU ISPELL.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <stdio.h>

unsigned short
nextprime (x)
  unsigned short x;
{
  unsigned short half;
  unsigned short y;

  if (x == 2)
    return (2);

  if (x % 2 == 0)
    x++;

  while (1)
    {
      half = x / 2;
      for (y = 3; y < half; y += 2)
	if (x % y == 0)
	  goto next;
      return (x);
    next:
      x += 2;
      if (x == 1)
	return (0);
    }
}

#if 0
main ()
{
  char buf[100];

  while (gets (buf) != NULL)
    (void) printf ("%u\n", nextprime (atoi (buf)));
}

#endif
