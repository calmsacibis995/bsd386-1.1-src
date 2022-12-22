/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: ppp_frame.c,v 1.4 1993/04/13 22:31:00 karels Exp $
 */

/*
 * This code is partially derived from CMU PPP.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * PPP frame encapsulation protocol routines
 */
#include "param.h"
#include "systm.h"
#include "kernel.h"
#include "malloc.h"
#include "mbuf.h"
#include "protosw.h"
#include "socket.h"
#include "ioctl.h"
#include "errno.h"
#include "syslog.h"

#include "if.h"
#include "netisr.h"
#include "route.h"
#include "if_llc.h"
#include "if_dl.h"

#include "machine/mtpr.h"

#ifdef INET
#include "../netinet/in.h"
#include "../netinet/in_var.h"
#include "../netinet/in_systm.h"
#include "../netinet/ip.h"
#endif

#ifdef NS
#include "../netns/ns.h"
#include "../netns/ns_if.h"
#endif

#ifdef ISO
#include "../netiso/argo_debug.h"
#include "../netiso/iso.h"
#include "../netiso/iso_var.h"
#include "../netiso/iso_snpac.h"
#endif

#include "ppp_proto.h"
#include "if_p2p.h"

extern struct timeval time;

/*
 * The following disgusting hack gets around the problem that IP TOS
 * can't be set yet.  We want to put "interactive" traffic on a high
 * priority queue.  To decide if traffic is interactive, we check that
 * a) it is TCP and b) one of its ports is telnet, rlogin or ftp control.
 */
static u_short interactive_ports[8] = {
	0,      513,    0,      0,
	0,      21,     0,      23,
};
#define INTERACTIVE(p) (interactive_ports[(p) & 7] == (p))

/*
 * Output a paket to point-to-point interface with PPP encapsulation
 */
int
ppp_output(ifp, m, dst, rt)
	register struct ifnet *ifp;
	struct mbuf *m;
	struct sockaddr *dst;
	struct rtentry *rt;
{
	u_short type;
	int hl;
	int s;
	int len = m->m_pkthdr.len;
	struct p2pcom *pp = (struct p2pcom *) ifp;
	struct ppp_header phdr;
	struct ifqueue *outq;
	struct ip *ip;
	char *p;
	struct mbuf *m0;

	if (!(ifp->if_flags & IFF_UP)) {
		m_freem(m);
		return (ENETDOWN);
	}
	ifp->if_lastchange = time;

	outq = &ifp->if_snd;
	switch (dst->sa_family) {
#ifdef INET
	case AF_INET:
		if (pp->Ppp_ipcp.fsm_state != CP_OPENED) {
			if (pp->Ppp_ipcp.fsm_state == CP_CLOSED ||
			    pp->Ppp_ipcp.fsm_state == CP_CLOSING) {
				m_freem(m);
				return (EAFNOSUPPORT);
			}

			/* If there was a daemon queue to ipdefq */
			s = splimp();
			if (pp->Ppp_wdrop || pp->Ppp_ipdefq.ifq_len > 0) {
				m0 = m;
				m = 0;
				if (pp->Ppp_ipdefq.ifq_len > 10)
					IF_DEQUEUE(&pp->Ppp_ipdefq, m);
				IF_ENQUEUE(&pp->Ppp_ipdefq, m0);
			}

			/* Wake up the daemon */
			if (pp->Ppp_wdrop) {
				wakeup((caddr_t) &pp->Ppp_wdrop);
				pp->Ppp_wdrop = 0;
			}
			splx(s);
			if (m)
				m_freem(m);
			return (0);     /* simply drop on the floor */
		}
		type = PPP_IP;
		ip = mtod(m, struct ip *);

		/*
		 * Push "interactive" packets
		 * to the priority queue
		 */
		if (ip->ip_tos == IPTOS_LOWDELAY)
			outq = &pp->p2p_isnd;
		else if (ip->ip_p == IPPROTO_TCP && ip->ip_off == 0 &&
			 (pp->Ppp_dflt.ppp_flags & PPP_FTEL)) {

			/* get src & dst port numbers */
			hl = ntohl(((int *)ip)[ip->ip_hl]);
			if (INTERACTIVE(hl & 0xffff) || INTERACTIVE(hl >> 16))
				outq = &pp->p2p_isnd;
		}

		/*
		 * Handle TCP compression here
		 */
		if (ip->ip_p == IPPROTO_TCP &&
		    (pp->Ppp_txflags & PPP_TCPC)) {
			switch (sl_compress_tcp(m, ip, &pp->Ppp_ctcp)) {
			case TYPE_UNCOMPRESSED_TCP:
				type = PPP_VJNC;
				break;
			case TYPE_COMPRESSED_TCP:
				type = PPP_VJC;
				break;
			}
		}
		break;
#endif

	default:
		printf("%s%d: af%d not supported\n", ifp->if_name,
				ifp->if_unit, dst->sa_family);
		m_freem(m);
		return (EAFNOSUPPORT);
	}

