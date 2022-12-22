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
#include <ctype.h>
#include "ispell.h"
#include "hash.h"
#include "good.h"
#include "build.h"
#include "charset.h"

#ifdef FPROTO
int d_ending (char *, int);
int t_ending (char *, int);
int r_ending (char *, int);
int g_ending (char *, int);
int h_ending (char *, int);
int s_ending (char *, int);
int n_ending (char *, int);
int e_ending (char *, int);
int y_ending (char *, int);
#else
int d_ending ();
int t_ending ();
int r_ending ();
int g_ending ();
int h_ending ();
int s_ending ();
int n_ending ();
int e_ending ();
int y_ending ();
#endif

struct hash_table_entry null_hte;

static entering = 0;

void
reapone (n)
  int n;
{
  int i;
  entering = 1;
  for (i = 0; i < n; i++)
    {
      if (lookup (words[i]) ||
	  sufcheck (words[i], strlen (words[i])))
	{
	  words[i][0] = 0;
	}
    }
  entering = 0;
}

void
reap (h)
     hash_index h;
{
  struct hash_table_entry hte;
  int i, n;
  char *entry;

  hash_retrieve (h, &hte);
  if (hte.next >= HASH_SPECIAL && hte.next != HASH_END)
    return;
  null_hte.next = hte.next;
  hash_store (h, &null_hte);

  entry = htetostr (&hte, h);
  n = expand (entry);

  reapone (n);

  for (i = 0; i < n; i++)
    if (words[i][0])
      break;

  if (i != n)
    {
      /* still some word that wasn't taken care of */
      hash_store (h, &hte);
    }
}

void
reapall ()
{
  hash_index h;
  for (h = 1; h < hashsize; h++)
    reap (h);
}

hash_index
lookup (w)
     char *w;
{
  comp_char outbuf[MAX_WORD_LEN];
  int len;
  hash_index h;

  len = lexword (w, strlen (w), outbuf);

  if (len == 0)
    return (0);

  h = hash_lookup (outbuf, len);
  return (h);
}

void
add_flag (h, flag)
  hash_index h;
  unsigned short flag;
{
  store_flags (h, get_flags (h) | flag);
}


int
sufcheck (w, len)
  char *w;
  int len;
{
  char w1[MAX_WORD_LEN];
  char *p, *q;
  int n;
  int c;

  if (len < 4)
    return (0);

  for (p = w, q = w1, n = len; n-- && *p; p++, q++)
    {
      c = charset[(unsigned int) *p].lowercase;
      if (c)
	*q = c;
      else
	*q = *p;
    }
  *q = 0;
  switch (w1[len - 1])
    {
    case 'd':
      return (d_ending (w1, len));
    case 't':
      return (t_ending (w1, len));
    case 'r':
      return (r_ending (w1, len));
    case 'g':
      return (g_ending (w1, len));
    case 'h':
      return (h_ending (w1, len));
    case 's':
      return (s_ending (w1, len));
    case 'n':
      return (n_ending (w1, len));
    case 'e':
      return (e_ending (w1, len));
    case 'y':
      return (y_ending (w1, len));
    default:
      return (0);
    }
}

/* FOR CREATING, FIXING */
int
g_ending (w, len)
  char *w;
  int len;
{
  char *p;
  hash_index h;

  p = w + len - 3;		/* if the word ends in 'ing', then *p == 'i' */

  if (p[0] != 'i')
    return (0);
  if (p[1] != 'n')
    return (0);
  if (p[2] != 'g')
    return (0);

  p[0] = 'e';			/* change I to E, like in CREATING */
  p[1] = 0;
  len -= 2;

  if (len < 2)
    return (0);

  if ((h = lookup (w)))
    {
      if (entering)
	add_flag (h, G_FLAG);
      if (get_flags (h) & G_FLAG)
	return (2);
    }

  p[0] = 0;
  len -= 1;

  if (len < 2)
    return (0);

  if (p[-1] == 'e')
    return (0);			/* this stops CREATEING */

  if ((h = lookup (w)))
    {
      if (entering)
	add_flag (h, G_FLAG);
      if (get_flags (h) & G_FLAG)
	return (2);
    }
  return (0);
}

