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
#include "ispell.h"
#include "hash.h"

/*
 * compute a 15 bit hash code
 */
unsigned int
hash (p)
  register char *p;
{
  register int acc = 0;
  register unsigned char b0;

  while (1)
    {
      if ((b0 = *p++) == 0)
	break;
      acc <<= 1;
      acc += b0;
      acc += (*p << 8);
      if (acc < 0)
	{
	  acc &= (((unsigned) -1) >> 1);
	  acc++;
	}
      if (*p++ == 0)
	break;
    }

  return (acc & 0x7fff);
}
