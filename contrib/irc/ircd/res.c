/*	BSDI	$Id: res.c,v 1.1.1.1 1994/01/13 21:15:38 polk Exp $	*/
/*
 * ircd/res.c (C)opyright 1992 Darren Reed. All rights reserved.
 * This file may not be distributed without the author's permission in any
 * shape or form. The author takes no responsibility for any damage or loss
 * of property which results from the use of this software.
 */
#include "struct.h"
#include "common.h"
#include "sys.h"
#include "res.h"
#include "numeric.h"
#include "h.h"

#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include "nameser.h"
#include "resolv.h"

#ifndef lint
static  char sccsid[] = "@(#)res.c	2.31 10 Oct 1993 (C) 1992 Darren Reed";
#endif

#undef	DEBUG	/* because there is a lot of debug code in here :-) */

extern	int	dn_expand PROTO((char *, char *, char *, char *, int));
#ifdef __bsdi__
extern	int	__dn_skipname PROTO((char *, char *));
#else
extern	int	dn_skipname PROTO((char *, char *));
#endif
extern	int	res_mkquery PROTO((int, char *, int, int, char *, int,
				   struct rrec *, char *, int));

extern	int	errno, h_errno;
extern	int	highest_fd;
extern	aClient	*local[];

static	char	hostbuf[HOSTLEN+1];
static	char	dot[] = ".";
static	int	incache = 0;
static	CacheTable	hashtable[ARES_CACSIZE];
static	aCache	*cachetop = NULL;
static	ResRQ	*last, *first;

static	void	rem_cache PROTO((aCache *));
static	void	rem_request PROTO((ResRQ *));
static	int	do_query_name PROTO((Link *, char *, ResRQ *));
static	int	do_query_number PROTO((Link *, struct in_addr *, ResRQ *));
static	void	resend_query PROTO((ResRQ *));
static	int	proc_answer PROTO((ResRQ *, HEADER *, char *, char *));
static	int	query_name PROTO((char *, int, int, ResRQ *));
static	aCache	*make_cache PROTO((ResRQ *));
static	aCache	*find_cache_name PROTO((ResRQ *, char *));
static	aCache	*find_cache_number PROTO((ResRQ *, char *));
static	int	add_request PROTO((ResRQ *));
static	ResRQ	*make_request PROTO((Link *));
static	int	send_res_msg PROTO((char *, int, int));
static	ResRQ	*find_id PROTO((int));
static	int	hash_number PROTO((unsigned char *));
static	void	update_list PROTO((ResRQ *, aCache *));
static	int	hash_name PROTO((char *));

static	struct cacheinfo {
	int	ca_adds;
	int	ca_dels;
	int	ca_expires;
	int	ca_lookups;
	int	ca_na_hits;
	int	ca_nu_hits;
	int	ca_updates;
} cainfo;

static	struct	resinfo {
	int	re_errors;
	int	re_nu_look;
	int	re_na_look;
	int	re_replies;
	int	re_requests;
	int	re_resends;
	int	re_sent;
	int	re_timeouts;
	int	re_shortttl;
	int	re_unkrep;
} reinfo;

int	init_resolver(op)
int	op;
{
	int	ret = 0;

	if (op & RES_INITLIST)
	    {
		bzero((char *)&reinfo, sizeof(reinfo));
		first = last = NULL;
	    }
	if (op & RES_CALLINIT)
	    {
		ret = res_init();
		if (!_res.nscount)
		    {
			_res.nscount = 1;
			_res.nsaddr_list[0].sin_addr.s_addr =
				inet_addr("127.0.0.1");
		    }
	    }

	if (op & RES_INITSOCK)
		ret = resfd = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef DEBUG
	if (op & RES_INITDEBG);
		_res.options |= RES_DEBUG;
#endif
	if (op & RES_INITCACH)
	    {
		bzero((char *)&cainfo, sizeof(cainfo));
		bzero((char *)hashtable, sizeof(hashtable));
	    }
	if (op == 0)
		ret = resfd;
	return ret;
}

static	int	add_request(new)
ResRQ *new;
{
	if (!new)
		return -1;
	if (!first)
		first = last = new;
	else
	    {
		last->next = new;
		last = new;
	    }
	new->next = NULL;
	reinfo.re_requests++;
	return 0;
}

/*
 * remove a request from the list. This must also free any memory that has
 * been allocated for temporary storage of DNS results.
 */
static	void	rem_request(old)
ResRQ	*old;
{
	Reg1	ResRQ	**rptr, *r2ptr = NULL;
	Reg2	int	i;
	Reg3	char	*s;

	if (!old)
		return;
	for (rptr = &first; *rptr; r2ptr = *rptr, rptr = &(*rptr)->next)
		if (*rptr == old)
		    {
			*rptr = old->next;
			if (last == old)
				last = r2ptr;
			break;
		    }
#ifdef	DEBUG
	Debug((DEBUG_INFO,"rem_request:Remove %#x at %#x %#x",
		old, *rptr, r2ptr));
#endif
	r2ptr = old;
	if (r2ptr->he.h_name)
		MyFree((char *)r2ptr->he.h_name);
	for (i = 0; i < MAXALIASES; i++)
		if ((s = r2ptr->he.h_aliases[i]))
			MyFree(s);
	if (r2ptr->name)
		MyFree(r2ptr->name);
	MyFree(r2ptr);

	return;
}

/*
 * Create a DNS request record for the server.
 */
