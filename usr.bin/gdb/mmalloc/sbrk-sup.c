/* Support for sbrk() regions.
   Copyright 1992 Free Software Foundation, Inc.
   Contributed by Fred Fish at Cygnus Support.   fnf@cygnus.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#include "mmalloc.h"

extern PTR sbrk ();

/* The mmalloc() package can use a single implicit malloc descriptor
   for mmalloc/mrealloc/mfree operations which do not supply an explicit
   descriptor.  For these operations, sbrk() is used to obtain more core
   from the system, or return core.  This allows mmalloc() to provide
   backwards compatibility with the non-mmap'd version. */

struct mdesc *__mmalloc_default_mdp;

/* Initialize the default malloc descriptor if this is the first time
   a request has been made to use the default sbrk'd region. */

struct mdesc *
__mmalloc_sbrk_init ()
{
  PTR base;

  base = sbrk (0);
  __mmalloc_default_mdp = (struct mdesc *) sbrk (sizeof (struct mdesc));
  memset ((char *) __mmalloc_default_mdp, 0, sizeof (struct mdesc));
  __mmalloc_default_mdp -> morecore = __mmalloc_sbrk_morecore;
  __mmalloc_default_mdp -> base = base;
  __mmalloc_default_mdp -> breakval = __mmalloc_default_mdp -> top = sbrk (0);
  __mmalloc_default_mdp -> fd = -1;
  return (__mmalloc_default_mdp);
}

PTR
__mmalloc_sbrk_morecore (mdp, size)
  struct mdesc *mdp;
  int size;
{
  PTR result = NULL;

  if ((result = sbrk (size)) != NULL)
    {
      mdp -> breakval += size;
      mdp -> top += size;
    }
  return (result);
}

