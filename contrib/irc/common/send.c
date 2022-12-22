/************************************************************************
 *   IRC - Internet Relay Chat, common/send.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *		      University of Oulu, Computing Center
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

/* -- Jto -- 16 Jun 1990
 * Added Armin's PRIVMSG patches...
 */

#ifndef lint
static  char sccsid[] = "@(#)send.c	2.28 17 Oct 1993 (C) 1988 University of Oulu, \
Computing Center and Jarkko Oikarinen";
#endif

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "h.h"
#include <stdio.h>

#ifdef	IRCII_KLUDGE
#define	NEWLINE	"\n"
#else
#define NEWLINE	"\r\n"
#endif

static	char	sendbuf[2048];
static	int	send_message PROTO((aClient *, char *, int));

#ifndef CLIENT_COMPILE
static	int	sentalong[MAXCONNECTIONS];
#endif

/*
** dead_link
**	An error has been detected. The link *must* be closed,
**	but *cannot* call ExitClient (m_bye) from here.
**	Instead, mark it with FLAGS_DEADSOCKET. This should
**	generate ExitClient from the main loop.
**
**	If 'notice' is not NULL, it is assumed to be a format
**	for a message to local opers. I can contain only one
**	'%s', which will be replaced by the sockhost field of
**	the failing link.
**
**	Also, the notice is skipped for "uninteresting" cases,
**	like Persons and yet unknown connections...
*/
static	int	dead_link(to, notice)
aClient *to;
char	*notice;
{
	to->flags |= FLAGS_DEADSOCKET;
#ifndef CLIENT_COMPILE
	if (notice != (char *)NULL && !IsPerson(to) && !IsUnknown(to) &&
	    !(to->flags & FLAGS_CLOSING))
		sendto_ops(notice, get_client_name(to, FALSE));
#endif
	DBufClear(&to->recvQ);
	DBufClear(&to->sendQ);
	return -1;
}

#ifndef CLIENT_COMPILE
/*
** flush_connections
**	Used to empty all output buffers for all connections. Should only
**	be called once per scan of connections. There should be a select in
**	here perhaps but that means either forcing a timeout or doing a poll.
**	When flushing, all we do is empty the obuffer array for each local
**	client and try to send it. if we cant send it, it goes into the sendQ
**	-avalon
*/
void	flush_connections(fd)
int	fd;
{
#ifdef SENDQ_ALWAYS
	Reg1	int	i;
	Reg2	aClient *cptr;

	if (fd == me.fd)
	    {
		for (i = 0; i <= highest_fd; i++)
			if ((cptr = local[i]) && DBufLength(&cptr->sendQ) > 0)
				(void)send_queued(cptr);
	    }
	else if (fd >= 0 && local[fd])
		(void)send_queued(local[fd]);
#endif
}
#endif