static	ResRQ	*make_request(lp)
Link	*lp;
{
	Reg1	ResRQ	*nreq;

	nreq = (ResRQ *)MyMalloc(sizeof(ResRQ));
	bzero((char *)nreq, sizeof(ResRQ));
	nreq->next = NULL; /* where NULL is non-zero ;) */
	nreq->sentat = time(NULL);
	nreq->retries = 4;
	nreq->resend = 1;
	nreq->srch = -1;
	if (lp)
		bcopy((char *)lp, (char *)&nreq->cinfo, sizeof(Link));
	else
		bzero((char *)&nreq->cinfo, sizeof(Link));
	nreq->timeout = nreq->sentat + RES_TIMEOUT;
	nreq->he.h_addrtype = AF_INET;
	nreq->he.h_name = NULL;
	nreq->he.h_aliases[0] = NULL;
	(void)add_request(nreq);
	return nreq;
}

/*
 * Remove queries from the list which have been there too long without
 * being resolved.
 */
time_t	timeout_query_list(now)
time_t	now;
{
	Reg1	ResRQ	*rptr, *r2ptr;
	Reg2	time_t	next = 0;
	aClient	*cptr;

	Debug((DEBUG_DNS,"timeout_query_list at %s",myctime(now)));
	for (rptr = first; rptr; rptr = r2ptr)
	    {
		r2ptr = rptr->next;
		if (now >= rptr->timeout)
			if (--rptr->retries <= 0)
			    {
#ifdef DEBUG
				Debug((DEBUG_ERROR,"timeout %x now %d cptr %x",
					rptr, now, rptr->cinfo.value.cptr));
#endif
				reinfo.re_timeouts++;
				cptr = rptr->cinfo.value.cptr;
				switch (rptr->cinfo.flags)
				{
				case ASYNC_CLIENT :
					ClearDNS(cptr);
					if (!DoingAuth(cptr))
						SetAccess(cptr);
					break;
				case ASYNC_SERVER :
					sendto_ops("Host %s unknown",
						   rptr->name);
					ClearDNS(cptr);
					if (check_server(cptr, NULL,
							 NULL, NULL, 1))
						(void)exit_client(cptr, cptr,
							cptr, "No Permission");
					break;
				case ASYNC_CONNECT :
					sendto_ops("Host %s unknown",
						   rptr->name);
					break;
				}
				rem_request(rptr);
				continue;
			    }
			else
			    {
				rptr->sentat = now;
				rptr->timeout = now + RES_TIMEOUT;
				resend_query(rptr);
#ifdef DEBUG
				Debug((DEBUG_INFO,"r %x now %d retry %d c %x",
					rptr, now, rptr->retries,
					rptr->cinfo.value.cptr));
#endif
			    }
		if (!next || rptr->timeout < next)
			next = rptr->timeout;
	    }
	return (next > now) ? next : (now + AR_TTL);
}

/*
 * del_queries - called by the server to cleanup outstanding queries for
 * which there no longer exist clients or conf lines.
 */
void	del_queries(cp)
char	*cp;
{
	Reg1	ResRQ	*rptr, *r2ptr;

	for (rptr = first, r2ptr = NULL; rptr; rptr = r2ptr)
	    {
		r2ptr = rptr->next;
		if (cp == rptr->cinfo.value.cp)
			rem_request(rptr);
	    }
}

/*
 * sends msg to all nameservers found in the "_res" structure.
 * This should reflect /etc/resolv.conf. We will get responses
 * which arent needed but is easier than checking to see if nameserver
 * isnt present. Returns number of messages successfully sent to 
 * nameservers or -1 if no successful sends.
 */
static	int	send_res_msg(msg, len, rcount)
char	*msg;
int	len, rcount;
{
	Reg1	int	i;
	int	sent = 0, max;

	if (!msg)
		return -1;

	max = MIN(_res.nscount, rcount);
	if (_res.options & RES_PRIMARY)
		max = 1;
	if (!max)
		max = 1;

	for (i = 0; i < max; i++)
	    {
		_res.nsaddr_list[i].sin_family = AF_INET;
		if (sendto(resfd, msg, len, 0, (struct sockaddr *)&(_res.nsaddr_list[i]),
			   sizeof(struct sockaddr)) == len)
		    {
			reinfo.re_sent++;
			sent++;
		    }
#ifdef	DEBUGMODE
		else
			Debug((DEBUG_ERROR,"s_r_m:sendto: %d on %d",
				errno, resfd));
#endif
	    }

	return (sent) ? sent : -1;
}


/*
 * find a dns request id (id is determined by dn_mkquery)
 */
static	ResRQ	*find_id(id)
int	id;
{
	Reg1	ResRQ	*rptr;

	for (rptr = first; rptr; rptr = rptr->next)
		if (rptr->id == id)
			return rptr;
	return NULL;
}

struct	hostent	*gethost_byname(name, lp)
char	*name;
Link	*lp;
{
	Reg1	aCache	*cp;

	reinfo.re_na_look++;
	if ((cp = find_cache_name(NULL, name)))
		return (struct hostent *)&(cp->he);
	if (!lp)
		return NULL;
	(void)do_query_name(lp, name, NULL);
	return NULL;
}

struct	hostent	*gethost_byaddr(addr, lp)
char	*addr;
Link	*lp;
{
	aCache	*cp;

	reinfo.re_nu_look++;
	if ((cp = find_cache_number(NULL, addr)))
		return (struct hostent *)&(cp->he);
	if (!lp)
		return NULL;
	(void)do_query_number(lp, (struct in_addr *)addr, NULL);
	return NULL;
}

