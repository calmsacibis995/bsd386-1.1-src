/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_c_hdlc.c,v 1.1 1993/02/22 17:16:26 karels Exp $
 */

/*
 * cisco HDLC link level encapsulation and cisco SLARP protocol
 * (Serial Line Address Resolution Protocol)
 */

/* #define SLARP_DEBUG */

#include "param.h"
#include "systm.h"
#include "kernel.h"
#include "malloc.h"
#include "mbuf.h"
#include "protosw.h"
#include "socket.h"
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

#include "if_p2p.h"
#include "if_c_hdlc.h"

#ifdef SLARP_DEBUG
int slarp_debug = 1;
#define dprintf(x)	if (slarp_debug) printf x
#else
#define dprintf(x)
#endif

struct	p2pcom *c_hdlc_ifs;	/* the list of active SLARP interfaces */
int	slarp_timer = 0;	/* SLARP keepalive timer active */


extern struct timeval time, boottime;

static void slarp_keepalive();

/*
 * Initialize the SLARP protocol on an interface
 */
int
c_hdlc_init(pp)
	struct p2pcom *pp;
{
	int s;

	pp->p2p_slarp.sls_myseq = time.tv_usec ^ time.tv_sec;   /* randomize */
	s = splimp();

	pp->p2p_prevp = &c_hdlc_ifs;
	if (pp->p2p_next = c_hdlc_ifs)
		c_hdlc_ifs->p2p_prevp = &pp->p2p_next;
	c_hdlc_ifs = pp;
	pp->p2p_slarp.sls_loopctr = 0;

	if (slarp_timer == 0) {
		timeout(slarp_keepalive, (caddr_t) 0, hz * 10);
		slarp_timer++;
	}

	pp->p2p_modem = 0;
	if (pp->p2p_mdmctl)
		(*pp->p2p_mdmctl)(pp, 1);

	pp->p2p_if.if_flags |= IFF_RUNNING;
	splx(s);
	dprintf(("%s%d: init c_hdlc link\n", pp->p2p_if.if_name,
	    pp->p2p_if.if_unit));
	return (0);
}

/*
 * Shutdown protocol on an interface
 */
void
c_hdlc_shutdown(pp)
	struct p2pcom *pp;
{
	int s;

	s = splimp();
	pp->p2p_if.if_flags &= ~IFF_RUNNING;
	if (pp->p2p_next)
		pp->p2p_next->p2p_prevp = pp->p2p_prevp;
	*(pp->p2p_prevp) = pp->p2p_next;
	if (pp->p2p_mdmctl)
		(*pp->p2p_mdmctl)(pp, 0);
	splx(s);
	dprintf(("%s%d: shutdown SLARP link\n", pp->p2p_if.if_name,
	    pp->p2p_if.if_unit));
}

/*
 * Send keepalive packets to all SLARP interfaces
 */
