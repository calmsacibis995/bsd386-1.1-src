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
 */

#include "rcs.h"
RCS_ID("$Id: mark.c,v 1.2 1994/01/29 17:18:58 polk Exp $ FAU")

#include <sys/types.h>

#include "config.h"
#include "screen.h"
#include "mark.h"
#include "extern.h"

#ifdef COPY_PASTE

static int  is_letter __P((int));
static void nextword __P((int *, int *, int, int));
static int  linestart __P((int));
static int  lineend __P((int));
static int  rem __P((int, int , int , int , int , char *, int));
static int  eq __P((int, int ));
static int  MarkScrollDownDisplay __P((int));
static int  MarkScrollUpDisplay __P((int));

static void MarkProcess __P((char **, int *));
static void MarkAbort __P((void));
static void MarkRedisplayLine __P((int, int, int, int));
static int  MarkRewrite __P((int, int, int, int));
static void MarkSetCursor __P((void));

extern struct win *fore;
extern struct display *display;
extern char *null, *blank;

#ifdef NETHACK
extern nethackflag;
#endif

static struct LayFuncs MarkLf =
{
  MarkProcess,
  MarkAbort,
  MarkRedisplayLine,
  DefClearLine,
  MarkRewrite,
  MarkSetCursor,
  DefResize,
  DefRestore
};

int join_with_cr =  0;
char mark_key_tab[256]; /* this array must be initialised first! */

static struct markdata *markdata;


/*
 * VI like is_letter: 0 - whitespace
 *                    1 - letter
 *		      2 - other
 */