static	int	do_query_name(lp, name, rptr)
Link	*lp;
char	*name;
Reg1	ResRQ	*rptr;
{
	char	hname[HOSTLEN+1];
	int	len;

	(void)strncpy(hname, name, sizeof(hname)-1);
	len = strlen(hname);

	if (rptr && (hname[len-1] != '.'))
	    {
		(void)strncat(hname, dot, sizeof(hname)-len-1);
		/*
		 * NOTE: The logical relationship between DNSRCH and DEFNAMES
		 * is implies. ie no DEFNAMES, no DNSRCH.
		 */
		if (_res.options & (RES_DEFNAMES|RES_DNSRCH) ==
		    (RES_DEFNAMES|RES_DNSRCH))
		    {
			if (_res.dnsrch[rptr->srch])
				(void)strncat(hname, _res.dnsrch[rptr->srch],
					sizeof(hname) - ++len -1);
		    }
		else if (_res.options & RES_DEFNAMES)
			(void)strncat(hname, _res.defdname,
					sizeof(hname) - len -1);
	    }

	/*
	 * Store the name passed as the one to lookup and generate other host
	 * names to pass onto the nameserver(s) for lookups.
	 */
	if (!rptr)
	    {
		rptr = make_request(lp);
		rptr->type = T_A;
		rptr->name = (char *)MyMalloc(strlen(name)+1);
		(void)strcpy(rptr->name, name);
	    }
	return (query_name(hname, C_IN, T_A, rptr));
}

/*
 * Use this to do reverse IP# lookups.
 */
static	int	do_query_number(lp, numb, rptr)
Link	*lp;
struct	in_addr	*numb;
Reg1	ResRQ	*rptr;
{
	char	ipbuf[32];
	Reg2	u_char	*cp;

	cp = (u_char *)&numb->s_addr;
	(void)sprintf(ipbuf,"%u.%u.%u.%u.in-addr.arpa.",
		(u_int)(cp[3]), (u_int)(cp[2]),
		(u_int)(cp[1]), (u_int)(cp[0]));

	if (!rptr)
	    {
		rptr = make_request(lp);
		rptr->type = T_PTR;
		rptr->addr.s_addr = numb->s_addr;
		bcopy((char *)&numb->s_addr,
			(char *)&rptr->he.h_addr, sizeof(struct in_addr));
		rptr->he.h_length = sizeof(struct in_addr);
	    }
	return (query_name(ipbuf, C_IN, T_PTR, rptr));
}

/*
 * generate a query based on class, type and name.
 */
static	int	query_name(name, class, type, rptr)
char	*name;
int	class, type;
ResRQ	*rptr;
{
	struct	timeval	tv;
	char	buf[MAXPACKET];
	int	r,s,k = 0;
	HEADER	*hptr;

	bzero(buf, sizeof(buf));
	r = res_mkquery(QUERY, name, class, type, NULL, 0, NULL,
			buf, sizeof(buf));
	if (r <= 0)
	    {
		h_errno = NO_RECOVERY;
		return r;
	    }
	hptr = (HEADER *)buf;
	(void) gettimeofday(&tv, NULL);
	do {
		hptr->id = htons(ntohs(hptr->id) + k +
				 (u_short)(tv.tv_usec & 0xffff));
		k++;
	} while (find_id(ntohs(hptr->id)));
	rptr->id = ntohs(hptr->id);
	rptr->sends++;
	s = send_res_msg(buf, r, rptr->sends);
	if (s == -1)
	    {
		h_errno = TRY_AGAIN;
		return -1;
	    }
	else
		rptr->sent += s;
	return 0;
}

static	void	resend_query(rptr)
ResRQ	*rptr;
{
	if (rptr->resend == 0)
		return;
	reinfo.re_resends++;
	switch(rptr->type)
	{
	case T_PTR:
		(void)do_query_number(NULL, &rptr->addr, rptr);
		break;
	case T_A:
		(void)do_query_name(NULL, rptr->name, rptr);
		break;
	default:
		break;
	}
	return;
}

/*
 * process name server reply.
 */
