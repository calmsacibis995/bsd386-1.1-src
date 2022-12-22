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
#ifdef MSDOS
#include <stdlib.h>
#include <io.h>
#endif
#include <fcntl.h>
#include <ctype.h>
#include "ispell.h"
#include "hash.h"
#include "good.h"

#ifndef _toupper
#define _toupper toupper
#endif
#ifndef _tolower
#define _tolower tolower
#endif

void
fixcase (word, c)
  char *word;
  struct sp_corrections *c;
{
  int i;
  int rest_case;
  int first_case;
  char *p;

  if (isupper (word[0]))
    {
      first_case = 1;
      if (isupper (word[1]))
	rest_case = 1;
      else
	rest_case = 0;
    }
  else
    {
      first_case = 0;
      rest_case = 0;
    }
  for (i = 0; i < c->nwords; i++)
    {
      p = c->posbuf[i];
      if (first_case)
	{
	  if (islower (*p))
	    *p = toupper (*p);
	}
      else
	{
	  if (isupper (*p))
	    *p = tolower (*p);
	}
      p++;
      while (*p)
	{
	  if (rest_case)
	    {
	      if (islower (*p))
		*p = toupper (*p);
	    }
	  else
	    {
	      if (isupper (*p))
		*p = tolower (*p);
	    }
	  p++;
	}
    }
}
