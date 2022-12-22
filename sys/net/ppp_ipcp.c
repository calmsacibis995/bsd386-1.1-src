/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: ppp_ipcp.c,v 1.3 1993/04/13 22:33:33 karels Exp $
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
 * PPP IP Control Protocol
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
#include "socketvar.h"

#include "machine/mtpr.h"

#if INET
#include "netinet/in.h"
#include "../netinet/in_var.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"		/* XXX for slcompress */
#endif

#include "ppp_proto.h"
#include "if_p2p.h"

#ifdef INET

static u_long ppp_ipcp_getaddr __P((struct p2pcom *, int));
static void   ppp_ipcp_setaddr __P((struct p2pcom *, u_long, int));

/*
 * Timeout
 */
void
ppp_ipcp_timeout(pp)
	struct p2pcom *pp;
{
	ppp_cp_timeout(pp, FSM_IPCP);
}

/*
 * This level finished
 */
void
ppp_ipcp_tlf(pp)
	struct p2pcom *pp;
{

	/* If there is no more active NCPs drop the connection */
	if (--(pp->Ppp_ncps) == 0) {
		pp->p2p_if.if_flags &= ~IFF_RUNNING;
		ppp_cp_close(pp, FSM_LCP);
	}
}

/*
 * IPCP reached CP_OPENED -- process the deferred queue
 */
void
ppp_ipcp_tlu(pp)
	struct p2pcom *pp;
{
	register struct mbuf *m;
	static struct sockaddr fakedst = { 4, AF_INET };

	for (;;) {
		IF_DEQUEUE(&pp->Ppp_ipdefq, m);
		if (m == 0)
			return;
		ppp_output(&pp->p2p_if, m, &fakedst, 0);
	}
	/* NOTREACHED */
}

/*
 * Compose an IPCP Config-Request packet
 */
struct mbuf *
ppp_ipcp_creq(pp)
	struct p2pcom *pp;
{
	register struct mbuf *m;
	struct ppp_option opt;
	u_long a;

	dprintf(("IPCP_CREQ --"));
	m = 0;

	/* IP-Compression-Protocol */
	if (!(pp->Ppp_ipcp.fsm_rej & (1<<IPCP_CPROT)) &&
	    (pp->Ppp_flags & PPP_TCPC)) {
		dprintf((" {CPROT: %x}", PPP_VJC));
		opt.po_type = IPCP_CPROT;
		opt.po_len = 6;
		opt.po_data.s = htons(PPP_VJC);  /* Van Jacobson's compression */
		opt.po_data.c[2] = MAX_STATES - 1;
		opt.po_data.c[3] = 0;
		m = ppp_addopt(m, &opt);
	}

	/* IP-Address */
	if (!(pp->Ppp_ipcp.fsm_rej & (1<<IPCP_ADDR))) {
		a = ppp_ipcp_getaddr(pp, 0);
		dprintf((" {ADDR: %d.%d.%d.%d}", ntohl(a)>>24,
			(ntohl(a)>>16) & 0xff, (ntohl(a)>>8) & 0xff, ntohl(a) & 0xff));
		opt.po_type = IPCP_ADDR;
		opt.po_len = 6;
		opt.po_data.l = a;
		m = ppp_addopt(m, &opt);
	}

	dprintf(("\n"));
	m = ppp_cp_prepend(m, PPP_IPCP, CP_CONF_REQ, (++pp->Ppp_ipcp.fsm_id), 0);
	return (m);
}

/*
 * Process the rejected option
 */
void
ppp_ipcp_optrej(pp, opt)
	struct p2pcom *pp;
	struct ppp_option *opt;
{

	switch (opt->po_type) {
	case IPCP_CPROT:
		dprintf((" {CPROT}"));
		/* indicate that peer can't send compressed */
		pp->Ppp_flags &= ~PPP_TCPC;
		break;

	default:
		dprintf((" {%d: len=%d}", opt->po_type, opt->po_len));
	}
}