/* FOR CREATED, IMPLIED, CROSSED */
int
d_ending (w, len)
  char *w;
  int len;
{
  char *p;
  hash_index h;

  p = w + len - 2;

  if (p[0] != 'e')
    return (0);
  if (p[1] != 'd')
    return (0);

  p[1] = 0;			/* kill 'd' */
  len--;

  /* like CREATED */
  if ((h = lookup (w)))
    {
      if (entering)
	add_flag (h, D_FLAG);
      if (get_flags (h) & D_FLAG)
	return (2);
    }

  if (len < 3)
    return (0);

  p[0] = 0;
  p--;

  /* ED is now completely gone */

  if (p[0] == 'i' && !isvowel (p[-1]))
    {
      p[0] = 'y';
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, D_FLAG);
	  if (get_flags (h) & D_FLAG)
	    return (2);
	}

      p[0] = 'i';
    }

  if ((p[0] != 'e' && p[0] != 'y') ||
      (p[0] == 'y' && isvowel (p[-1])))
    {
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, D_FLAG);
	  if (get_flags (h) & D_FLAG)
	    return (2);
	}
    }

  return (0);
}

/* FOR LATEST, DIRTIEST, BOLDEST */
int
t_ending (w, len)
  char *w;
  int len;
{
  char *p;
  hash_index h;

  p = w + len - 3;

  if (p[0] != 'e')
    return (0);
  if (p[1] != 's')
    return (0);
  if (p[2] != 't')
    return (0);

  p[1] = 0;			/* kill 's' */
  len -= 2;

  if ((h = lookup (w)))
    {
      if (entering)
	add_flag (h, T_FLAG);
      if (get_flags (h) & T_FLAG)
	return (2);
    }

  if (len < 3)
    return (0);

  p[0] = 0;
  p--;

  /* EST is now completely gone */

  if (p[0] == 'i' && !isvowel (p[-1]))
    {
      p[0] = 'y';
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, T_FLAG);
	  if (get_flags (h) & T_FLAG)
	    return (2);
	}
      return (0);
    }

  if ((p[0] != 'e' && p[0] != 'y') ||
      (p[0] == 'y' && isvowel (p[-1])))
    {
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, T_FLAG);
	  if (get_flags (h) & T_FLAG)
	    return (2);
	}
    }
  return (0);
}

/* FOR LATER, DIRTIER, BOLDER */
int
r_ending (w, len)
  char *w;
  int len;
{
  char *p;
  hash_index h;

  p = w + len - 2;

  if (p[0] != 'e')
    return (0);
  if (p[1] != 'r')
    return (0);

  p[1] = 0;			/* kill 'r' */
  len--;

  if ((h = lookup (w)))
    {
      if (entering)
	add_flag (h, R_FLAG);
      if (get_flags (h) & R_FLAG)
	return (2);
    }

  if (len < 3)
    return (0);

  p[0] = 0;
  p--;

  /* ER is now completely gone */

  if (p[0] == 'i' && !isvowel (p[-1]))
    {
      p[0] = 'y';
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, R_FLAG);
	  if (get_flags (h) & R_FLAG)
	    return (2);
	}
      return (0);
    }

  if ((p[0] != 'e' && p[0] != 'y') ||
      (p[0] == 'y' && isvowel (p[-1])))
    {
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, R_FLAG);
	  if (get_flags (h) & R_FLAG)
	    return (2);
	}
    }
  return (0);
}

/* FOR HUNDREDTH, TWENTIETH */
int
h_ending (w, len)
  char *w;
  int len;
{
  char *p;
  hash_index h;

  p = w + len - 2;

  if (p[0] != 't')
    return (0);
  if (p[1] != 'h')
    return (0);

  *p = 0;
  p -= 2;

  if (p[0] == 'i' && p[1] == 'e')
    {
      p[0] = 'y';
      p[1] = 0;
    }
  else
    {
      if (p[1] == 'y')
	return (0);		/* stops myth -> my/h */
    }

  if ((h = lookup (w)))
    {
      if (entering)
	add_flag (h, H_FLAG);
      if (get_flags (h) & H_FLAG)
	return (2);
    }

  return (0);
}

