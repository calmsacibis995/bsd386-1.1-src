/*	BSDI $Id: in_proto.c,v 1.4 1994/01/30 02:20:59 karels Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1993 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)in_proto.c	7.5 (Berkeley) 6/28/90
 */

#include "param.h"
#include "socket.h"
#include "protosw.h"
#include "domain.h"
#include "mbuf.h"

#include "in.h"
#include "in_systm.h"

/*
 * TCP/IP protocol family: IP, ICMP, UDP, TCP.
 */
int	ip_output(),ip_ctloutput();
int	ip_init(),ip_slowtimo(),ip_drain();
int	icmp_input();
#ifdef MULTICAST
int	igmp_init(),igmp_input(),igmp_fasttimo();
#endif
int	udp_input(),udp_ctlinput();
int	udp_usrreq();
int	udp_init();
int	tcp_input(),tcp_ctlinput();
int	tcp_usrreq(),tcp_ctloutput();
int	tcp_init(),tcp_fasttimo(),tcp_slowtimo(),tcp_drain();
int	rip_input(),rip_output(),rip_ctloutput(), rip_usrreq();
/*
 * IMP protocol family: raw interface.
 * Using the raw interface entry to get the timer routine
 * in is a kludge.
 */
#include "imp.h"
#if NIMP > 0
int	rimp_output(), hostslowtimo();
#endif

#ifdef NSIP
int	idpip_input(), nsip_ctlinput();
#endif

#ifdef TPIP
int	tpip_input(), tpip_ctlinput(), tp_ctloutput(), tp_usrreq();
int	tp_init(), tp_slowtimo(), tp_drain();
#endif

#ifdef EON
int	eoninput(), eonctlinput(), eonprotoinit();
#endif EON

extern	struct domain inetdomain;

struct protosw inetsw[] = {
{ 0,		&inetdomain,	0,		0,
  0,		ip_output,	0,		0,
  0,
  ip_init,	0,		ip_slowtimo,	ip_drain,
},
{ SOCK_DGRAM,	&inetdomain,	IPPROTO_UDP,	PR_ATOMIC|PR_ADDR,
  udp_input,	0,		udp_ctlinput,	ip_ctloutput,
  udp_usrreq,
  udp_init,	0,		0,		0,
},
{ SOCK_STREAM,	&inetdomain,	IPPROTO_TCP,	PR_CONNREQUIRED|PR_WANTRCVD,
  tcp_input,	0,		tcp_ctlinput,	tcp_ctloutput,
  tcp_usrreq,
  tcp_init,	tcp_fasttimo,	tcp_slowtimo,	tcp_drain,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_RAW,	PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		rip_ctloutput,
  rip_usrreq,
  0,		0,		0,		0,
},
{ SOCK_RAW,	&inetdomain,	IPPROTO_ICMP,	PR_ATOMIC|PR_ADDR,
  icmp_input,	rip_output,	0,		rip_ctloutput,
  rip_usrreq,
  0,		0,		0,		0,
},
#ifdef TPIP
{ SOCK_SEQPACKET,&inetdomain,	IPPROTO_TP,	PR_CONNREQUIRED|PR_WANTRCVD,
  tpip_input,	0,		tpip_ctlinput,		tp_ctloutput,
  tp_usrreq,
  tp_init,	0,		tp_slowtimo,	tp_drain,
},
#endif
/* EON (ISO CLNL over IP) */
#ifdef EON
{ SOCK_RAW,	&inetdomain,	IPPROTO_EON,	0,
  eoninput,	0,		eonctlinput,		0,
  0,
  eonprotoinit,	0,		0,		0,
},
#endif
#ifdef MULTICAST
{ SOCK_RAW,	&inetdomain,	IPPROTO_IGMP,	PR_ATOMIC|PR_ADDR,
  igmp_input,	rip_output,	0,		rip_ctloutput,
  rip_usrreq,
  igmp_init,	igmp_fasttimo,	0,		0,
},
#endif
#ifdef NSIP
{ SOCK_RAW,	&inetdomain,	IPPROTO_IDP,	PR_ATOMIC|PR_ADDR,
  idpip_input,	rip_output,	nsip_ctlinput,	0,
  rip_usrreq,
  0,		0,		0,		0,
},
#endif
	/* raw wildcard */
{ SOCK_RAW,	&inetdomain,	0,		PR_ATOMIC|PR_ADDR,
  rip_input,	rip_output,	0,		rip_ctloutput,
  rip_usrreq,
  0,		0,		0,		0,
},
};

