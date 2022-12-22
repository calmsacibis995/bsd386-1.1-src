/************************************************************************
 *   IRC - Internet Relay Chat, common/dbuf.c
 *   Copyright (C) 1990 Markku Savela
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* -- Jto -- 20 Jun 1990
 * extern void free() fixed as suggested by
 * gruner@informatik.tu-muenchen.de
 */

/* -- Jto -- 10 May 1990
 * Changed memcpy into bcopy and removed the declaration of memset
 * because it was unnecessary.
 * Added the #includes for "struct.h" and "sys.h" to get bcopy/memcpy
 * work
 */

/*
** For documentation of the *global* functions implemented here,
** see the header file (dbuf.h).
**
*/

#ifndef lint
static  char sccsid[] = "@(#)dbuf.c	2.15 17 Oct 1993 (C) 1990 Markku Savela";
#endif

#include <stdio.h>
#include "struct.h"
#include "common.h"
#include "sys.h"

#if !defined(VALLOC) && !defined(valloc)
#define	valloc malloc
#endif

int	dbufalloc = 0, dbufblocks = 0;
static	dbufbuf	*freelist = NULL;

/* This is a dangerous define because a broken compiler will set DBUFSIZ
** to 4, which will work but will be very inefficient. However, there
** are other places where the code breaks badly if this is screwed
** up, so... -- Wumpus
*/

#define DBUFSIZ sizeof(((dbufbuf *)0)->data)

/*
** dbuf_alloc - allocates a dbufbuf structure either from freelist or
** creates a new one.
*/
static dbufbuf *dbuf_alloc()
{
#ifdef	VALLOC
	Reg1	dbufbuf	*dbptr, *db2ptr;
	Reg2	int	num;
#else
	Reg1	dbufbuf *dbptr;
#endif

	dbufalloc++;
	if (dbptr = freelist)
	    {
		freelist = freelist->next;
		return dbptr;
	    }
	if (dbufalloc * DBUFSIZ > BUFFERPOOL)
	    {
		dbufalloc--;
		return NULL;
	    }

#if defined(VALLOC) && !defined(DEBUGMODE)
# if defined(SOL20) || defined(_SC_PAGESIZE)
	num = sysconf(_SC_PAGESIZE)/sizeof(dbufbuf);
# else
	num = getpagesize()/sizeof(dbufbuf);
# endif
	if (num < 0)
		num = 1;

	dbufblocks += num;

	dbptr = (dbufbuf *)valloc(num*sizeof(dbufbuf));
	if (!dbptr)
		return (dbufbuf *)NULL;

	num--;
	for (db2ptr = dbptr; num; num--)
	    {
		db2ptr = (dbufbuf *)((char *)db2ptr + sizeof(dbufbuf));
		db2ptr->next = freelist;
		freelist = db2ptr;
	    }
	return dbptr;
#else
	dbufblocks++;
	return (dbufbuf *)MyMalloc(sizeof(dbufbuf));
#endif
}
/*
** dbuf_free - return a dbufbuf structure to the freelist
*/
static	void	dbuf_free(ptr)
Reg1	dbufbuf	*ptr;
{
	dbufalloc--;
	ptr->next = freelist;
	freelist = ptr;
}
/*
** This is called when malloc fails. Scrap the whole content
** of dynamic buffer and return -1. (malloc errors are FATAL,
** there is no reason to continue this buffer...). After this
** the "dbuf" has consistent EMPTY status... ;)
*/
static int dbuf_malloc_error(dyn)
dbuf *dyn;
    {
	dbufbuf *p;

	dyn->length = 0;
	dyn->offset = 0;
	while ((p = dyn->head) != NULL)
	    {
		dyn->head = p->next;
		MyFree((char *)p);
	    }
	return -1;
    }


int	dbuf_put(dyn, buf, length)
dbuf	*dyn;
char	*buf;
int	length;
{
	Reg1	dbufbuf	**h, *d;
	Reg2	int	nbr, off;
	Reg3	int	chunk;

	off = (dyn->offset + dyn->length) % DBUFSIZ;
	nbr = (dyn->offset + dyn->length) / DBUFSIZ;
	/*
	** Locate the last non-empty buffer. If the last buffer is
	** full, the loop will terminate with 'd==NULL'. This loop
	** assumes that the 'dyn->length' field is correctly
	** maintained, as it should--no other check really needed.
	*/
	for (h = &(dyn->head); (d = *h) && --nbr >= 0; h = &(d->next));
	/*
	** Append users data to buffer, allocating buffers as needed
	*/
	chunk = DBUFSIZ - off;
	dyn->length += length;
	for ( ;length > 0; h = &(d->next))
	    {
		if ((d = *h) == NULL)
		    {
			if ((d = (dbufbuf *)dbuf_alloc()) == NULL)
				return dbuf_malloc_error(dyn);
			*h = d;
			d->next = NULL;
		    }
		if (chunk > length)
			chunk = length;
		bcopy(buf, d->data + off, chunk);
		length -= chunk;
		buf += chunk;
		off = 0;
		chunk = DBUFSIZ;
	    }
	return 1;
    }