static int is_letter(c)
char c;
{
  if ((c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '_' || c == '.' ||
      c == '@' || c == ':' ||
      c == '%' || c == '!' ||
      c == '-' || c == '+')
    /* thus we can catch email-addresses as a word :-) */
    return 1;
  else if (c != ' ')
    return 2;
  return 0;
}

static int
linestart(y)
int y;
{
  register int x;
  register char *i;

  for (x = markdata->left_mar, i = iWIN(y) + x; x < d_width - 1; x++)
    if (*i++ != ' ')
      break;
  if (x == d_width - 1)
    x = markdata->left_mar;
  return(x);
}

static int
lineend(y)
int y;
{
  register int x;
  register char *i;

  for (x = markdata->right_mar, i = iWIN(y) + x; x >= 0; x--)
    if (*i-- != ' ')
      break;
  if (x < 0)
    x = markdata->left_mar;
  return(x);
}


/*
 *  nextword calculates the cursor position of the num'th word.
 *  If the cursor is on a word, it counts as the first.
 *  NW_BACK:		search backward
 *  NW_ENDOFWORD:	find the end of the word
 *  NW_MUSTMOVE:	move at least one char
 */

#define NW_BACK		(1<<0)
#define NW_ENDOFWORD	(1<<1)
#define NW_MUSTMOVE	(1<<2)

static void
nextword(xp, yp, flags, num)
int *xp, *yp, flags, num;
{
  int xx = d_width, yy = fore->w_histheight + d_height;
  register int sx, oq, q, x, y;

  x = *xp;
  y = *yp;
  sx = (flags & NW_BACK) ? -1 : 1;
  if ((flags & NW_ENDOFWORD) && (flags & NW_MUSTMOVE))
    x += sx;
  for (oq = -1; ; x += sx, oq = q)
    {
      if (x >= xx || x < 0)
	q = 0;
      else
        q = is_letter(iWIN(y)[x]);
      if (oq >= 0 && oq != q)
	{
	  if (oq == 0 || !(flags & NW_ENDOFWORD))
	    *xp = x;
	  else
	    *xp = x-sx;
	  *yp = y;
	  if ((!(flags & NW_ENDOFWORD) && q) ||
	      ((flags & NW_ENDOFWORD) && oq))
	    {
	      if (--num <= 0)
	        return;
	    }
	}
      if (x == xx)
	{
	  x = -1;
	  if (++y >= yy)
	    return;
	}
      else if (x < 0)
	{
	  x = xx;
	  if (--y < 0)
	    return;
	}
    }
}


/*
 * y1, y2 are WIN coordinates
 *
 * redisplay:	0  -  just copy
 * 		1  -  redisplay + copy
 *		2  -  count + copy, don't redisplay
 */

static int
rem(x1, y1, x2, y2, redisplay, pt, yend)
int x1, y1, x2, y2, redisplay, yend;
char *pt;
{
  int i, j, from, to, ry;
  int l = 0;
  char *im;

  markdata->second = 0;
  if (y2 < y1 || ((y2 == y1) && (x2 < x1)))
    {
      i = y2;
      y2 = y1;
      y1 = i;
      i = x2;
      x2 = x1;
      x1 = i;
    }
  ry = y1 - markdata->hist_offset;
  
  i = y1;
  if (redisplay != 2 && pt == 0 && ry <0)
    {
      i -= ry;
      ry = 0;
    }
  for (; i <= y2; i++, ry++)
    {
      if (redisplay != 2 && pt == 0 && ry > yend)
	break;
      from = (i == y1) ? x1 : 0;
      if (from < markdata->left_mar)
	from = markdata->left_mar;
      for (to = d_width, im = iWIN(i) + to; to >= 0; to--)
        if (*im-- != ' ')
	  break;
      if (i == y2 && x2 < to)
	to = x2;
      if (to > markdata->right_mar)
	to = markdata->right_mar;
      if (redisplay == 1 && from <= to && ry >=0 && ry <= yend)
	MarkRedisplayLine(ry, from, to, 0);
      if (redisplay != 2 && pt == 0)	/* don't count/copy */
	continue;
      for (j = from, im = iWIN(i)+from; j <= to; j++)
	{
	  if (pt)
	    *pt++ = *im++;
	  l++;
	}
      if (i != y2 && (to != d_width - 1 || iWIN(i)[to + 1] == ' '))
	{
	  /* 
	   * this code defines, what glues lines together
	   */
	  switch (markdata->nonl)
	    {
	    case 0:		/* lines separated by newlines */
	      if (join_with_cr)
		{
		  if (pt)
		    *pt++ = '\r';
		  l++;
		}
	      if (pt)
		*pt++ = '\n';
	      l++;
	      break;
	    case 1:		/* nothing to separate lines */
	      break;
	    case 2:		/* lines separated by blanks */
	      if (pt)
		*pt++ = ' ';
	      l++;
	      break;
	    }
	}
    }
  return(l);
}

/* Check if two chars are identical. All digits are treatened
 * as same. Used for GetHistory()
 */

static int
eq(a, b)
int a, b;
{
  if (a == b)
    return 1;
  if (a == 0 || b == 0)
    return 1;
  if (a <= '9' && a >= '0' && b <= '9' && b >= '0')
    return 1;
  return 0;
}


int
GetHistory()	/* return value 1 if u_copybuffer changed */
{
  int i = 0, q = 0, xx, yy, x, y;
  char *linep;

  x = fore->w_x;
  if (x >= d_width)
    x = d_width - 1;
  y = fore->w_y + fore->w_histheight;
  debug2("cursor is at x=%d, y=%d\n", x, y);
  for (xx = x - 1, linep = iWIN(y) + xx; xx >= 0; xx--)
    if ((q = *linep--) != ' ' )
      break;
  debug3("%c at (%d,%d)\n", q, xx, y);
  for (yy = y - 1; yy >= 0; yy--)
    {
      linep = iWIN(yy);
      if (xx < 0 || eq(linep[xx], q))
	{		/* line is matching... */
	  for (i = d_width - 1, linep += i; i >= x; i--)
	    if (*linep-- != ' ')
	      break;
	  if (i >= x)
	    break;
	}
    }
  if (yy < 0)
    return 0;
  if (d_user->u_copybuffer != NULL)
    UserFreeCopyBuffer(d_user);
  if ((d_user->u_copybuffer = malloc((unsigned) (i - x + 2))) == NULL)
    {
      Msg(0, "Not enough memory... Sorry.");
      return 0;
    }
  bcopy(linep - i + x + 1, d_user->u_copybuffer, i - x + 1);
  d_user->u_copylen = i - x + 1;
  return 1;
}

void
MarkRoutine()
{
  int x, y;
 
  ASSERT(fore->w_active);
  if (InitOverlayPage(sizeof(*markdata), &MarkLf, 1))
    return;
  markdata = (struct markdata *)d_lay->l_data;
  markdata->second = 0;
  markdata->rep_cnt = 0;
  markdata->append_mode = 0;
  markdata->write_buffer = 0;
  markdata->nonl = 0;
  markdata->left_mar  = 0;
  markdata->right_mar = d_width - 1;
  markdata->hist_offset = fore->w_histheight;
  x = fore->w_x;
  y = D2W(fore->w_y);
  if (x >= d_width)
    x = d_width - 1;

  GotoPos(x, W2D(y));
#ifdef NETHACK
  if (nethackflag)
    Msg(0, "Welcome to hacker's treasure zoo - Column %d Line %d(+%d) (%d,%d)",
	x + 1, W2D(y + 1), fore->w_histheight, d_width, d_height);
  else
#endif
  Msg(0, "Copy mode - Column %d Line %d(+%d) (%d,%d)",
      x + 1, W2D(y + 1), fore->w_histheight, d_width, d_height);
  markdata->cx = markdata->x1 = x;
  markdata->cy = markdata->y1 = y;
}

static void
MarkSetCursor()
{
  markdata = (struct markdata *)d_lay->l_data;
  fore = d_fore;
  GotoPos(markdata->cx, W2D(markdata->cy));
}

static void
MarkProcess(inbufp,inlenp)
char **inbufp;
int *inlenp;
{
  char *inbuf, *pt;
  int inlen;
  int cx, cy, x2, y2, j, yend;
  int newcopylen = 0, od;
  int in_mark;
  int rep_cnt;
 
/*
  char *extrap = 0, extrabuf[100];
*/
      
  markdata = (struct markdata *)d_lay->l_data;
  fore = d_fore;
  if (inbufp == 0)
    {
      MarkAbort();
      return;
    }
 
  GotoPos(markdata->cx, W2D(markdata->cy));
  inbuf= *inbufp;
  inlen= *inlenp;
  pt = inbuf;
  in_mark = 1;
  while (in_mark && (inlen /* || extrap */))
    {
/*
      if (extrap)
	{
	  od = *extrap++;
	  if (*extrap == 0)
	    extrap = 0;
	}
      else
*/
	{
          od = mark_key_tab[(unsigned int)*pt++];
          inlen--;
	}
      rep_cnt = markdata->rep_cnt;
      if (od >= '0' && od <= '9')
        {
	  if (rep_cnt < 1001 && (od != '0' || rep_cnt != 0))
	    {
	      markdata->rep_cnt = 10 * rep_cnt + od - '0';
	      continue;
 	      /*
	       * Now what is that 1001 here? Well, we have a screen with
	       * 25 * 80 = 2000 characters. Movement is at most across the full
	       * screen. This we do with word by word movement, as character by
	       * character movement never steps over line boundaries. The most words
	       * we can place on the screen are 1000 single letter words. Thus 1001
	       * is sufficient. Users with bigger screens never write in single letter
	       * words, as they should be more advanced. jw.
	       * Oh, wrong. We still give even the experienced user a factor of ten.
	       */
	    }
	}
      cx = markdata->cx;
      cy = markdata->cy;
      switch (od)
	{
	case '\014':	/* CTRL-L Redisplay */
	  Redisplay(0);
	  GotoPos(cx, W2D(cy));
	  break;
	case '\010':	/* CTRL-H Backspace */
	case 'h':
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  revto(cx - rep_cnt, cy);
	  break;
	case '\016':	/* CTRL-N */
	case 'j':
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  revto(cx, cy + rep_cnt);
	  break;
	case '+':
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  j = cy + rep_cnt;
	  if (j > fore->w_histheight + d_height - 1)
	    j = fore->w_histheight + d_height - 1;
	  revto(linestart(j), j);
	  break;
	case '-':
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  cy -= rep_cnt;
	  if (cy < 0)
	    cy = 0;
	  revto(linestart(cy), cy);
	  break;
	case '^':
	  revto(linestart(cy), cy);
	  break;
	case '\n':
	  revto(markdata->left_mar, cy + 1);
	  break;
	case 'k':
	case '\020':	/* CTRL-P */
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  revto(cx, cy - rep_cnt);
	  break;
	case 'l':
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  revto(cx + rep_cnt, cy);
	  break;
	case '\001':	/* CTRL-A from tcsh/emacs */
	case '0':
	  revto(markdata->left_mar, cy);
	  break;
	case '\004':    /* CTRL-D down half screen */
	  if (rep_cnt == 0)
	    rep_cnt = (d_height + 1) >> 1;
	  revto_line(cx, cy + rep_cnt, W2D(cy));
	  break;
	case '$':
	  revto(lineend(cy), cy);
	  break;
        case '\022':	/* CTRL-R emacs style backwards search */
	  ISearch(-1);
	  in_mark = 0;
	  break;
        case '\023':	/* CTRL-S emacs style search */
	  ISearch(1);
	  in_mark = 0;
	  break;
	case '\025':	/* CTRL-U up half screen */
	  if (rep_cnt == 0)
	    rep_cnt = (d_height + 1) >> 1;
	  revto_line(cx, cy - rep_cnt, W2D(cy));
	  break;
	case '\007':	/* CTRL-G show cursorpos */
	  if (markdata->left_mar == 0 && markdata->right_mar == d_width - 1)
	    Msg(0, "Column %d Line %d(+%d)", cx+1, W2D(cy)+1,
		markdata->hist_offset);
	  else
	    Msg(0, "Column %d(%d..%d) Line %d(+%d)", cx+1,
		markdata->left_mar+1, markdata->right_mar+1, W2D(cy)+1, markdata->hist_offset);
	  break;
	case '\002':	/* CTRL-B  back one page */
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  rep_cnt *= d_height; 
	  revto(cx, cy - rep_cnt);
	  break;
	case '\006':	/* CTRL-F  forward one page */
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  rep_cnt *= d_height;
	  revto(cx, cy + rep_cnt);
	  break;
	case '\005':	/* CTRL-E  scroll up */
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  rep_cnt = MarkScrollUpDisplay(rep_cnt);
	  if (cy < D2W(0))
            revto(cx, D2W(0));
	  else
            GotoPos(cx, W2D(cy));
	  break;
	case '\031': /* CTRL-Y  scroll down */
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  rep_cnt = MarkScrollDownDisplay(rep_cnt);
	  if (cy > D2W(d_height-1))
            revto(cx, D2W(d_height-1));
	  else
            GotoPos(cx, W2D(cy));
	  break;
	case '@':
	  /* it may be usefull to have a key that does nothing */
	  break;
	case '%':
	  rep_cnt--;
	  /* rep_cnt is a percentage for the history buffer */
	  if (rep_cnt < 0)
	    rep_cnt = 0;
	  if (rep_cnt > 100)
	    rep_cnt = 100;
	  revto_line(markdata->left_mar, (rep_cnt * (fore->w_histheight + d_height)) / 100, (d_height - 1) / 2);
	  break;
	case 'g':
	  rep_cnt = 1;
	  /* FALLTHROUGH */
	case 'G':
	  /* rep_cnt is here the WIN line number */
	  if (rep_cnt == 0)
	    rep_cnt = fore->w_histheight + d_height;
	  revto_line(markdata->left_mar, --rep_cnt, (d_height - 1) / 2);
	  break;
	case 'H':
	  revto(markdata->left_mar, D2W(0));
	  break;
	case 'M':
	  revto(markdata->left_mar, D2W((d_height - 1) / 2));
	  break;
	case 'L':
	  revto(markdata->left_mar, D2W(d_height - 1));
	  break;
	case '|':
	  revto(--rep_cnt, cy);
	  break;
	case 'w':
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  nextword(&cx, &cy, NW_MUSTMOVE, rep_cnt);
	  revto(cx, cy);
	  break;
	case 'e':
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  nextword(&cx, &cy, NW_ENDOFWORD|NW_MUSTMOVE, rep_cnt);
	  revto(cx, cy);
	  break;
	case 'b':
	  if (rep_cnt == 0)
	    rep_cnt = 1;
	  nextword(&cx, &cy, NW_BACK|NW_ENDOFWORD|NW_MUSTMOVE, rep_cnt);
	  revto(cx, cy);
	  break;
	case 'a':
	  markdata->append_mode = 1 - markdata->append_mode;
	  debug1("append mode %d--\n", markdata->append_mode);
	  Msg(0, (markdata->append_mode) ? ":set append" : ":set noappend");
	  break;
	case 'v':
	case 'V':
	  /* this sets start column to column 9 for VI :set nu users */
	  if (markdata->left_mar == 8)
	    rep_cnt = 1;
	  else
	    rep_cnt = 9;
	  /* FALLTHROUGH */
	case 'c':
	case 'C':
	  /* set start column (c) and end column (C) */
	  if (markdata->second)
	    {
	      rem(markdata->x1, markdata->y1, cx, cy, 1, (char *)0, d_height-1); /* Hack */
	      markdata->second = 1;	/* rem turns off second */
	    }
	  rep_cnt--;
	  if (rep_cnt < 0)
	    rep_cnt = cx;
	  if (od != 'C')
	    {
	      markdata->left_mar = rep_cnt;
	      if (markdata->left_mar > markdata->right_mar)
		markdata->left_mar = markdata->right_mar;
	    }
	  else
	    {
	      markdata->right_mar = rep_cnt;
	      if (markdata->left_mar > markdata->right_mar)
		markdata->right_mar = markdata->left_mar;
	    }
	  if (markdata->second)
	    {
	      markdata->cx = markdata->x1; markdata->cy = markdata->y1;
	      revto(cx, cy);
	    }
	  if (od == 'v' || od == 'V')
	    Msg(0, (markdata->left_mar != 8) ? ":set nonu" : ":set nu");
	  break;
	case 'J':
	  /* how do you join lines in VI ? */
	  markdata->nonl = (markdata->nonl + 1) % 3;
	  switch (markdata->nonl)
	    {
	    case 0:
	      if (join_with_cr)
		Msg(0, "Multiple lines (CR/LF)");
	      else
		Msg(0, "Multiple lines (LF)");
	      break;
	    case 1:
	      Msg(0, "Lines joined");
	      break;
	    case 2:
	      Msg(0, "Lines joined with blanks");
	      break;
	    }
	  break;
	case '/':
	  Search(1);
	  in_mark = 0;
	  break;
	case '?':
	  Search(-1);
	  in_mark = 0;
	  break;
	case 'n':
	  Search(0);
	  break;
	case 'y':
	case 'Y':
	  if (markdata->second == 0)
	    {
	      revto(linestart(cy), cy);
	      markdata->second++;
	      cx = markdata->x1 = markdata->cx;
	      cy = markdata->y1 = markdata->cy;
	    }
	  if (--rep_cnt > 0)
	    revto(cx, cy + rep_cnt);
	  revto(lineend(markdata->cy), markdata->cy);
	  if (od == 'y')
	    break;
	  /* FALLTHROUGH */
	case 'W':
	  if (od == 'W')
	    {
	      if (rep_cnt == 0)
		rep_cnt = 1;
	      if (!markdata->second)
		{
		  nextword(&cx, &cy, NW_BACK|NW_ENDOFWORD, 1);
		  revto(cx, cy);
		  markdata->second++;
		  cx = markdata->x1 = markdata->cx;
		  cy = markdata->y1 = markdata->cy;
		}
	      nextword(&cx, &cy, NW_ENDOFWORD, rep_cnt);
	      revto(cx, cy);
	    }
	  cx = markdata->cx;
	  cy = markdata->cy;
	  /* FALLTHROUGH */
	case 'A':
	  if (od == 'A')
	    markdata->append_mode = 1;
	  /* FALLTHROUGH */
	case '>':
	  if (od == '>')
	    markdata->write_buffer = 1;
	  /* FALLTHROUGH */
	case ' ':
	case '\r':
	  if (!markdata->second)
	    {
	      markdata->second++;
	      markdata->x1 = cx;
	      markdata->y1 = cy;
	      revto(cx, cy);
#ifdef NETHACK
	      if (nethackflag)
		Msg(0, "You drop a magic marker - Column %d Line %d",
	    	    cx+1, W2D(cy)+1);
	      else
#endif
	      Msg(0, "First mark set - Column %d Line %d", cx+1, W2D(cy)+1);
	      break;
	    }
	  else
	    {
	      int append_mode = markdata->append_mode;
	      int write_buffer = markdata->write_buffer;

	      x2 = cx;
	      y2 = cy;
	      newcopylen = rem(markdata->x1, markdata->y1, x2, y2, 2, (char *)0, 0); /* count */
	      if (d_user->u_copybuffer != NULL && !append_mode)
		{
		  UserFreeCopyBuffer(d_user);
		}
	      if (newcopylen > 0)
		{
		  /* the +3 below is for : cr + lf + \0 */
		  if (d_user->u_copybuffer != NULL)
		    d_user->u_copybuffer = realloc(d_user->u_copybuffer,
			(unsigned) (d_user->u_copylen + newcopylen + 3));
		  else
		    {
		      d_user->u_copylen = 0;
		      d_user->u_copybuffer = malloc((unsigned) (newcopylen + 3));
		    }
		  if (d_user->u_copybuffer == NULL)
		    {
		      MarkAbort();
		      in_mark = 0;
		      Msg(0, "Not enough memory... Sorry.");
		      d_user->u_copylen = 0;
		      d_user->u_copybuffer = NULL;
		      break;
		    }
		  if (append_mode)
		    {
		      switch (markdata->nonl)
			{
			/* 
			 * this code defines, what glues lines together
			 */
			case 0:
			  if (join_with_cr)
			    {
			      d_user->u_copybuffer[d_user->u_copylen] = '\r';
			      d_user->u_copylen++;
			    }
			  d_user->u_copybuffer[d_user->u_copylen] = '\n';
			  d_user->u_copylen++;
			  break;
			case 1:
			  break;
			case 2:
			  d_user->u_copybuffer[d_user->u_copylen] = ' ';
			  d_user->u_copylen++;
			  break;
			}
		    }
		  yend = d_height - 1;
		  if (fore->w_histheight - markdata->hist_offset < d_height)
		    {
		      markdata->second = 0;
		      yend -= MarkScrollUpDisplay(fore->w_histheight - markdata->hist_offset);
		    }
		  d_user->u_copylen += rem(markdata->x1, markdata->y1, x2, y2, 
		    markdata->hist_offset == fore->w_histheight, 
		    d_user->u_copybuffer + d_user->u_copylen, yend);
		}
	      if (markdata->hist_offset != fore->w_histheight)
		LAY_CALL_UP(Activate(0));
	      ExitOverlayPage();
	      if (append_mode)
		Msg(0, "Appended %d characters to buffer",
		    newcopylen);
	      else
		Msg(0, "Copied %d characters into buffer", d_user->u_copylen);
	      if (write_buffer)
		WriteFile(DUMP_EXCHANGE);
	      in_mark = 0;
	      break;
	    }
	default:
	  MarkAbort();
#ifdef NETHACK
	  if (nethackflag)
	    Msg(0, "You escaped the dungeon.");
	  else
#endif
	  Msg(0, "Copy mode aborted");
	  in_mark = 0;
	  break;
	}
      if (in_mark)	/* markdata may be freed */
        markdata->rep_cnt = 0;
    }
  *inbufp = pt;
  *inlenp = inlen;
}

void revto(tx, ty)
int tx, ty;
{
  revto_line(tx, ty, -1);
}

/* tx, ty: WINDOW,  line: DISPLAY */
void revto_line(tx, ty, line)
int tx, ty, line;
{
  int fx, fy;
  int x, y, t, revst, reven, qq, ff, tt, st, en, ce = 0;
  int ystart = 0, yend = d_height-1;
  int i, ry;
 
  if (tx < 0)
    tx = 0;
  else if (tx > d_width - 1)
    tx = d_width -1;
  if (ty < 0)
    ty = 0;
  else if (ty > fore->w_histheight + d_height - 1)
    ty = fore->w_histheight + d_height - 1;
  
  fx = markdata->cx; fy = markdata->cy;
  markdata->cx = tx; markdata->cy = ty;
 
  /*
   * if we go to a position that is currently offscreen 
   * then scroll the screen
   */
  i = 0;
  if (line >= 0 && line < d_height)
    i = W2D(ty) - line;
  else if (ty < markdata->hist_offset)
    i = ty - markdata->hist_offset;
  else if (ty > markdata->hist_offset + (d_height-1))
    i = ty-markdata->hist_offset-(d_height-1);
  if (i > 0)
    yend -= MarkScrollUpDisplay(i);
  else if (i < 0)
    ystart += MarkScrollDownDisplay(-i);

  if (markdata->second == 0)
    {
      GotoPos(tx, W2D(ty));
      return;
    }
  
  qq = markdata->x1 + markdata->y1 * d_width;
  ff = fx + fy * d_width; /* "from" offset in WIN coords */
  tt = tx + ty * d_width; /* "to" offset  in WIN coords*/
 
  if (ff > tt)
    {
      st = tt; en = ff;
      x = tx; y = ty;
    }
  else
    {
      st = ff; en = tt;
      x = fx; y = fy;
    }
  if (st > qq)
    {
      st++;
      x++;
    }
  if (en < qq)
    en--;
  if (tt > qq)
    {
      revst = qq; reven = tt;
    }
  else
    {
      revst = tt; reven = qq;
    }
  ry = y - markdata->hist_offset;
  if (ry < ystart)
    {
      y += (ystart - ry);
      x = 0;
      st = y * d_width;
      ry = ystart;
    }
  for (t = st; t <= en; t++, x++)
    {
      if (x >= d_width)
	{
	  x = 0;
	  y++, ry++;
	}
      if (ry > yend)
	break;
      if (t == st || x == 0)
	{
	  for (ce = d_width; ce >= 0; ce--)
	    if (iWIN(y)[ce] != ' ')
	      break;
	}
      if (x <= ce && x >= markdata->left_mar && x <= markdata->right_mar
          && (CLP || x < d_width-1 || ry < d_bot))
	{
	  GotoPos(x, W2D(y));
	  if (t >= revst && t <= reven)
	    SetAttrFont(A_SO, ASCII);
	  else
	    SetAttrFont(aWIN(y)[x], fWIN(y)[x]);
	  PUTCHARLP(iWIN(y)[x]);
	}
    }
  GotoPos(tx, W2D(ty));
}

static void
MarkAbort()
{
  int yend, redisp;

  debug("MarkAbort\n");
  markdata = (struct markdata *)d_lay->l_data;
  fore = d_fore;
  yend = d_height - 1;
  redisp = markdata->second;
  if (fore->w_histheight - markdata->hist_offset < d_height)
    {
      markdata->second = 0;
      yend -= MarkScrollUpDisplay(fore->w_histheight - markdata->hist_offset);
    }
  if (markdata->hist_offset != fore->w_histheight)
    {
      LAY_CALL_UP(Activate(0));
    }
  else
    {
      rem(markdata->x1, markdata->y1, markdata->cx, markdata->cy, redisp, (char *)0, yend);
    }
  ExitOverlayPage();
}


static void
MarkRedisplayLine(y, xs, xe, isblank)
int y;	/* NOTE: y is in DISPLAY coords system! */
int xs, xe;
int isblank;
{
  int x, i, rm;
  int sta, sto, cp;	/* NOTE: these 3 are in WINDOW coords system */
  char *wi, *wa, *wf, *oldi;

  if (y < 0)	/* No special full page handling */
    return;

  markdata = (struct markdata *)d_lay->l_data;
  fore = d_fore;

  wi = iWIN(D2W(y));
  wa = aWIN(D2W(y));
  wf = fWIN(D2W(y));
  oldi = isblank ? blank : null;
 
  if (markdata->second == 0)
    {
      DisplayLine(oldi, null, null, wi, wa, wf, y, xs, xe);
      return;
    }
 
  sta = markdata->y1 * d_width + markdata->x1;
  sto = markdata->cy * d_width + markdata->cx;
  if (sta > sto)
    {
      i=sta; sta=sto; sto=i;
    }
  cp = D2W(y) * d_width + xs;
 
  rm = markdata->right_mar;
  for (x = d_width; x >= 0; x--)
    if (wi[x] != ' ')
      break;
  if (x < rm)
    rm = x;
 
  for (x = xs; x <= xe; x++, cp++)
    if (cp >= sta && x >= markdata->left_mar)
      break;
  if (x > xs)
    DisplayLine(oldi, null, null, wi, wa, wf, y, xs, x-1);
  for (; x <= xe; x++, cp++)
    {
      if (cp > sto || x > rm || (!CLP && x >= d_width-1 && y == d_bot))
	break;
      GotoPos(x, y);
      SetAttrFont(A_SO, ASCII);
      PUTCHARLP(wi[x]);
    }
  if (x <= xe)
    DisplayLine(oldi, null, null, wi, wa, wf, y, x, xe);
}


/*
 * This ugly routine is to speed up GotoPos()
 */
static int
MarkRewrite(ry, xs, xe, doit)
int ry, xs, xe, doit;
{
  int dx, x, y, st, en, t, rm;
  char *a, *f, *i;

  markdata = (struct markdata *)d_lay->l_data;
  fore = d_fore;
  y = D2W(ry);
  dx = xe - xs;
  if (doit)
    {
      i = iWIN(y) + xs;
      while (dx--)
        PUTCHARLP(*i++);
      return(0);
    }
  
  a = aWIN(y) + xs,
  f = fWIN(y) + xs;
  if (markdata->second == 0)
    st = en = -1;
  else
    {
      st = markdata->y1 * d_width + markdata->x1;
      en = markdata->cy * d_width + markdata->cx;
      if (st > en)
        {
          t = st; st = en; en = t;
        }
    }
  t = y * d_width + xs;
  for (rm=d_width, i=iWIN(y) + d_width; rm>=0; rm--)
    if (*i-- != ' ')
      break;
  if (rm > markdata->right_mar)
    rm = markdata->right_mar;
  x = xs;
  while (dx--)
    {
      if (t >= st && t <= en && x >= markdata->left_mar && x <= rm)
        {
	  if (d_attr != A_SO || d_font != ASCII)
	    return(EXPENSIVE);
        }
      else
        {
	  if (d_attr != *a || d_font != *f)
	    return(EXPENSIVE);
        }
      a++, f++, t++, x++;
    }
  return(xe - xs);
}


/*
 * scroll the screen contents up/down.
 */
static int MarkScrollUpDisplay(n)
int n;
{
  int i;

  debug1("MarkScrollUpDisplay(%d)\n", n);
  if (n <= 0)
    return 0;
  if (n > fore->w_histheight - markdata->hist_offset)
    n = fore->w_histheight - markdata->hist_offset;
  i = (n < d_height) ? n : (d_height);
  ScrollRegion(0, d_height - 1, i);
  markdata->hist_offset += n;
  while (i-- > 0)
    MarkRedisplayLine(d_height - i - 1, 0, d_width - 1, 1);
  return n;
}

static int MarkScrollDownDisplay(n)
int n;
{
  int i;

  debug1("MarkScrollDownDisplay(%d)\n", n);
  if (n <= 0)
    return 0;
  if (n > markdata->hist_offset)
    n = markdata->hist_offset;
  i = (n < d_height) ? n : (d_height);
  ScrollRegion(0, d_height - 1, -i);
  markdata->hist_offset -= n;
  while (i-- > 0)
    MarkRedisplayLine(i, 0, d_width - 1, 1);
  return n;
}

#endif /* COPY_PASTE */