static	int	proc_answer(rptr, hptr, buf, eob)
ResRQ	*rptr;
char	*buf, *eob;
HEADER	*hptr;
{
	Reg1	char	*cp, **alias;
	Reg2	struct	hent	*hp;
	int	class, type, dlen, len, ans = 0, n;
	struct	in_addr	dr, *adr;

	cp = buf + sizeof(HEADER);
	hp = (struct hent *)&(rptr->he);
	adr = &hp->h_addr;
	while (adr->s_addr)
		adr++;
	alias = hp->h_aliases;
	while (*alias)
		alias++;
	while (hptr->qdcount-- > 0)
#ifdef __bsdi__
		cp += __dn_skipname(cp, eob) + QFIXEDSZ;
#else
		cp += dn_skipname(cp, eob) + QFIXEDSZ;
#endif
	/*
	 * proccess each answer sent to us blech.
	 */
	while (hptr->ancount-- > 0 && cp < eob) {
		n = dn_expand(buf, eob, cp, hostbuf, sizeof(hostbuf));
		cp += n;
		if (n <= 0)
			return ans;

		ans++;
		type = (int)_getshort(cp);
		cp += sizeof(short);
		class = (int)_getshort(cp);
		cp += sizeof(short);
		rptr->ttl = _getlong(cp);
		cp += sizeof(rptr->ttl);
		dlen =  (int)_getshort(cp);
		cp += sizeof(short);
		rptr->type = type;

		/* name server never returns with trailing '.' */
		if (!index(hostbuf,'.') && (_res.options & RES_DEFNAMES))
		    {
			(void)strcat(hostbuf, dot);
			(void)strcat(hostbuf, _res.defdname);
		    }

		switch(type)
		{
		case T_A :
			hp->h_length = dlen;
			if (ans == 1)
				hp->h_addrtype =  (class == C_IN) ?
							AF_INET : AF_UNSPEC;
			bcopy(cp, (char *)&dr, dlen);
			adr->s_addr = dr.s_addr;
			Debug((DEBUG_INFO,"got ip # %s for %s",
				inetntoa((char *)adr), hostbuf));
			len = strlen(hostbuf);
			if (!hp->h_name)
			    {
				hp->h_name =(char *)MyMalloc(len+1);
				(void)strcpy(hp->h_name, hostbuf);
			    }
			adr++;
			cp += dlen;
 			break;
		case T_PTR :
			if((n = dn_expand(buf, eob, cp, hostbuf,
					  sizeof(hostbuf) )) < 0)
			    {
				cp += n;
				continue;
			    }
			cp += n;
			n = strlen(hostbuf) + 1;
			Debug((DEBUG_INFO,"got host %s",hostbuf));
			/*
			 * copy the returned hostname into the host name
			 * or alias field if there is a known hostname
			 * already.
			 */
			if (hp->h_name)
			    {
				*alias = (char *)MyMalloc(n);
				(void)strcpy(*alias++, hostbuf);
				*alias = NULL;
			    }
			else
			    {
				hp->h_name = (char *)MyMalloc(n);
				(void)strcpy(hp->h_name, hostbuf);
			    }
			break;
		case T_CNAME :
			cp += dlen;
			if (alias >= &(hp->h_aliases[MAXALIASES-1]))
				continue;
			n = strlen(hostbuf)+1;
			Debug((DEBUG_INFO,"got cname %s",hostbuf));
			*alias = (char *)MyMalloc(n);
			(void)strcpy(*alias++, hostbuf);
			*alias = NULL;
			break;
		default :
#ifdef DEBUG
			Debug((DEBUG_INFO,"proc_answer: type:%d for:%s",
			      type,hostbuf));
#endif
			break;
		}
	}
	return ans;
}

/*
 * read a dns reply from the nameserver and process it.
 */
struct	hostent	*get_res(lp)
char	*lp;
{
	static	char	buf[sizeof(HEADER) + MAXPACKET];
	Reg1	HEADER	*hptr;
	Reg2	ResRQ	*rptr = NULL;
	aCache	*cp;
	struct	sockaddr_in	sin;
	int	rc, a, len = sizeof(sin), max;

	(void)alarm((unsigned)4);
	rc = recvfrom(resfd, buf, sizeof(buf), 0, (struct sockaddr *)&sin,
		      &len);
	(void)alarm((unsigned)0);
	if (rc <= 0)
		goto getres_err;
	/*
	 * convert DNS reply reader from Network byte order to CPU byte order.
	 */
	hptr = (HEADER *)buf;
	hptr->id = ntohs(hptr->id);
	hptr->ancount = ntohs(hptr->ancount);
	hptr->qdcount = ntohs(hptr->qdcount);
	hptr->nscount = ntohs(hptr->nscount);
	hptr->arcount = ntohs(hptr->arcount);
#ifdef	DEBUG
	Debug((DEBUG_NOTICE, "get_res:id = %d rcode = %d ancount = %d",
		hptr->id, hptr->rcode, hptr->ancount));
#endif
	reinfo.re_replies++;
	/*
	 * response for an id which we have already received an answer for
	 * just ignore this response.
	 */
	rptr = find_id(hptr->id);
	if (!rptr)
		goto getres_err;
	/*
	 * check against possibly fake replies
	 */
	max = MIN(_res.nscount, rptr->sends);
	if (_res.options & RES_PRIMARY)
		max = 1;
	if (!max)
		max = 1;

	for (a = 0; a < max; a++)
		if (!_res.nsaddr_list[a].sin_addr.s_addr ||
		    !bcmp((char *)&sin.sin_addr,
			  (char *)&_res.nsaddr_list[a].sin_addr,
			  sizeof(struct in_addr)))
			break;
	if (a == max)
	    {
		reinfo.re_unkrep++;
		goto getres_err;
	    }

	if ((hptr->rcode != NOERROR) || (hptr->ancount == 0))
	    {
		switch (hptr->rcode)
		{
		case NXDOMAIN:
			h_errno = TRY_AGAIN;
			break;
		case SERVFAIL:
			h_errno = TRY_AGAIN;
			break;
		case NOERROR:
			h_errno = NO_DATA;
			break;
		case FORMERR:
		case NOTIMP:
		case REFUSED:
		default:
			h_errno = NO_RECOVERY;
			break;
		}
		reinfo.re_errors++;
		/*
		** If a bad error was returned, we stop here and dont send
		** send any more (no retries granted).
		*/
		if (h_errno != TRY_AGAIN)
		    {
			Debug((DEBUG_DNS, "Fatal DNS error %d for %d",
				h_errno, hptr->rcode));
			rptr->resend = 0;
			rptr->retries = 0;
		    }
		goto getres_err;
	    }
	a = proc_answer(rptr, hptr, buf, buf+rc);
#ifdef DEBUG
	Debug((DEBUG_INFO,"get_res:Proc answer = %d",a));
#endif
	if (rptr->type == T_PTR)
	    {
		struct	hostent	*hp2 = NULL;

		Debug((DEBUG_DNS, "relookup %s <-> %s",
			rptr->he.h_name, inetntoa((char *)&rptr->he.h_addr)));
		/*
		 * Lookup the 'authoritive' name that we were given for the
		 * ip#.  By using this call rather than regenerating the
		 * type we automatically gain the use of the cache with no
		 * extra kludges.
		 */
		if ((hp2 = gethost_byname(rptr->he.h_name, &rptr->cinfo)))
			if (lp)
				bcopy((char *)&rptr->cinfo, lp, sizeof(Link));
		/*
		 * If name wasn't found, a request has been queued and it will
		 * be the last one queued.  This is rather nasty way to keep
		 * a host alias with the query. -avalon
		 */
		if (!hp2 && rptr->he.h_aliases[0])
			for (a = 0; rptr->he.h_aliases[a]; a++)
			    {
				Debug((DEBUG_DNS, "Copied CNAME %s for %s",
					rptr->he.h_aliases[a],
					rptr->he.h_name));
				last->he.h_aliases[a] = rptr->he.h_aliases[a];
				rptr->he.h_aliases[a] = NULL;
			    }

		rem_request(rptr);
		return hp2;
	    }

	if (lp)
		bcopy((char *)&rptr->cinfo, lp, sizeof(Link));
	cp = make_cache(rptr);
#ifdef	DEBUG
	Debug((DEBUG_INFO,"get_res:cp=%#x rptr=%#x (made)",cp,rptr));
#endif

	if (a > 0)
		rem_request(rptr);
	else
		if (!rptr->sent)
			rem_request(rptr);
	return cp ? (struct hostent *)&cp->he : NULL;

getres_err:
	/*
	 * Reprocess an error if the nameserver didnt tell us to "TRY_AGAIN".
	 */
	if (rptr)
	    {
		if (h_errno != TRY_AGAIN)
		    {
			/*
			 * Try again using different hostnames (append a
			 * different domain to the query name).
			 */
			if (_res.options & (RES_DNSRCH|RES_DEFNAMES) ==
					    (RES_DNSRCH|RES_DEFNAMES))
			    {
				if (_res.dnsrch[++rptr->srch])
				    {
					rptr->retries = _res.retry;
					rptr->sends = 0;
					rptr->resend = 1;
					resend_query(rptr);
				    }
			    }
			/*
			 * If we havent tried with the default domain and its
			 * set, then give it a try next.
			 */
			else if (_res.options & RES_DEFNAMES &&
				 ++rptr->srch == 0)
			    {
				rptr->retries = _res.retry;
				rptr->sends = 0;
				rptr->resend = 1;
				resend_query(rptr);
			    }
			else
				resend_query(rptr);
		    }
		else if (lp)
			bcopy((char *)&rptr->cinfo, lp, sizeof(Link));
	    }
	return (struct hostent *)NULL;
}

