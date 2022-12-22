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
#include <fcntl.h>
#include "ispell.h"
#include "hash.h"
#include "good.h"

int
good (w, len, alllower)
  char *w;
  int len, alllower;
{
  if (len >= MAX_WORD_LEN - 5)
    return (0);

  if (lookup (w))
    return (1);

  if (len == 1)
    return (1);

  if (len < 4)
    return (p_lookup (w, alllower));

  if (sufcheck (w, len))
    return (1);

  return (p_lookup (w, alllower));
}