/*
 * Process the NAK-ed option
 */
void
ppp_ipcp_optnak(pp, opt)
	struct p2pcom *pp;
	struct ppp_option *opt;
{
	u_long a;

	switch (opt->po_type) {
	case IPCP_CPROT:
		dprintf((" {MRU: len=%d proto=0x%x}",
			opt->po_len, ntohs(opt->po_data.s)));
		if (ntohs(opt->po_data.s) != PPP_VJC)
			/* indicate that peer can't send compressed */
			pp->Ppp_flags &= ~PPP_TCPC;
		break;

	case IPCP_ADDR:
		dprintf((" {ADDR: len=%d addr=%d.%d.%d.%d}",
			opt->po_len, opt->po_data.c[0],
			opt->po_data.c[1], opt->po_data.c[2],
			opt->po_data.c[3]));
		a = ppp_ipcp_getaddr(pp, 0);
		if (a != 0) {
			if (a != opt->po_data.l) {
				printf("ppp: cannot negotiate local IP address, peer requests %d.%d.%d.%d\n",
					opt->po_data.c[0], opt->po_data.c[1],
					opt->po_data.c[2], opt->po_data.c[3]);
				ppp_cp_close(pp, FSM_IPCP);
			}
		} else {
			dprintf((" *SET SRC ADDR*"));
			ppp_ipcp_setaddr(pp, opt->po_data.l, 0);
		}
		break;

	default:
		dprintf((" {%d: len=%d}", opt->po_type, opt->po_len));
	}
}

/*
 * Process the received option
 */
int
ppp_ipcp_opt(pp, opt)
	struct p2pcom *pp;
	struct ppp_option *opt;
{
	u_long a, b;
	int res;