static	int	hash_number(ip)
Reg1 unsigned char *ip;
{
	Reg1	u_int	hashv = 0;

	/* could use loop but slower */
	hashv += (int)*ip++;
	hashv += hashv + (int)*ip++;
	hashv += hashv + (int)*ip++;
	hashv += hashv + (int)*ip++;
	hashv %= ARES_CACSIZE;
	return (hashv);
}

static	int	hash_name(name)
register	char	*name;
{
	Reg1	u_int	hashv = 0;

	for (; *name && *name != '.'; name++)
		hashv += *name;
	hashv %= ARES_CACSIZE;
	return (hashv);
}

/*
** Add a new cache item to the queue and hash table.
*/
static	aCache	*add_to_cache(ocp)
Reg1	aCache	*ocp;
{
	Reg1	aCache	*cp = NULL;
	Reg2	int	hashv;

#ifdef DEBUG
	Debug((DEBUG_INFO,
	      "add_to_cache:ocp %#x he %#x name %#x addrl %#x 0 %#x",
		ocp, &ocp->he, ocp->he.h_name, ocp->he.h_addr_list,
		ocp->he.h_addr_list[0]));
#endif
	ocp->list_next = cachetop;
	cachetop = ocp;

	hashv = hash_name(ocp->he.h_name);
	ocp->hname_next = hashtable[hashv].name_list;
	hashtable[hashv].name_list = ocp;

	hashv = hash_number((u_char *)ocp->he.h_addr);
	ocp->hnum_next = hashtable[hashv].num_list;
	hashtable[hashv].num_list = ocp;

#ifdef	DEBUG
	Debug((DEBUG_INFO, "add_to_cache:added %s[%08x] cache %#x.",
		ocp->he.h_name, ocp->he.h_addr_list[0], ocp));
	Debug((DEBUG_INFO,
		"add_to_cache:h1 %d h2 %x lnext %#x namnext %#x numnext %#x",
		hash_name(ocp->he.h_name), hashv, ocp->list_next,
		ocp->hname_next, ocp->hnum_next));
#endif

	/*
	 * LRU deletion of excessive cache entries.
	 */
	if (++incache > MAXCACHED)
	    {
		for (cp = cachetop; cp->list_next; cp = cp->list_next)
			;
		rem_cache(cp);
	    }
	cainfo.ca_adds++;

	return ocp;
}