/*
 * check for flags: X, J, Z, S, P, M
 *
 * X	-ions or -ications or -ens
 * J	-ings
 * Z	-ers or -iers
 * S	-ies or -es or -s
 * P	-iness or -ness
 * M	-'s
 */

/* FOR ALL SORTS OF THINGS ENDING IN S */
int
s_ending (w, len)
  char *w;
  int len;
{
  char *p;
  hash_index h;

  p = w + len;

  /* len >= 4 if we get here at all */

  if (p[-2] == '\'')
    {
      p[-2] = 0;
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, M_FLAG);
	  if (get_flags (h) & M_FLAG)
	    return (2);
	}
      return (0);
    }
  if (!issxzhy (p[-2]))
    {
      /* see if it was simple adding S */
      p[-1] = 0;		/* kill S */
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, S_FLAG);
	  if (get_flags (h) & S_FLAG)
	    return (2);
	}
    }

  if (p[-2] == 'y')
    {
      if (!isvowel (p[-3]))
	return (0);
      p[-1] = 0;
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, S_FLAG);
	  if (get_flags (h) & S_FLAG)
	    return (2);
	}
      return (0);
    }

  if (p[-2] == 'e')
    {
      if (issxzh (p[-3]))
	{
	  p[-2] = 0;

	  if ((h = lookup (w)))
	    {
	      if (entering)
		add_flag (h, S_FLAG);
	      if (get_flags (h) & S_FLAG)
		return (2);
	    }

	  return (0);
	}

      if (p[-3] == 'i')
	{
	  if (isvowel (p[-4]))
	    return (0);
	  p[-3] = 'y';
	  p[-2] = 0;
	  if ((h = lookup (w)))
	    {
	      if (entering)
		add_flag (h, S_FLAG);
	      if (get_flags (h) & S_FLAG)
		return (2);
	    }
	}
      return (0);
    }

  if (p[-2] == 's')
    {
      if (p[-4] != 'n' || p[-3] != 'e')
	return (0);
      if (len < 5)
	return (0);
      p[-4] = 0;		/* zap NESS */
      if (p[-5] == 'i' && !isvowel (p[-6]))
	{
	  p[-5] = 'y';
	  if ((h = lookup (w)))
	    {
	      if (entering)
		add_flag (h, P_FLAG);
	      if (get_flags (h) & P_FLAG)
		return (2);
	      return (0);
	    }
	  p[-5] = 'i';
	}

      if (p[-5] == 'y' && !isvowel (p[-6]))
	return (0);		/* stops shyness -> shy/p */

      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, P_FLAG);
	  if (get_flags (h) & P_FLAG)
	    return (2);
	}

      return (0);
    }

  if (p[-2] == 'r')
    {
      if (p[-3] != 'e')
	return (0);
      p[-2] = 0;
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, Z_FLAG);
	  if (get_flags (h) & Z_FLAG)
	    return (2);
	}

      if (p[-4] != 'e')
	{
	  if (p[-4] == 'i' && !isvowel (p[-5]))
	    p[-4] = 'y';
	  p[-3] = 0;
	  if ((h = lookup (w)))
	    {
	      if (entering)
		add_flag (h, Z_FLAG);
	      if (get_flags (h) & Z_FLAG)
		return (2);
	    }
	}
      return (0);
    }

  if (p[-2] == 'g')
    {
      if (p[-4] != 'i' || p[-3] != 'n')
	return (0);
      p[-4] = 'e';
      p[-3] = 0;
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, J_FLAG);
	  if (get_flags (h) & J_FLAG)
	    return (2);
	  return (0);
	}

      if (len < 5)
	return (0);
      if (p[-5] == 'e')
	return (0);
      p[-4] = 0;

      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, J_FLAG);
	  if (get_flags (h) & J_FLAG)
	    return (2);
	}
      return (0);
    }

  if (p[-2] == 'n')
    {
      if (p[-3] == 'e' && p[-4] != 'e' && p[-4] != 'y')
	{
	  p[-3] = 0;
	  if ((h = lookup (w)))
	    {
	      if (entering)
		add_flag (h, X_FLAG);
	      if (get_flags (h) & X_FLAG)
		return (2);
	    }
	  return (0);
	}

      if (p[-4] != 'i' || p[-3] != 'o')
	return (0);
      p[-4] = 'e';
      p[-3] = 0;
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, X_FLAG);
	  if (get_flags (h) & X_FLAG)
	    return (2);
	}

      if (len < 8)
	return (0);
      if (p[-8] != 'i')
	return (0);
      if (p[-7] != 'c')
	return (0);
      if (p[-6] != 'a')
	return (0);
      if (p[-5] != 't')
	return (0);

      p[-8] = 'y';
      p[-7] = 0;
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, X_FLAG);
	  if (get_flags (h) & X_FLAG)
	    return (2);
	}
    }
  return (0);
}

