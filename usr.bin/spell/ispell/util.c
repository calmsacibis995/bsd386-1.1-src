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

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "ispell.h"


/* Allocate NBYTES bytes of memory */

void *
xmalloc (nbytes)
  int nbytes;
{
  void *tmp = (void *) malloc (nbytes);

  if (!tmp)
    fprintf (stderr, "out of memory: can't allocate %d bytes\n", nbytes);

  return tmp;
}


/* Allocate NBLOCKS of NBYTES bytes of cleared memory */

void *
xcalloc (nblocks, nbytes)
  int nblocks, nbytes;
{
  void *tmp = (void *) xmalloc (nblocks * nbytes);

  bzero (tmp, nblocks * nbytes);
  return tmp;
}