/*
** update_list does not alter the cache structure passed. It is assumed that
** it already contains the correct expire time, if it is a new entry. Old
** entries have the expirey time updated.
*/
static	void	update_list(rptr, cachep)
ResRQ	*rptr;
aCache	*cachep;
{
	Reg1	aCache	**cpp, *cp = cachep;
	Reg2	char	*s, *t, **base;
	Reg3	int	i, j;
	int	addrcount;

	/*
	** search for the new cache item in the cache list by hostname.
	** If found, move the entry to the top of the list and return.
	*/
	cainfo.ca_updates++;

	for (cpp = &cachetop; *cpp; cpp = &((*cpp)->list_next))
		if (cp == *cpp)
			break;
	if (!*cpp)
		return;
	*cpp = cp->list_next;
	cp->list_next = cachetop;
	cachetop = cp;
	if (!rptr)
		return;

#ifdef	DEBUG
	Debug((DEBUG_DEBUG,"u_l:cp %#x na %#x al %#x ad %#x",
		cp,cp->he.h_name,cp->he.h_aliases,cp->he.h_addr));
	Debug((DEBUG_DEBUG,"u_l:rptr %#x h_n %#x", rptr, rptr->he.h_name));
#endif
	/*
	 * Compare the cache entry against the new record.  Add any
	 * previously missing names for this entry.
	 */
	for (i = 0; cp->he.h_aliases[i]; i++)
		;
	addrcount = i;
	for (i = 0, s = rptr->he.h_name; s && i < MAXALIASES;
	     s = rptr->he.h_aliases[i++])
	    {
		for (j = 0, t = cp->he.h_name; t && j < MAXALIASES;
		     t = cp->he.h_aliases[j++])
			if (!mycmp(t, s))
				break;
		if (!t && j < MAXALIASES-1)
		    {
			base = cp->he.h_aliases;

			addrcount++;
			base = (char **)MyRealloc((char *)base,
					sizeof(char *) * (addrcount + 1));
			cp->he.h_aliases = base;
#ifdef	DEBUG
			Debug((DEBUG_DNS,"u_l:add name %s hal %x ac %d",
				s, cp->he.h_aliases, addrcount));
#endif
			base[addrcount-1] = s;
			base[addrcount] = NULL;
			rptr->he.h_aliases[i-1] = NULL;
		    }
	    }
	for (i = 0; cp->he.h_addr_list[i]; i++)
		;
	addrcount = i;

	/*
	 * Do the same again for IP#'s.
	 */
	for (s = (char *)&rptr->he.h_addr.s_addr;
	     ((struct in_addr *)s)->s_addr; s += sizeof(struct in_addr))
	    {
		for (i = 0; (t = cp->he.h_addr_list[i]); i++)
			if (!bcmp(s, t, sizeof(struct in_addr)))
				break;
		if (i >= MAXADDRS || addrcount >= MAXADDRS)
			break;
		/*
		 * Oh man this is bad...I *HATE* it. -avalon
		 *
		 * Whats it do ?  Reallocate two arrays, one of pointers
		 * to "char *" and the other of IP addresses.  Contents of
		 * the IP array *MUST* be preserved and the pointers into
		 * it recalculated.
		 */
		if (!t)
		    {
			base = cp->he.h_addr_list;
			addrcount++;
			t = (char *)MyRealloc(*base,
					addrcount * sizeof(struct in_addr));
			base = (char **)MyRealloc((char *)base,
					(addrcount + 1) * sizeof(char *));
			cp->he.h_addr_list = base;
#ifdef	DEBUG
			Debug((DEBUG_DNS,"u_l:add IP %x hal %x ac %d",
				ntohl(((struct in_addr *)s)->s_addr),
				cp->he.h_addr_list,
				addrcount));
#endif
			for (; addrcount; addrcount--)
			    {
				*base++ = t;
				t += sizeof(struct in_addr);
			    }
			*base = NULL;
			bcopy(s, *--base, sizeof(struct in_addr));
		    }
	    }
	return;
}

static	aCache	*find_cache_name(rptr, name)
ResRQ	*rptr;
char	*name;
{
	Reg1	aCache	*cp;
	Reg2	char	*s;
	Reg3	int	hashv, i;

	hashv = hash_name(name);

	cp = hashtable[hashv].name_list;
#ifdef	DEBUG
	Debug((DEBUG_DNS,"find_cache_name:find %s : hashv = %d",name,hashv));
#endif

	for (; cp; cp = cp->hname_next)
		for (i = 0, s = cp->he.h_name; s; s = cp->he.h_aliases[i++])
			if (mycmp(s, name) == 0)
			    {
				cainfo.ca_na_hits++;
				update_list(rptr, cp);
				return cp;
			    }

	for (cp = cachetop; cp; cp = cp->list_next)
	    {
		/*
		 * if no aliases or the hash value matches, we've already
		 * done this entry and all possiblilities concerning it.
		 */
		if (!*cp->he.h_aliases)
			continue;
		if (hashv == hash_name(cp->he.h_name))
			continue;
		for (i = 0, s = cp->he.h_aliases[i]; s && i < MAXALIASES; i++)
			if (!mycmp(name, s)) {
				cainfo.ca_na_hits++;
				update_list(rptr, cp);
				return cp;
			    }
	    }
	return NULL;
}

/*
 * find a cache entry by ip# and update its expire time
 */