struct domain inetdomain =
    { AF_INET, "internet", 0, 0, 0, 
      inetsw, &inetsw[sizeof(inetsw)/sizeof(inetsw[0])] };

#if NIMP > 0
extern	struct domain impdomain;

struct protosw impsw[] = {
{ SOCK_RAW,	&impdomain,	0,		PR_ATOMIC|PR_ADDR,
  0,		rimp_output,	0,		0,
  rip_usrreq,
  0,		0,		hostslowtimo,	0,
},
};

struct domain impdomain =
    { AF_IMPLINK, "imp", 0, 0, 0,
      impsw, &impsw[sizeof (impsw)/sizeof(impsw[0])] };
#endif

#include "hy.h"
#if NHY > 0
/*
 * HYPERchannel protocol family: raw interface.
 */
int	rhy_output();
extern	struct domain hydomain;

struct protosw hysw[] = {
{ SOCK_RAW,	&hydomain,	0,		PR_ATOMIC|PR_ADDR,
  0,		rhy_output,	0,		0,
  rip_usrreq,
  0,		0,		0,		0,
},
};

struct domain hydomain =
    { AF_HYLINK, "hy", 0, 0, 0, hysw, &hysw[sizeof (hysw)/sizeof(hysw[0])] };
#endif

/*
 * Internet configuration info
 */
#ifndef	IPFORWARDING
#ifdef GATEWAY
#define	IPFORWARDING	1	/* forward IP packets not for us */
#else /* GATEWAY */
#define	IPFORWARDING	0	/* don't forward IP packets not for us */
#endif /* GATEWAY */
#endif /* IPFORWARDING */

#ifndef IPFORWARDSRCRT
#ifdef GWSCREEN
#define	IPFORWARDSRCRT	0	/* don't forward source-routed IP packets */
#else
#define	IPFORWARDSRCRT	1	/* don't forward source-routed IP packets */
#endif /* GWSCREEN */
#endif /* IPFORWARDSRCRT */

#ifndef	IPSENDREDIRECTS
#define	IPSENDREDIRECTS	1
#endif

int	ipforwarding = IPFORWARDING;		/* act as router? */
int	ipforward_srcrt = IPFORWARDSRCRT;	/* forward src-routed? */
int	ipsendredirects = IPSENDREDIRECTS;

/*
 * If ip_input is compiled with GWSCREEN, it will call via ip_forward_fn
 * to forward packets.  The same binary will work for systems compiled
 * with or without GWSCREEN by initializing ip_forward_fn here
 * depending on the configuration when this (config-dependent) file
 * is compiled.
 */
#ifdef GWSCREEN
void ip_forwardscreen __P((struct mbuf *, int));
void (*ip_forward_fn) __P((struct mbuf *, int)) = ip_forwardscreen;
#else
void ip_forward __P((struct mbuf *, int));
void (*ip_forward_fn) __P((struct mbuf *, int)) = ip_forward;
#endif

#ifndef MROUTING
/* dummies when MROUTING is not defined */
int	ip_mrtproto;				/* for netstat only */
struct	socket *ip_mrouter;

ip_mrouter_cmd(cmd, so, m)
	int cmd;
        struct socket *so;
	struct mbuf *m;

{
	return (EINVAL);
}

ip_mrouter_done() { ; }

int
ip_mforward(ip, ifp, m)
	struct ip *ip;
	struct ifnet *ifp;
	struct mbuf *m;
{

	return (0);
}
#endif /* !MROUTING */


/* TCP parameters */
u_long	tcp_sendspace = 1024*4;
u_long	tcp_recvspace = 1024*4;

/* UDP parameters */
u_long	udp_sendspace = 9216;		/* really max datagram size */
u_long	udp_recvspace = 40 * (1024 + sizeof(struct sockaddr_in));
					/* 40 1K datagrams */