static void
slarp_keepalive()
{
	struct p2pcom *pp;
	struct mbuf *m;
	register caddr_t b;
	long t;
	int s;
	struct cisco_hdr *ch;
	struct cisco_slarp *sl;

	s = splimp();
	if (c_hdlc_ifs == 0) {
		slarp_timer = 0;
		splx(s);
		return;
	}
	t = (time.tv_sec - boottime.tv_sec) * 1000;
	for (pp = c_hdlc_ifs; pp != 0; pp = pp->p2p_next) {
#define	ifp	(&pp->p2p_if)
		if ((ifp->if_flags & IFF_UP) == 0)
			continue;
		pp->p2p_slarp.sls_myseq++;

		dprintf(("%s%d: send SLARP keepalive myseq=%d yourseq=%d\n",
			ifp->if_name, ifp->if_unit, pp->p2p_slarp.sls_myseq,
			ntohl(pp->p2p_slarp.sls_rmtseq)));

		/* Allocate mbuf */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			break;
		m->m_pkthdr.len = m->m_len = sizeof(struct cisco_hdr) + SLARP_SIZE;
		m->m_pkthdr.rcvif = 0;

		/* Fill in SLARP keepalive packet */
		ch = mtod(m, struct cisco_hdr *);
		ch->csh_addr = CISCO_ADDR_BCAST;
		ch->csh_ctl = 0;
		ch->csh_type = htons(CISCO_TYPE_SLARP);
		sl = (struct cisco_slarp *) (ch + 1);
		sl->csl_code = htonl(SLARP_KEEPALIVE);
		sl->csl_myseq = htonl(pp->p2p_slarp.sls_myseq);
		sl->csl_yourseq = pp->p2p_slarp.sls_rmtseq;
		sl->csl_rel = 0xffff;
		sl->csl_t1 = t >> 16;
		sl->csl_t0 = t;

		/* Push it to the interface and start output */
		if (IF_QFULL(&ifp->if_snd)) {
			IF_DROP(&ifp->if_snd);
			m_freem(m);
		} else {
			ifp->if_obytes += m->m_len;
			IF_ENQUEUE(&ifp->if_snd, m);
			if ((ifp->if_flags & IFF_OACTIVE) == 0)
				(*ifp->if_start)(ifp);
		}
	}

	timeout(slarp_keepalive, (caddr_t) 0, hz * 10);
	splx(s);
#undef ifp
}

/*
 * Output a packet to point-to-point interface with cisco HDLC enacapsulation
 */
int
c_hdlc_output(ifp, m, dst, rt)
	register struct ifnet *ifp;
	struct mbuf *m;
	struct sockaddr *dst;
	struct rtentry *rt;
{
	u_short type;
	int s;
	int len = m->m_pkthdr.len;
	struct cisco_hdr *ch;

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
		m_freem(m);
		return (ENETDOWN);
	}
	ifp->if_lastchange = time;

	switch (dst->sa_family) {
#ifdef INET
	case AF_INET:
		type = htons(CISCO_TYPE_INET);
		break;
#endif

	default:
		printf("%s%d: af%d not supported\n", ifp->if_name,
				ifp->if_unit, dst->sa_family);
		m_freem(m);
		return (EAFNOSUPPORT);
	}

	/*
	 * Add cisco serial line header. If there is no
	 * space in the first mbuf, allocate another.
	 */
	M_PREPEND(m, sizeof(struct cisco_hdr), M_DONTWAIT);
	if (m == 0)
		return (ENOBUFS);
	ch = mtod(m, struct cisco_hdr *);
	ch->csh_addr = CISCO_ADDR_UNICAST;
	ch->csh_ctl = 0;
	ch->csh_type = type;

	/*
	 * Queue packet on interface and
	 * start output if not active.
	 */
	s = splimp();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
		splx(s);
		m_freem(m);
		return (ENOBUFS);
	}
	IF_ENQUEUE(&ifp->if_snd, m);
	if ((ifp->if_flags & IFF_OACTIVE) == 0)
		(*ifp->if_start)(ifp);
	splx(s);
	ifp->if_obytes += sizeof(struct cisco_hdr) + len;
	return (0);
}

/*
 * Process a received cisco HDLC packet
 */
int
c_hdlc_input(pp, m)
	register struct p2pcom *pp;
	struct mbuf *m;
{
	struct cisco_hdr *ch;
	struct ifqueue *inq;
	int s;

	pp->p2p_if.if_lastchange = time;
	pp->p2p_if.if_ibytes += m->m_pkthdr.len;

	if (m->m_pkthdr.len <= sizeof(struct cisco_hdr) )
		goto dropit;
	ch = mtod(m, struct cisco_hdr *);
	if (ch->csh_addr != CISCO_ADDR_UNICAST &&
	    ch->csh_addr != CISCO_ADDR_BCAST)
		goto dropit;
	if (ch->csh_ctl != 0)
		goto dropit;

	m->m_pkthdr.len -= sizeof(struct cisco_hdr);
	m->m_len -= sizeof(struct cisco_hdr);
	m->m_data += sizeof(struct cisco_hdr);