char	*dbuf_map(dyn,length)
dbuf	*dyn;
int	*length;
    {
	if (dyn->head == NULL)
	    {
		*length = 0;
		return NULL;
	    }
	*length = DBUFSIZ - dyn->offset;
	if (*length > dyn->length)
		*length = dyn->length;
	return (dyn->head->data + dyn->offset);
    }

int	dbuf_delete(dyn,length)
dbuf	*dyn;
int	length;
    {
	dbufbuf *d;
	int chunk;

	if (length > dyn->length)
		length = dyn->length;
	chunk = DBUFSIZ - dyn->offset;
	while (length > 0)
	    {
		if (chunk > length)
			chunk = length;
		length -= chunk;
		dyn->offset += chunk;
		dyn->length -= chunk;
		if (dyn->offset == DBUFSIZ || dyn->length == 0)
		    {
			d = dyn->head;
			dyn->head = d->next;
			dyn->offset = 0;
			dbuf_free(d);
		    }
		chunk = DBUFSIZ;
	    }
	if (dyn->head == (dbufbuf *)NULL)
		dyn->length = 0;
	return 0;
    }

int	dbuf_get(dyn, buf, length)
dbuf	*dyn;
char	*buf;
int	length;
    {
	int	moved = 0;
	int	chunk;
	char	*b;

	while (length > 0 && (b = dbuf_map(dyn, &chunk)) != NULL)
	    {
		if (chunk > length)
			chunk = length;
		bcopy(b, buf, (int)chunk);
		(void)dbuf_delete(dyn, chunk);
		buf += chunk;
		length -= chunk;
		moved += chunk;
	    }
	return moved;
    }

/*
int	dbuf_copy(dyn, buf, length)
dbuf	*dyn;
register char	*buf;
int	length;
{
	register dbufbuf	*d = dyn->head;
	register char	*s;
	register int	chunk, len = length, dlen = dyn->length;

	s = d->data + dyn->offset;
	chunk = MIN(DBUFSIZ - dyn->offset, dlen);

	while (len > 0)
	    {
		if (chunk > dlen)
			chunk = dlen;
		if (chunk > len)
			chunk = len;

		bcopy(s, buf, chunk);
		buf += chunk;
		len -= chunk;
		dlen -= chunk;

		if (dlen > 0 && (d = d->next))
		    {
			chunk = DBUFSIZ;
			s = d->data;
		    }
		else
			break;
	    }
	return length - len;
}
*/

/*
** dbuf_getmsg
**
** Check the buffers to see if there is a string which is terminted with
** either a \r or \n prsent.  If so, copy as much as possible (determined by
** length) into buf and return the amount copied - else return 0.
*/
int	dbuf_getmsg(dyn, buf, length)
dbuf	*dyn;
char	*buf;
register int	length;
{
	dbufbuf	*d;
	register char	*s;
	register int	dlen;
	register int	i;
	int	copy;

getmsg_init:
	d = dyn->head;
	dlen = dyn->length;
	i = DBUFSIZ - dyn->offset;
	if (i <= 0)
		return -1;
	copy = 0;
	if (d && dlen)
		s = dyn->offset + d->data;
	else
		return 0;

	if (i > dlen)
		i = dlen;
	while (length > 0 && dlen > 0)
	    {
		dlen--;
		if (*s == '\n' || *s == '\r')
		    {
			copy = dyn->length - dlen;
			/*
			** Shortcut this case here to save time elsewhere.
			** -avalon
			*/
			if (copy == 1)
			    {
				(void)dbuf_delete(dyn, 1);
				goto getmsg_init;
			    }
			break;
		    }
		length--;
		if (!--i)
		    {
			if (d = d->next)
			    {
				s = d->data;
				i = MIN(DBUFSIZ, dlen);
			    }
		    }
		else
			s++;
	    }

	if (copy <= 0)
		return 0;

	/*
	** copy as much of the message as wanted into parse buffer
	*/
	i = dbuf_get(dyn, buf, MIN(copy, length));
	/*
	** and delete the rest of it!
	*/
	if (copy - i > 0)
		(void)dbuf_delete(dyn, copy - i);
	if (i >= 0)
		*(buf+i) = '\0';	/* mark end of messsage */

	return i;
}