	/* Reset the idle time counter */
	pp->Ppp_idlecnt = 1;

	/*
	 * Add ppp serial line header. If there is no
	 * space in the first mbuf, allocate another.
	 */
	phdr.phdr_addr = PPP_ADDRESS;
	phdr.phdr_ctl  = PPP_CONTROL;
	phdr.phdr_type = htons(type);
	p = (char *) &phdr;
	hl = sizeof(struct ppp_header);
	if (pp->Ppp_txflags & PPP_ACFC) {
		hl -= 2;
		p += 2;         /* skip address and control bytes */
	}
	if ((pp->Ppp_txflags & PPP_PFC) && (type & 0xff00) == 0) {
		hl--;
		p++;            /* skip first (zero) protocol selector byte */
	}
	M_PREPEND(m, hl, M_DONTWAIT);
	if (m == 0)
		return (ENOBUFS);
	bcopy(p, mtod(m, caddr_t), hl);

	/*
	 * Queue packet on interface and
	 * start output if not active.
	 */
	s = splimp();
	if (IF_QFULL2(&ifp->if_snd, &pp->p2p_isnd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
		m_freem(m);
		return (ENOBUFS);
	}
	IF_ENQUEUE(outq, m);
	if ((ifp->if_flags & IFF_OACTIVE) == 0)
		(*ifp->if_start)(ifp);
	splx(s);
	ifp->if_obytes += hl + len;
	return (0);
}

/*
 * Process a received PPP packet
 */
int
ppp_input(pp, m)
	struct p2pcom *pp;
	struct mbuf *m;
{
	struct ifqueue *inq;
	int s;
	u_short type;
	u_char *p;

	pp->p2p_if.if_lastchange = time;
	pp->p2p_if.if_ibytes += m->m_pkthdr.len;

	/* Skip address/control bytes */
	p = mtod(m, u_char *);
	if (p[0] == PPP_ADDRESS && p[1] == PPP_CONTROL) {
		m->m_pkthdr.len -= 2;
		m->m_len -= 2;
		m->m_data += 2;
		p += 2;
	}

	/* Retrieve the protocol type */
	if (*p & 01) {  /* Compressed protocol field */
		type = *p;
		m->m_pkthdr.len--;
		m->m_len--;
		m->m_data++;
	} else {
		type = ntohs(*(u_short *)p);
		m->m_pkthdr.len -= 2;
		m->m_len -= 2;
		m->m_data += 2;
	}

	if (m->m_pkthdr.len <= 0) {
		m_freem(m);
		return (0);
	}

	s = splimp();
	switch (type) {
	default:
		ppp_cp_in(pp, m, type);
		goto out;

#ifdef INET
	case PPP_IP:
	raw_ip:
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;

	case PPP_VJC:
		m = sl_muncompress_tcp(m, TYPE_COMPRESSED_TCP, &pp->Ppp_ctcp);
		if (m == 0)
			goto out;
		goto raw_ip;

	case PPP_VJNC:
		m = sl_muncompress_tcp(m, TYPE_UNCOMPRESSED_TCP, &pp->Ppp_ctcp);
		goto raw_ip;
#endif

	}

	/*
	 * Queue the packet to incoming list
	 */
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
	} else
		IF_ENQUEUE(inq, m);
out:
	splx(s);
	return (0);
}

/*
 * Polling the idle time counters
 */
void
ppp_idlepoll()
{
	extern struct p2pcom *ppp_ifs;
	extern int ppp_idletimer;
	register struct p2pcom *pp, *ppn;
	int s;

	s = splimp();
	if (ppp_ifs == 0) {
		ppp_idletimer = 0;
		splx(s);
		return;
	}
	for (pp = ppp_ifs; pp; pp = ppn) {
		ppn = pp->p2p_next;
		if (pp->Ppp_idletime == 0)
			continue;
		if ((pp->p2p_if.if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
			continue;
		if (pp->Ppp_idlecnt == 0)
			continue;
		if (pp->Ppp_idlecnt++ >= pp->Ppp_idletime) {
			dprintf(("IDLE TIMER EXPIRED\n"));
			ppp_cp_close(pp, FSM_LCP);
		}
	}
	timeout(ppp_idlepoll, 0, hz);
	splx(s);
}