static	aCache	*find_cache_number(rptr, numb)
ResRQ	*rptr;
char	*numb;
{
	Reg1	aCache	*cp;
	Reg2	int	hashv,i;
#ifdef	DEBUG
	struct	in_addr	*ip = (struct in_addr *)numb;
#endif

	hashv = hash_number((u_char *)numb);

	cp = hashtable[hashv].num_list;
#ifdef DEBUG
	Debug((DEBUG_DNS,"find_cache_number:find %s[%08x]: hashv = %d",
		inetntoa(numb), ntohl(ip->s_addr), hashv));
#endif

	for (; cp; cp = cp->hnum_next)
		for (i = 0; cp->he.h_addr_list[i]; i++)
			if (!bcmp(cp->he.h_addr_list[i], numb,
				  sizeof(struct in_addr)))
			    {
				cainfo.ca_nu_hits++;
				update_list(rptr, cp);
				return cp;
			    }

	for (cp = cachetop; cp; cp = cp->list_next)
	    {
		/*
		 * single address entry...would have been done by hashed
		 * search above...
		 */
		if (!cp->he.h_addr_list[1])
			continue;
		/*
		 * if the first IP# has the same hashnumber as the IP# we
		 * are looking for, its been done already.
		 */
		if (hashv == hash_number((u_char *)cp->he.h_addr_list[0]))
			continue;
		for (i = 1; cp->he.h_addr_list[i]; i++)
			if (!bcmp(cp->he.h_addr_list[i], numb,
				  sizeof(struct in_addr)))
			    {
				cainfo.ca_nu_hits++;
				update_list(rptr, cp);
				return cp;
			    }
	    }
	return NULL;
}

static	aCache	*make_cache(rptr)
ResRQ	*rptr;
{
	Reg1	aCache	*cp;
	Reg2	int	i, n;
	Reg3	struct	hostent	*hp;
	Reg3	char	*s, **t;

	/*
	** shouldn't happen but it just might...
	*/
	if (!rptr->he.h_name || !rptr->he.h_addr.s_addr)
		return NULL;
	/*
	** Make cache entry.  First check to see if the cache already exists
	** and if so, return a pointer to it.
	*/
	if ((cp = find_cache_number(rptr, (char *)&rptr->he.h_addr.s_addr)))
		return cp;
	for (i = 1; rptr->he.h_addr_list[i].s_addr; i++)
		if ((cp = find_cache_number(rptr,
				(char *)&(rptr->he.h_addr_list[i].s_addr))))
			return cp;

	/*
	** a matching entry wasnt found in the cache so go and make one up.
	*/ 
	cp = (aCache *)MyMalloc(sizeof(aCache));
	bzero((char *)cp, sizeof(aCache));
	hp = &cp->he;
	for (i = 0; i < MAXADDRS; i++)
		if (!rptr->he.h_addr_list[i].s_addr)
			break;

	/*
	** build two arrays, one for IP#'s, another of pointers to them.
	*/
	t = hp->h_addr_list = (char **)MyMalloc(sizeof(char *) * (i+1));
	bzero((char *)t, sizeof(char *) * (i+1));

	s = (char *)MyMalloc(sizeof(struct in_addr) * i);
	bzero(s, sizeof(struct in_addr) * i);

	for (n = 0; n < i; n++, s += sizeof(struct in_addr))
	    {
		*t++ = s;
		bcopy((char *)&(rptr->he.h_addr_list[n].s_addr), s,
		      sizeof(struct in_addr));
	    }
	*t = (char *)NULL;

	/*
	** an array of pointers to CNAMEs.
	*/
	for (i = 0; i < MAXALIASES; i++)
		if (!rptr->he.h_aliases[i])
			break;
	i++;
	t = hp->h_aliases = (char **)MyMalloc(sizeof(char *) * i);
	for (n = 0; n < i; n++, t++)
	    {
		*t = rptr->he.h_aliases[n];
		rptr->he.h_aliases[n] = NULL;
	    }

	hp->h_addrtype = rptr->he.h_addrtype;
	hp->h_length = rptr->he.h_length;
	hp->h_name = rptr->he.h_name;
	if (rptr->ttl < 600)
	    {
		reinfo.re_shortttl++;
		cp->ttl = 600;
	    }
	else
		cp->ttl = rptr->ttl;
	cp->expireat = time(NULL) + cp->ttl;
	rptr->he.h_name = NULL;
#ifdef DEBUG
	Debug((DEBUG_INFO,"make_cache:made cache %#x", cp));
#endif
	return add_to_cache(cp);
}

/*
 * rem_cache
 *     delete a cache entry from the cache structures and lists and return
 *     all memory used for the cache back to the memory pool.
 */
