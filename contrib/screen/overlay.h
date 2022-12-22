/* Copyright (c) 1993
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ****************************************************************
 * $Id: overlay.h,v 1.1 1994/01/29 17:20:37 polk Exp $ FAU
 */

/*
 * This is the overlay structure. It is used to create a seperate
 * layer over the current windows.
 */

struct LayFuncs
{
  void	(*LayProcess) __P((char **, int *));
  void	(*LayAbort) __P((void));
  void	(*LayRedisplayLine) __P((int, int, int, int));
  void	(*LayClearLine) __P((int, int, int));
  int	(*LayRewrite) __P((int, int, int, int));
  void	(*LaySetCursor) __P((void));
  int	(*LayResize) __P((int, int));
  void	(*LayRestore) __P((void));
};

struct layer
{
  struct layer *l_next;
  int	l_block;
  struct LayFuncs *l_layfn;
  char	*l_data;		/* should be void * */
};

#define Process		(*d_layfn->LayProcess)
#define Abort		(*d_layfn->LayAbort)
#define RedisplayLine	(*d_layfn->LayRedisplayLine)
#define ClearLine	(*d_layfn->LayClearLine)
#define Rewrite		(*d_layfn->LayRewrite)
#define SetCursor	(*d_layfn->LaySetCursor)
#define Resize		(*d_layfn->LayResize)
#define Restore		(*d_layfn->LayRestore)

#define LAY_CALL_UP(fn) \
	{ \
	  struct layer *oldlay = d_lay; \
	  d_lay = d_lay->l_next; \
	  d_layfn = d_lay->l_layfn; \
	  fn; \
	  d_lay = oldlay; \
	  d_layfn = d_lay->l_layfn; \
	}

