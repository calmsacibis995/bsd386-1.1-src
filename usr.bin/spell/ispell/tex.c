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

int
skip_to_next_word_tex (outfile)
     FILE *outfile;
{
  /* tex mode is omitted n this release because:
   * 1. I'm not sure it is a good idea (once you insert tex's
   *    command names, ispell can check their spelling for you)
   * 2. I need to track down the author and get a copyright release.
   */
  return (skip_to_next_word_generic ());
}