/* TIGHTEN, CREATION, MULIPLICATION */
/* only the N flag */
int
n_ending (w, len)
  char *w;
  int len;
{
  char *p;
  hash_index h;

  p = w + len;

  if (p[-2] == 'e')
    {
      if (p[-3] == 'e' || p[-3] == 'y')
	return (0);
      p[-2] = 0;
      len -= 2;
      if ((h = lookup (w)))
	{
	  if (entering)
	    add_flag (h, N_FLAG);
	  if (get_flags (h) & N_FLAG)
	    return (2);
	}
      return (0);
    }

  if (p[-3] != 'i')
    return (0);
  if (p[-2] != 'o')
    return (0);
  if (p[-1] != 'n')
    return (0);

  p[-3] = 'e';
  p[-2] = 0;
  len -= 2;

  if ((h = lookup (w)))
    {
      if (entering)
	add_flag (h, N_FLAG);
      if (get_flags (h) & N_FLAG)
	return (2);

      return (0);
    }

  /* really against ICATION */
  if (p[-7] != 'i')
    return (0);
  if (p[-6] != 'c')
    return (0);
  if (p[-5] != 'a')
    return (0);
  if (p[-4] != 't')
    return (0);
  if (p[-3] != 'e')
    return (0);

  p[-7] = 'y';
  p[-6] = 0;
  len -= 4;

  if ((h = lookup (w)))
    {
      if (entering)
	add_flag (h, N_FLAG);
      if (get_flags (h) & N_FLAG)
	return (2);
    }

  return (0);
}

/* FOR CREATIVE, PREVENTIVE */
/* flags: v */
int
e_ending (w, len)
  char *w;
  int len;
{
  char *p;
  hash_index h;

  p = w + len;

  if (p[-3] != 'i')
    return (0);
  if (p[-2] != 'v')
    return (0);
  if (p[-1] != 'e')
    return (0);

  p[-3] = 'e';
  p[-2] = 0;
  len -= 2;

  if ((h = lookup (w)))
    {
      if (entering)
	add_flag (h, V_FLAG);
      if (get_flags (h) & V_FLAG)
	return (2);
      return (0);
    }

  if (p[-4] == 'e')
    return (0);

  p[-3] = 0;

  if ((h = lookup (w)))
    {
      if (entering)
	add_flag (h, V_FLAG);
      if (get_flags (h) & V_FLAG)
	return (2);
    }

  return (0);
}

/* FOR QUICKLY */
/* flags: y */
int
y_ending (w, len)
  char *w;
  int len;
{
  char *p;
  hash_index h;

  p = w + len;

  if (p[-2] != 'l')
    return (0);
  if (p[-1] != 'y')
    return (0);

  p[-2] = 0;
  len -= 2;

  if ((h = lookup (w)))
    {
      if (entering)
	add_flag (h, Y_FLAG);
      if (get_flags (h) & Y_FLAG)
	return (2);
    }

  return (0);
}