	switch (ntohs(ch->csh_type)) {
	default:
		goto dropit;

#ifdef INET
	case CISCO_TYPE_INET:
		schednetisr(NETISR_IP);
		inq = &ipintrq;
		break;
#endif

	case CISCO_TYPE_SLARP:
		slarp_input(pp, m);
		return;
	}

	/*
	 * Queue the packet to incoming list
	 */
	s = splimp();
	if (IF_QFULL(inq)) {
		IF_DROP(inq);
		m_freem(m);
	} else
		IF_ENQUEUE(inq, m);
	splx(s);
	return (0);

dropit:
	m_freem(m);
	return (0);
}

slarp_input(pp, m)
	register struct p2pcom *pp;
	struct mbuf *m;
{
	struct cisco_hdr *ch;
	struct cisco_slarp *sl;
	struct ifaddr *ifa;
	int s;
	long t;
#define	ifp	(&pp->p2p_if)

	dprintf(("%s%d: got SLARP ctl", ifp->if_name, ifp->if_unit));
	sl = mtod(m, struct cisco_slarp *);
	if (m->m_pkthdr.len < SLARP_SIZE) {
		dprintf((" %d bytes - too short\n", m->m_pkthdr.len));
		goto dropit;
	}
	switch (ntohl(sl->csl_code)) {
	case SLARP_REQUEST:
		dprintf((" REQUEST"));
		for (ifa = ifp->if_addrlist; ifa &&
		    ifa->ifa_addr->sa_family != AF_INET; ifa = ifa->ifa_next)
			;
		if (ifa == 0) {
			dprintf((" inet addr unknown\n"));
			goto dropit;
		}
		dprintf((" send reply\n"));
		m_freem(m);

		/* Allocate mbuf */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			return (0);
		m->m_pkthdr.len = m->m_len = sizeof(struct cisco_hdr) + SLARP_SIZE;
		m->m_pkthdr.rcvif = 0;

		/* Fill in SLARP reply packet */
		ch = mtod(m, struct cisco_hdr *);
		ch->csh_addr = CISCO_ADDR_UNICAST;
		ch->csh_ctl = 0;
		ch->csh_type = htons(CISCO_TYPE_SLARP);
		sl = (struct cisco_slarp *) (ch + 1);
		sl->csl_code = htonl(SLARP_REPLY);
		sl->csl_addr =
			((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr.s_addr;
		sl->csl_mask =
			((struct sockaddr_in *)(ifa->ifa_netmask))->sin_addr.s_addr;
		sl->csl_rel = 0xffff;
		t = (time.tv_sec - boottime.tv_sec) * 1000;
		sl->csl_t1 = t >> 16;
		sl->csl_t0 = t;

		/* Push it to the interface and start output */
		s = splimp();
		if (IF_QFULL(&ifp->if_snd)) {
			IF_DROP(&ifp->if_snd);
			m_freem(m);
		} else {
			ifp->if_obytes += m->m_len;
			IF_ENQUEUE(&ifp->if_snd, m);
			if ((ifp->if_flags & IFF_OACTIVE) == 0)
				(*ifp->if_start)(ifp);
		}
		splx(s);
		return (0);

	case SLARP_REPLY:
		dprintf((" REPLY - ignore\n"));
		break;

	case SLARP_KEEPALIVE:
		pp->p2p_slarp.sls_rmtseq = sl->csl_myseq;
		dprintf((" KEEPALIVE myseq=%d yourseq=%d\n",
			ntohl(sl->csl_myseq), ntohl(sl->csl_yourseq)));
		if (ntohl(sl->csl_myseq) == pp->p2p_slarp.sls_myseq) {
			if (pp->p2p_slarp.sls_loopctr++ == 2)
				printf("%s%d: link looped?",
					ifp->if_name, ifp->if_unit);

			/* rerandomize seq number */
			pp->p2p_slarp.sls_myseq ^= time.tv_usec;
		} else
			pp->p2p_slarp.sls_loopctr = 0;
		break;

	default:
		dprintf((" unknown code %d\n", ntohl(sl->csl_code)));
		break;
	}
dropit:
	m_freem(m);
	return (0);
}
