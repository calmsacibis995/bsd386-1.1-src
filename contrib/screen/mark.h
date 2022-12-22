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
 * $Id: mark.h,v 1.1 1994/01/29 17:19:38 polk Exp $ FAU
 */

struct markdata
{
  int	cx, cy;		/* cursor Position in WIN coords*/
  int	x1, y1;		/* first mark in WIN coords */
  int	second;		/* first mark dropped flag */
  int	left_mar, right_mar, nonl;
  int	rep_cnt;	/* number of repeats */
  int	append_mode;	/* shall we overwrite or append to copybuffer */
  int	write_buffer;	/* shall we do a KEY_WRITE_EXCHANGE right away? */
  int	hist_offset;	/* how many lines are on top of the screen */
  char	isstr[100];	/* string we are searching for */
  int	isstrl;
  char	isistr[200];	/* string of chars user has typed */
  int	isistrl;
  int	isdir;		/* current search direction */
  int	isstartpos;	/* position where isearch was started */
  int	isstartdir;	/* direction when isearch was started */
};


#define W2D(y) ((y) - markdata->hist_offset)
#define D2W(y) ((y) + markdata->hist_offset)