	switch (opt->po_type) {
	case IPCP_ADDRS:        /* IP-Addresses */
		dprintf((" {ADDRS: len=%d}", opt->po_len));
		if (opt->po_len != 2 + 2 * sizeof(long)) {
			dprintf((" {ADDRS: len=%d (should be %d)}", opt->po_len,2 + 2 * sizeof(long)));
			return (OPT_REJ);
		}
		dprintf((" {ADDRS: len=%d, dst=%d.%d.%d.%d src=%d.%d.%d.%d}", opt->po_len,
		    opt->po_data.c[0], opt->po_data.c[1], opt->po_data.c[2], opt->po_data.c[3],
		    opt->po_data.c[4], opt->po_data.c[5], opt->po_data.c[6], opt->po_data.c[7]
		));

		res = OPT_OK;

		/* Handle the destination's address */

		a = ppp_ipcp_getaddr(pp, 1);
		if (a == 0) {   /* Our dst addr undefined -- accept peer's */
			dprintf((" *SET DST ADDR <to %d.%d.%d.%d>*",
			opt->po_data.c[0], opt->po_data.c[1], opt->po_data.c[2], opt->po_data.c[3]
			));
			ppp_ipcp_setaddr(pp, opt->po_data.l, 1);
		} else if (opt->po_data.l != a) {
			dprintf((" *CHG DST ADDR <from %d.%d.%d.%d",
			opt->po_data.c[0], opt->po_data.c[1], opt->po_data.c[2], opt->po_data.c[3]
			));
			opt->po_data.l = a;
			dprintf((" to %d.%d.%d.%d>",
			opt->po_data.c[0], opt->po_data.c[1], opt->po_data.c[2], opt->po_data.c[3]
			));
			res = OPT_NAK;
		}

		/* Handle what the destination thinks our address is */

		a = ppp_ipcp_getaddr(pp, 0);
		b = (&opt->po_data.l)[1];
		if (a == 0) {   /* Our src addr undefined -- accept peer's */
			dprintf((" *SET SRC ADDR <to %d.%d.%d.%d>*",
			opt->po_data.c[4], opt->po_data.c[5], opt->po_data.c[6], opt->po_data.c[7]
			));
			ppp_ipcp_setaddr(pp, b, 0);
		} else if (b != a) {
			dprintf((" *WRONG SRC ADDR <from %d.%d.%d.%d",
			opt->po_data.c[4], opt->po_data.c[5], opt->po_data.c[6], opt->po_data.c[7]
			));
			(&opt->po_data.l)[1] = a;
			dprintf((" to %d.%d.%d.%d>",
			opt->po_data.c[4], opt->po_data.c[5], opt->po_data.c[6], opt->po_data.c[7]
			));
			res = OPT_NAK;
		}

		return (res);

	case IPCP_CPROT:        /* IP-Compression */
		dprintf((" {CPROT: len=%d prot=0x%x ms=%d c=%d}",
			opt->po_len, ntohs(opt->po_data.s),
			opt->po_data.c[2], opt->po_data.c[3]));
		if (opt->po_len != 4 && opt->po_len != 6)
			return (OPT_REJ);
		if (!(pp->Ppp_dflt.ppp_flags & PPP_TCPC))
			return (OPT_REJ);
		res = OPT_OK;
		if (ntohs(opt->po_data.s) != PPP_VJC) {
			opt->po_data.s = htons(PPP_VJC);
			res = OPT_NAK;
		}
		if (opt->po_len == 6) {         /* rfc1332? */
			if (opt->po_data.c[2] >= MAX_STATES) {
				opt->po_data.c[2] = MAX_STATES - 1;
				res = OPT_NAK;
			}
			if (opt->po_data.c[3] != 0) {
				opt->po_data.c[3] = 0;
				res = OPT_NAK;
			}
		}
		if (res == OPT_OK)
			pp->Ppp_txflags |= PPP_TCPC;
		return (res);

	case IPCP_ADDR:
		dprintf((" {ADDR: len=%d addr=%d.%d.%d.%d}",
			opt->po_len, opt->po_data.c[0],
			opt->po_data.c[1], opt->po_data.c[2],
			opt->po_data.c[3]));
		if (opt->po_len != 6)
			return (OPT_REJ);

		/* Get dst IP address */
		a = ppp_ipcp_getaddr(pp, 1);
		if (a == 0) {   /* Our dst addr undefined -- accept peer's */
			dprintf((" *SET DST ADDR*"));
			ppp_ipcp_setaddr(pp, opt->po_data.l, 1);
		} else if (opt->po_data.l != a) {
			opt->po_data.l = a;
			return (OPT_NAK);
		}
		return (OPT_OK);
	}
	/*NOTREACHED*/
}

/*
 * Get IP address from the address list.
 * Returns 0 if there is no address.
 */
static u_long
ppp_ipcp_getaddr(pp, dst)
	struct p2pcom *pp;
	int dst;
{
	register struct ifaddr *ifa = pp->p2p_if.if_addrlist;
	struct sockaddr_in *s;

	while (ifa && ifa->ifa_addr->sa_family != AF_INET)
		ifa = ifa->ifa_next;
	if (ifa == 0)
		return (0);
	if (dst)
		s = (struct sockaddr_in *) (ifa->ifa_dstaddr);
	else
		s = (struct sockaddr_in *) (ifa->ifa_addr);
	if (s == 0)
		return (0);
	return (s->sin_addr.s_addr);
}

/*
 * Set remote interface address
 */
static void
ppp_ipcp_setaddr(pp, a, dst)
	struct p2pcom *pp;
	u_long a;
	int dst;
{
	struct socket so;
	struct ifreq ifr;
	struct sockaddr_in sin;

	so.so_state = SS_PRIV;
	bzero(&sin, sizeof sin);
	sin.sin_len = 4;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = a;
	ifr.ifr_addr = *(struct sockaddr *) &sin;;
	in_control(&so, dst? SIOCSIFDSTADDR : SIOCSIFADDR,
		   (caddr_t) &ifr, &pp->p2p_if);
}
#endif /* INET */