/*
** send_message
**	Internal utility which delivers one message buffer to the
**	socket. Takes care of the error handling and buffering, if
**	needed.
*/
static	int	send_message(to, msg, len)
aClient	*to;
char	*msg;	/* if msg is a null pointer, we are flushing connection */
int	len;
#ifdef SENDQ_ALWAYS
{
	if (IsDead(to))
		return 0; /* This socket has already been marked as dead */
# ifdef	CLIENT_COMPILE
	if (DBufLength(&to->sendQ) > MAXSENDQLENGTH)
		return dead_link(to,"Max SendQ limit exceeded for %s");
# else
	if (DBufLength(&to->sendQ) > get_sendq(to))
	    {
		if (IsServer(to))
			sendto_ops("Max SendQ limit exceeded for %s: %d > %d",
			   	get_client_name(to, FALSE),
				DBufLength(&to->sendQ), get_sendq(to));
		return dead_link(to, NULL);
	    }
# endif
	else if (dbuf_put(&to->sendQ, msg, len) < 0)
		return dead_link(to, "Buffer allocation error for %s");
	/*
	** Update statistics. The following is slightly incorrect
	** because it counts messages even if queued, but bytes
	** only really sent. Queued bytes get updated in SendQueued.
	*/
	to->sendM += 1;
	me.sendM += 1;
	if (to->acpt != &me)
		to->acpt->sendM += 1;
	/*
	** This little bit is to stop the sendQ from growing too large when
	** there is no need for it to. Thus we call send_queued() every time
	** 2k has been added to the queue since the last non-fatal write.
	** Also stops us from deliberately building a large sendQ and then
	** trying to flood that link with data (possible during the net
	** relinking done by servers with a large load).
	*/
	if (DBufLength(&to->sendQ)/1024 > to->lastsq)
		send_queued(to);
	return 0;
}
#else
{
	int	rlen = 0;

	if (IsDead(to))
		return 0; /* This socket has already been marked as dead */

	/*
	** DeliverIt can be called only if SendQ is empty...
	*/
	if ((DBufLength(&to->sendQ) == 0) &&
	    (rlen = deliver_it(to, msg, len)) < 0)
		return dead_link(to,"Write error to %s, closing link");
	else if (rlen < len)
	    {
		/*
		** Was unable to transfer all of the requested data. Queue
		** up the remainder for some later time...
		*/
# ifdef	CLIENT_COMPILE
		if (DBufLength(&to->sendQ) > MAXSENDQLENGTH)
			return dead_link(to,"Max SendQ limit exceeded for %s");
# else
		if (DBufLength(&to->sendQ) > get_sendq(to))
		    {
			sendto_ops("Max SendQ limit exceeded for %s : %d > %d",
				   get_client_name(to, FALSE),
				   DBufLength(&to->sendQ), get_sendq(to));
			return dead_link(to, NULL);
		    }
# endif
		else if (dbuf_put(&to->sendQ,msg+rlen,len-rlen) < 0)
			return dead_link(to,"Buffer allocation error for %s");
	    }
	/*
	** Update statistics. The following is slightly incorrect
	** because it counts messages even if queued, but bytes
	** only really sent. Queued bytes get updated in SendQueued.
	*/
	to->sendM += 1;
	me.sendM += 1;
	if (to->acpt != &me)
		to->acpt->sendM += 1;
	to->sendB += rlen;
	me.sendB += rlen;
	if (to->acpt != &me)
		to->acpt->sendB += 1;
	return 0;
}
#endif

/*
** send_queued
**	This function is called from the main select-loop (or whatever)
**	when there is a chance the some output would be possible. This
**	attempts to empty the send queue as far as possible...
*/
int	send_queued(to)
aClient *to;
{
	char	*msg;
	int	len, rlen;

	/*
	** Once socket is marked dead, we cannot start writing to it,
	** even if the error is removed...
	*/
	if (IsDead(to))
	    {
		/*
		** Actually, we should *NEVER* get here--something is
		** not working correct if send_queued is called for a
		** dead socket... --msa
		*/
#ifndef SENDQ_ALWAYS
		return dead_link(to, "send_queued called for a DEADSOCKET:%s");
#else
		return -1;
#endif
	    }
	while (DBufLength(&to->sendQ) > 0)
	    {
		msg = dbuf_map(&to->sendQ, &len);
					/* Returns always len > 0 */
		if ((rlen = deliver_it(to, msg, len)) < 0)
			return dead_link(to,"Write error to %s, closing link");
		(void)dbuf_delete(&to->sendQ, rlen);
		to->lastsq = DBufLength(&to->sendQ)/1024;
		to->sendB += rlen;
		me.sendB += rlen;
		if (to->acpt != &me)
			to->acpt->sendB += rlen;
		if (rlen < len) /* ..or should I continue until rlen==0? */
			break;
	    }

	return (IsDead(to)) ? -1 : 0;
}