static	void	rem_cache(ocp)
aCache	*ocp;
{
	Reg1	aCache	**cp;
	Reg2	struct	hostent *hp = &ocp->he;
	Reg3	int	hashv;
	Reg4	aClient	*cptr;

#ifdef	DEBUG
	Debug((DEBUG_DNS, "rem_cache: ocp %#x hp %#x l_n %#x aliases %#x",
		ocp, hp, ocp->list_next, hp->h_aliases));
#endif
	/*
	** Cleanup any references to this structure by destroying the
	** pointer.
	*/
	for (hashv = 0; hashv < highest_fd; hashv++)
		if ((cptr = local[hashv]) && (cptr->hostp == hp))
			cptr->hostp = NULL;
	/*
	 * remove cache entry from linked list
	 */
	for (cp = &cachetop; *cp; cp = &((*cp)->list_next))
		if (*cp == ocp)
		    {
			*cp = ocp->list_next;
			break;
		    }
	/*
	 * remove cache entry from hashed name lists
	 */
	hashv = hash_name(hp->h_name);
#ifdef	DEBUG
	Debug((DEBUG_DEBUG,"rem_cache: h_name %s hashv %d next %#x first %#x",
		hp->h_name, hashv, ocp->hname_next,
		hashtable[hashv].name_list));
#endif
	for (cp = &hashtable[hashv].name_list; *cp; cp = &((*cp)->hname_next))
		if (*cp == ocp)
		    {
			*cp = ocp->hname_next;
			break;
		    }
	/*
	 * remove cache entry from hashed number list
	 */
	hashv = hash_number((u_char *)hp->h_addr);
#ifdef	DEBUG
	Debug((DEBUG_DEBUG,"rem_cache: h_addr %s hashv %d next %#x first %#x",
		inetntoa(hp->h_addr), hashv, ocp->hnum_next,
		hashtable[hashv].num_list));
#endif
	for (cp = &hashtable[hashv].num_list; *cp; cp = &((*cp)->hnum_next))
		if (*cp == ocp)
		    {
			*cp = ocp->hnum_next;
			break;
		    }

	/*
	 * free memory used to hold the various host names and the array
	 * of alias pointers.
	 */
	if (hp->h_name)
		MyFree(hp->h_name);
	if (hp->h_aliases)
	    {
		for (hashv = 0; hp->h_aliases[hashv]; hashv++)
			MyFree(hp->h_aliases[hashv]);
		MyFree((char *)hp->h_aliases);
	    }

	/*
	 * free memory used to hold ip numbers and the array of them.
	 */
	if (hp->h_addr_list)
	    {
		if (*hp->h_addr_list)
			MyFree((char *)*hp->h_addr_list);
		MyFree((char *)hp->h_addr_list);
	    }

	MyFree((char *)ocp);

	incache--;
	cainfo.ca_dels++;

	return;
}

/*
 * removes entries from the cache which are older than their expirey times.
 * returns the time at which the server should next poll the cache.
 */
time_t	expire_cache(now)
time_t	now;
{
	Reg1	aCache	*cp, *cp2;
	Reg2	time_t	next = 0;

	for (cp = cachetop; cp; cp = cp2)
	    {
		cp2 = cp->list_next;

		if (now >= cp->expireat)
		    {
			cainfo.ca_expires++;
			rem_cache(cp);
		    }
		else if (!next || next > cp->expireat)
			next = cp->expireat;
	    }
	return (next > now) ? next : (now + AR_TTL);
}

/*
 * remove all dns cache entries.
 */
void	flush_cache()
{
	Reg1	aCache	*cp;

	while ((cp = cachetop))
		rem_cache(cp);
}

int	m_dns(cptr, sptr, parc, parv)
aClient *cptr, *sptr;
int	parc;
char	*parv[];
{
	Reg1	aCache	*cp;
	Reg2	int	i;

	if (parv[1] && *parv[1] == 'l') {
		for(cp = cachetop; cp; cp = cp->list_next)
		    {
			sendto_one(sptr, "NOTICE %s :Ex %d ttl %d host %s(%s)",
				   parv[0], cp->expireat - time(NULL), cp->ttl,
				   cp->he.h_name, inetntoa(cp->he.h_addr));
			for (i = 0; cp->he.h_aliases[i]; i++)
				sendto_one(sptr,"NOTICE %s : %s = %s (CN)",
					   parv[0], cp->he.h_name,
					   cp->he.h_aliases[i]);
			for (i = 1; cp->he.h_addr_list[i]; i++)
				sendto_one(sptr,"NOTICE %s : %s = %s (IP)",
					   parv[0], cp->he.h_name,
					   inetntoa(cp->he.h_addr_list[i]));
		    }
		return 0;
	}
	sendto_one(sptr,"NOTICE %s :Ca %d Cd %d Ce %d Cl %d Ch %d:%d Cu %d",
		   sptr->name,
		   cainfo.ca_adds, cainfo.ca_dels, cainfo.ca_expires,
		   cainfo.ca_lookups,
		   cainfo.ca_na_hits, cainfo.ca_nu_hits, cainfo.ca_updates);

	sendto_one(sptr,"NOTICE %s :Re %d Rl %d/%d Rp %d Rq %d",
		   sptr->name, reinfo.re_errors, reinfo.re_nu_look,
		   reinfo.re_na_look, reinfo.re_replies, reinfo.re_requests);
	sendto_one(sptr,"NOTICE %s :Ru %d Rsh %d Rs %d(%d) Rt %d", sptr->name,
		   reinfo.re_unkrep, reinfo.re_shortttl, reinfo.re_sent,
		   reinfo.re_resends, reinfo.re_timeouts);
	return 0;
}

u_long	cres_mem(sptr)
aClient	*sptr;
{
	register aCache	*c = cachetop;
	register struct	hostent	*h;
	register int	i;
	u_long	nm = 0, im = 0, sm = 0, ts = 0;

	for ( ;c ; c = c->list_next)
	    {
		sm += sizeof(*c);
		h = &c->he;
		for (i = 0; h->h_addr_list[i]; i++)
		    {
			im += sizeof(char *);
			im += sizeof(struct in_addr);
		    }
		im += sizeof(char *);
		for (i = 0; h->h_aliases[i]; i++)
		    {
			nm += sizeof(char *);
			nm += strlen(h->h_aliases[i]);
		    }
		nm += i - 1;
		nm += sizeof(char *);
		if (h->h_name)
			nm += strlen(h->h_name);
	    }
	ts = ARES_CACSIZE * sizeof(CacheTable);
	sendto_one(sptr, ":%s %d %s :RES table %d",
		   me.name, RPL_STATSDEBUG, sptr->name, ts);
	sendto_one(sptr, ":%s %d %s :Structs %d IP storage %d Name storage %d",
		   me.name, RPL_STATSDEBUG, sptr->name, sm, im, nm);
	return ts + sm + im + nm;
}