/*
** send message to single client
*/
#ifndef	USE_VARARGS
/*VARARGS*/
void	sendto_one(to, pattern, p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)
aClient *to;
char	*pattern, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10, *p11;
{
#else
void	sendto_one(to, pattern, va_alist)
aClient	*to;
char	*pattern;
va_dcl
{
	va_list	vl;
#endif

/*
# ifdef NPATH
        check_command((long)1, pattern, p1, p2, p3);
# endif
*/

#ifdef VMS
	extern int goodbye;
	
	if (StrEq("QUIT", pattern)) 
		goodbye = 1;
#endif

#ifdef	USE_VARARGS
	va_start(vl);
	(void)vsprintf(sendbuf, pattern, vl);
	va_end(vl);
#else
	(void)sprintf(sendbuf, pattern, p1, p2, p3, p4, p5, p6,
		p7, p8, p9, p10, p11);
#endif
	Debug((DEBUG_SEND,"Sending [%s] to %s", sendbuf,to->name));

	if (to->from)
		to = to->from;
	if (to->fd < 0)
	    {
		Debug((DEBUG_ERROR,
		      "Local socket %s with negative fd... AARGH!",
		      to->name));
	    }
#ifndef	CLIENT_COMPILE
	else if (IsMe(to))
	    {
		sendto_ops("Trying to send [%s] to myself!", sendbuf);
		return;
	    }
#endif
	(void)strcat(sendbuf, NEWLINE);
#ifndef	IRCII_KLUDGE
	sendbuf[510] = '\r';
#endif
	sendbuf[511] = '\n';
	sendbuf[512] = '\0';
	(void)send_message(to, sendbuf, strlen(sendbuf));
}

#ifndef CLIENT_COMPILE
# ifndef	USE_VARARGS
/*VARARGS*/
void	sendto_channel_butone(one, from, chptr, pattern,
			      p1, p2, p3, p4, p5, p6, p7, p8)
aClient *one, *from;
aChannel *chptr;
char	*pattern, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
{
# else
void	sendto_channel_butone(one, from, chptr, pattern, va_alist)
aClient	*one, *from;
aChannel *chptr;
char	*pattern;
va_dcl
{
	va_list	vl;
# endif
	Reg1	Link	*lp;
	Reg2	aClient *acptr;
	Reg3	int	i;

# ifdef	USE_VARARGS
	va_start(vl);
# endif
	for (i = 0; i < MAXCONNECTIONS; i++)
		sentalong[i] = 0;
	for (lp = chptr->members; lp; lp = lp->next)
	    {
		acptr = lp->value.cptr;
		if (acptr->from == one)
			continue;	/* ...was the one I should skip */
		i = acptr->from->fd;
		if (MyConnect(acptr) && IsRegisteredUser(acptr))
		    {
# ifdef	USE_VARARGS
			sendto_prefix_one(acptr, from, pattern, vl);
# else
			sendto_prefix_one(acptr, from, pattern, p1, p2,
					  p3, p4, p5, p6, p7, p8);
# endif
			sentalong[i] = 1;
		    }
		else
		    {
		/* Now check whether a message has been sent to this
		 * remote link already */
			if (sentalong[i] == 0)
			    {
# ifdef	USE_VARARGS
	  			sendto_prefix_one(acptr, from, pattern, vl);
# else
	  			sendto_prefix_one(acptr, from, pattern,
						  p1, p2, p3, p4,
						  p5, p6, p7, p8);
# endif
				sentalong[i] = 1;
			    }
		    }
	    }
# ifdef	USE_VARARGS
	va_end(vl);
# endif
	return;
}

/*
 * sendto_server_butone
 *
 * Send a message to all connected servers except the client 'one'.
 */
# ifndef	USE_VARARGS
/*VARARGS*/
void	sendto_serv_butone(one, pattern, p1, p2, p3, p4, p5, p6, p7, p8)
aClient *one;
char	*pattern, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
{
# else
void	sendto_serv_butone(one, pattern, va_alist)
aClient	*one;
char	*pattern;
va_dcl
{
	va_list	vl;
# endif
	Reg1	int	i;
	Reg2	aClient *cptr;

# ifdef	USE_VARARGS
	va_start(vl);
# endif

# ifdef NPATH
        check_command((long)2, pattern, p1, p2, p3);
# endif

	for (i = 0; i <= highest_fd; i++)
	    {
		if (!(cptr = local[i]) || (one && cptr == one->from))
			continue;
		if (IsServer(cptr))
# ifdef	USE_VARARGS
			sendto_one(cptr, pattern, vl);
	    }
	va_end(vl);
# else
			sendto_one(cptr, pattern, p1, p2, p3, p4,
				   p5, p6, p7, p8);
	    }
# endif
	return;
}

/*
 * sendto_common_channels()
 *
 * Sends a message to all people (inclusing user) on local server who are
 * in same channel with user.
 */
# ifndef	USE_VARARGS
/*VARARGS*/
void	sendto_common_channels(user, pattern, p1, p2, p3, p4,
				p5, p6, p7, p8)
aClient *user;
char	*pattern, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
{
# else
void	sendto_common_channels(user, pattern, va_alist)
aClient	*user;
char	*pattern;
va_dcl
{
	va_list	vl;
# endif
	Reg1	int	i;
	Reg2	aClient *cptr;
	Reg3	Link	*lp;

# ifdef	USE_VARARGS
	va_start(vl);
# endif
	for (i = 0; i <= highest_fd; i++)
	    {
		if (!(cptr = local[i]) || IsServer(cptr) ||
		    user == cptr || !user->user)
			continue;
		for (lp = user->user->channel; lp; lp = lp->next)
			if (IsMember(cptr, lp->value.chptr))
			    {
# ifdef	USE_VARARGS
				sendto_prefix_one(cptr, user, pattern, vl);
# else
				sendto_prefix_one(cptr, user, pattern,
						  p1, p2, p3, p4,
						  p5, p6, p7, p8);
# endif
				break;
			    }
	    }
	if (MyConnect(user))
# ifdef	USE_VARARGS
		sendto_prefix_one(user, user, pattern, vl);
	va_end(vl);
# else
		sendto_prefix_one(user, user, pattern, p1, p2, p3, p4,
					p5, p6, p7, p8);
# endif
	return;
}
#endif /* CLIENT_COMPILE */

/*
 * sendto_channel_butserv
 *
 * Send a message to all members of a channel that are connected to this
 * server.
 */
#ifndef	USE_VARARGS
/*VARARGS*/
void	sendto_channel_butserv(chptr, from, pattern, p1, p2, p3,
			       p4, p5, p6, p7, p8)
aChannel *chptr;
aClient *from;
char	*pattern, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
{
#else
void	sendto_channel_butserv(chptr, from, pattern, va_alist)
aChannel *chptr;
aClient *from;
char	*pattern;
va_dcl
{
	va_list	vl;
#endif
	Reg1	Link	*lp;
	Reg2	aClient	*acptr;

#ifdef	USE_VARARGS
	for (va_start(vl), lp = chptr->members; lp; lp = lp->next)
		if (MyConnect(acptr = lp->value.cptr))
			sendto_prefix_one(acptr, from, pattern, vl);
	va_end(vl);
#else
	for (lp = chptr->members; lp; lp = lp->next)
		if (MyConnect(acptr = lp->value.cptr))
			sendto_prefix_one(acptr, from, pattern,
					  p1, p2, p3, p4,
					  p5, p6, p7, p8);
#endif

	return;
}

/*
** send a msg to all ppl on servers/hosts that match a specified mask
** (used for enhanced PRIVMSGs)
**
** addition -- Armin, 8jun90 (gruner@informatik.tu-muenchen.de)
*/

static	int	match_it(one, mask, what)
aClient *one;
char	*mask;
int	what;
{
	switch (what)
	{
	case MATCH_HOST:
		return (matches(mask, one->user->host)==0);
	case MATCH_SERVER:
	default:
		return (matches(mask, one->user->server)==0);
	}
}

#ifndef CLIENT_COMPILE
/*
 * sendto_match_servs
 *
 * send to all servers which match the mask at the end of a channel name
 * (if there is a mask present) or to all if no mask.
 */
#ifndef	USE_VARARGS
/*VARARGS*/
void	sendto_match_servs(chptr, from, format, p1,p2,p3,p4,p5,p6,p7,p8,p9)
aChannel *chptr;
aClient	*from;
char	*format, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9;
{
#else
void	sendto_match_servs(chptr, from, format, va_alist)
aChannel *chptr;
aClient	*from;
char	*format;
va_dcl
{
	va_list	vl;
#endif
	Reg1	int	i;
	Reg2	aClient	*cptr;
	char	*mask;

#ifdef	USE_VARARGS
	va_start(vl);
#endif

# ifdef NPATH
        check_command((long)3, format, p1, p2, p3);
# endif
	if (chptr)
	    {
		if (*chptr->chname == '&')
			return;
		if (mask = (char *)rindex(chptr->chname, ':'))
			mask++;
	    }
	else
		mask = (char *)NULL;

	for (i = 0; i <= highest_fd; i++)
	    {
		if (!(cptr = local[i]))
			continue;
		if ((cptr == from) || !IsServer(cptr))
			continue;
		if (!BadPtr(mask) && IsServer(cptr) &&
		    matches(mask, cptr->name))
			continue;
#ifdef	USE_VARARGS
		sendto_one(cptr, format, vl);
	    }
	va_end(vl);
#else
		sendto_one(cptr, format, p1, p2, p3, p4, p5, p6, p7, p8, p9);
	    }
#endif
}

/*
 * sendto_match_butone
 *
 * Send to all clients which match the mask in a way defined on 'what';
 * either by user hostname or user servername.
 */
#ifndef	USE_VARARGS
/*VARARGS*/
void	sendto_match_butone(one, from, mask, what, pattern,
			    p1, p2, p3, p4, p5, p6, p7, p8)
aClient *one, *from;
int	what;
char	*mask, *pattern, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
{
#else
void	sendto_match_butone(one, from, mask, what, pattern, va_alist)
aClient *one, *from;
int	what;
char	*mask, *pattern;
va_dcl
{
	va_list	vl;
#endif
	Reg1	int	i;
	Reg2	aClient *cptr, *acptr;
  
#ifdef	USE_VARARGS
	va_start(vl);
#endif
	for (i = 0; i <= highest_fd; i++)
	    {
		if (!(cptr = local[i]))
			continue;       /* that clients are not mine */
 		if (cptr == one)	/* must skip the origin !! */
			continue;
		if (IsServer(cptr))
		    {
			for (acptr = client; acptr; acptr = acptr->next)
				if (IsRegisteredUser(acptr)
				    && match_it(acptr, mask, what)
				    && acptr->from == cptr)
					break;
			/* a person on that server matches the mask, so we
			** send *one* msg to that server ...
			*/
			if (acptr == NULL)
				continue;
			/* ... but only if there *IS* a matching person */
		    }
		/* my client, does he match ? */
		else if (!(IsRegisteredUser(cptr) &&
			 match_it(cptr, mask, what)))
			continue;
#ifdef	USE_VARARGS
		sendto_prefix_one(cptr, from, pattern, vl);
	    }
	va_end(vl);
#else
		sendto_prefix_one(cptr, from, pattern,
				  p1, p2, p3, p4, p5, p6, p7, p8);
	    }
#endif
	return;
}

/*
 * sendto_all_butone.
 *
 * Send a message to all connections except 'one'. The basic wall type
 * message generator.
 */
#ifndef	USE_VARARGS
/*VARARGS*/
void	sendto_all_butone(one, from, pattern, p1, p2, p3, p4, p5, p6, p7, p8)
aClient *one, *from;
char	*pattern, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
{
#else
void	sendto_all_butone(one, from, pattern, va_alist)
aClient *one, *from;
char	*pattern;
va_dcl
{
	va_list	vl;
#endif
	Reg1	int	i;
	Reg2	aClient *cptr;

#ifdef	USE_VARARGS
	for (va_start(vl), i = 0; i <= highest_fd; i++)
		if ((cptr = local[i]) && !IsMe(cptr) && one != cptr)
			sendto_prefix_one(cptr, from, pattern, vl);
	va_end(vl);
#else
	for (i = 0; i <= highest_fd; i++)
		if ((cptr = local[i]) && !IsMe(cptr) && one != cptr)
			sendto_prefix_one(cptr, from, pattern,
					  p1, p2, p3, p4, p5, p6, p7, p8);
#endif

	return;
}

/*
 * sendto_ops
 *
 *	Send to *local* ops only.
 */
#ifndef	USE_VARARGS
/*VARARGS*/
void	sendto_ops(pattern, p1, p2, p3, p4, p5, p6, p7, p8)
char	*pattern, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
{
#else
void	sendto_ops(pattern, va_alist)
char	*pattern;
va_dcl
{
	va_list	vl;
#endif
	Reg1	aClient *cptr;
	Reg2	int	i;
	char	nbuf[1024];

#ifdef	USE_VARARGS
	va_start(vl);
#endif
	for (i = 0; i <= highest_fd; i++)
		if ((cptr = local[i]) && !IsServer(cptr) && !IsMe(cptr) &&
		    SendServNotice(cptr))
		    {
			(void)sprintf(nbuf, ":%s NOTICE %s :*** Notice -- ",
					me.name, cptr->name);
			(void)strncat(nbuf, pattern,
					sizeof(nbuf) - strlen(nbuf));
#ifdef	USE_VARARGS
			sendto_one(cptr, nbuf, va_alist);
#else
			sendto_one(cptr, nbuf, p1, p2, p3, p4, p5, p6, p7, p8);
#endif
		    }
#ifdef	USE_SERVICES
		else if (cptr && IsService(cptr) &&
			 (cptr->service->wanted & SERVICE_WANT_SERVNOTE))
		    {
			(void)sprintf(nbuf, "NOTICE %s :*** Notice -- ",
					cptr->name);
			(void)strncat(nbuf, pattern,
					sizeof(nbuf) - strlen(nbuf));
# ifdef	USE_VARARGS
			sendto_one(cptr, nbuf, vl);
		    }
	va_end(vl);
# else
			sendto_one(cptr, nbuf, p1, p2, p3, p4, p5, p6, p7, p8);
		    }
# endif
#endif
	return;
}

/*
** sendto_ops_butone
**	Send message to all operators.
** one - client not to send message to
** from- client which message is from *NEVER* NULL!!
*/
#ifndef	USE_VARARGS
/*VARARGS*/
void	sendto_ops_butone(one, from, pattern, p1, p2, p3, p4, p5, p6, p7, p8)
aClient *one, *from;
char	*pattern, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
{
#else
void	sendto_ops_butone(one, from, pattern, va_alist)
aClient *one, *from;
char	*pattern;
va_dcl
{
	va_list	vl;
#endif
	Reg1	int	i;
	Reg2	aClient *cptr;

#ifdef	USE_VARARGS
	va_start(vl);
#endif
	for (i=0; i <= highest_fd; i++)
		sentalong[i] = 0;
	for (cptr = client; cptr; cptr = cptr->next)
	    {
		if (!SendWallops(cptr))
			continue;
		if (MyClient(cptr) && !(IsServer(from) || IsMe(from)))
			continue;
		i = cptr->from->fd;	/* find connection oper is on */
		if (sentalong[i])	/* sent message along it already ? */
			continue;
		if (cptr->from == one)
			continue;	/* ...was the one I should skip */
		sentalong[i] = 1;
# ifdef	USE_VARARGS
      		sendto_prefix_one(cptr->from, from, pattern, vl);
	    }
	va_end(vl);
# else
      		sendto_prefix_one(cptr->from, from, pattern,
				  p1, p2, p3, p4, p5, p6, p7, p8);
	    }
# endif
	return;
}
#endif

/*
 * to - destination client
 * from - client which message is from
 *
 * NOTE: NEITHER OF THESE SHOULD *EVER* BE NULL!!
 * -avalon
 */
#ifndef	USE_VARARGS
/*VARARGS*/
void	sendto_prefix_one(to, from, pattern, p1, p2, p3, p4, p5, p6, p7, p8)
Reg1	aClient *to;
Reg2	aClient *from;
char	*pattern, *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
{
#else
void	sendto_prefix_one(to, from, pattern, va_alist)
Reg1	aClient *to;
Reg2	aClient *from;
char	*pattern;
va_dcl
{
	va_list	vl;
#endif
	static	char	sender[HOSTLEN+NICKLEN+USERLEN+5];
	Reg3	anUser	*user;
	char	*par;
	int	flag = 0;

#ifdef	USE_VARARGS
	va_start(vl);
	par = va_arg(vl, char *);
#else
	par = p1;
#endif
	if (to && from && MyClient(to) && IsPerson(from) &&
	    !mycmp(par, from->name))
	    {
		user = from->user;
		(void)strcpy(sender, from->name);
		if (user)
		    {
			if (*user->username)
			    {
				(void)strcat(sender, "!");
				(void)strcat(sender, user->username);
			    }
			if (*user->host && !MyConnect(from))
			    {
				(void)strcat(sender, "@");
				(void)strcat(sender, user->host);
				flag = 1;
			    }
		    }
		/*
		** flag is used instead of index(sender, '@') for speed and
		** also since username/nick may have had a '@' in them. -avalon
		*/
		if (!flag && MyConnect(from) && *user->host)
		    {
			(void)strcat(sender, "@");
			if (IsUnixSocket(from))
				(void)strcat(sender, user->host);
			else
				(void)strcat(sender, from->sockhost);
		    }
#ifdef	USE_VARARGS
		par = sender;
	    }
	sendto_one(to, pattern, par, vl);
	va_end(vl);
#else
		par = sender;
	    }
	sendto_one(to, pattern, par, p2, p3, p4, p5, p6, p7, p8);
#endif
}
