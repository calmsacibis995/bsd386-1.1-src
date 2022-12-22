/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: ppp_lcp.c,v 1.3 1993/09/02 23:32:27 karels Exp $
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
 * PPP Link Control Protocol
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
#include "vmmeter.h"

#if INET
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"		/* XXX for slcompress */
#endif

#include "ppp_proto.h"
#include "if_p2p.h"

struct p2pcom   *ppp_ifs;       /* the list of active PPP interfaces */
int              ppp_idletimer; /* idle timer is active */

extern struct mbuf *ppp_addopt();

/*
 * Initialize the default values for PPP parameters
 */
static void
ppp_defaults(pp, name)
	struct p2pcom *pp;
	char *name;
{
	if (pp->Ppp_dflt.ppp_mru != 0)
		return;

	/* The default parameters is for the best case */
	pp->Ppp_dflt.ppp_cmap = 0x0;
	pp->Ppp_dflt.ppp_flags = PPP_FTEL;
	pp->Ppp_dflt.ppp_mru = LCP_DFLT_RMU;
	pp->Ppp_dflt.ppp_idletime = 0;   /* idle timer disabled */
	pp->Ppp_dflt.ppp_maxconf = 0;    /* connect timer disabled */
	pp->Ppp_dflt.ppp_maxterm = 3;    /* 3 attempts to terminate */
	pp->Ppp_dflt.ppp_timeout = 30;   /* 3 sec */

	/* Dial-up defauts are a bit different */
	if (!strcmp(name, "ppp")) {
		pp->Ppp_dflt.ppp_maxconf = 10;  /* 30 sec */
		pp->Ppp_dflt.ppp_flags |= PPP_PFC | PPP_ACFC | PPP_TCPC;
	}
#ifdef PPP_DEBUG
#if PPP_DEBUG == 1
	pp->Ppp_dflt.ppp_flags |= PPP_TRACE;
#endif
#endif
	pp->Ppp_params = pp->Ppp_dflt;
}

/*
 * The PPP ioctl routine
 */
int
ppp_ioctl(pp, cmd, data)
	struct p2pcom *pp;
	int cmd;
	caddr_t data;
{
	struct ppp_ioctl *io = ifr_pppioctl(data);
	int s = splimp();
	int error;
	extern struct ifnet *ppp_dropif;

	error = 0;
	switch (cmd) {
	case PPPIOCNPAR:
		*io = pp->Ppp_params;
		if (pp->Ppp_lcp.fsm_state == CP_OPENED)
			break;
		/* FALL THROUGH */
	case PPPIOCGPAR:
		ppp_defaults(pp, pp->p2p_if.if_name);
		*io = pp->Ppp_dflt;
		break;

	case PPPIOCSPAR:
		if (io->ppp_mru < LCP_MIN_RMU || io->ppp_timeout < 1 ||
		    io->ppp_maxterm < 1)
			error = EINVAL;
		else
			pp->Ppp_dflt = *io;
		break;

	case PPPIOCWAIT:        /* wait for a dropped packet */
		if_qflush(&pp->Ppp_ipdefq);
		pp->Ppp_wdrop = 1;
		error = tsleep((caddr_t) &pp->Ppp_wdrop,
			       PCATCH|(PZERO+10), "pppwait", 0);
		break;

	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}

/*
 * Initialize the PPP protocol on an interface
 */
int
ppp_init(pp)
	struct p2pcom *pp;
{
	extern void ppp_idlepoll();
	int s;

	s = splimp();

	pp->p2p_prevp = &ppp_ifs;
	if (pp->p2p_next = ppp_ifs)
		ppp_ifs->p2p_prevp = &pp->p2p_next;
	ppp_ifs = pp;
	if (ppp_idletimer == 0) {
		ppp_idletimer++;
		timeout(ppp_idlepoll, 0, hz);
	}

	dprintf(("%s%d: init PPP link\n", pp->p2p_if.if_name,
	    pp->p2p_if.if_unit));

	pp->Ppp_wdrop = 1;      /* For any case... */
	pp->Ppp_lcp.fsm_state = CP_STARTING;
#ifdef INET
	pp->Ppp_ipcp.fsm_state = CP_STARTING;
#endif

	/*
	 * Raise DTR (assume it is ON on local lines)
	 * We should get the "got CD event"
	 */
	if (pp->p2p_mdmctl)
		(*pp->p2p_mdmctl)(pp, 1);
	else
		ppp_modem(pp, 1);
	splx(s);
	return (0);
}

/*
 * Shutdown protocol on an interface
 */
void
ppp_shutdown(pp)
	struct p2pcom *pp;
{
	int s;
	struct mbuf *m;

	dprintf(("%s%d: shutdown PPP link\n", pp->p2p_if.if_name,
	    pp->p2p_if.if_unit));

	s = splimp();
	if (pp->p2p_next)
		pp->p2p_next->p2p_prevp = pp->p2p_prevp;
	*(pp->p2p_prevp) = pp->p2p_next;

	ppp_cp_close(pp, FSM_LCP);
	splx(s);
}

/*
 * Turn upper levels Up
 */
void
ppp_lcp_tlu(pp)
	struct p2pcom *pp;
{

	/* Load the async control characters map */
	pp->Ppp_txcmap = pp->Ppp_cmap;

#ifdef INET
	sl_compress_init(&pp->Ppp_ctcp);
	pp->Ppp_ncps++;
	ppp_cp_up(pp, FSM_IPCP);
#endif
	pp->p2p_if.if_flags |= IFF_RUNNING;
}

/*
 * Turn upper levels Down
 */
void
ppp_lcp_tld(pp)
	struct p2pcom *pp;
{

#ifdef INET
	ppp_cp_down(pp, FSM_IPCP);
#endif
}

/*
 * Timeout
 */
void
ppp_lcp_timeout(pp)
	struct p2pcom *pp;
{
	ppp_cp_timeout(pp, FSM_LCP);
}

/*
 * Handle the extra LCP packet codes
 */
struct mbuf *
ppp_lcp_xcode(pp, m, cp)
	struct p2pcom *pp;
	struct mbuf *m;
	struct ppp_cp_hdr *cp;
{
	u_short type;

	switch (cp->cp_code) {
	default:
		m = (struct mbuf *) -1;
		break;

	case LCP_PROTO_REJ:     /* Protocol-Reject */
		type = ntohs(*mtod(m, u_short *));
		switch (type) {
#ifdef INET
		case PPP_IP:
		case PPP_IPCP:
			dprintf((" IP disabled\n"));
			ppp_cp_close(pp, FSM_IPCP);
			break;
#endif

		default:
			dprintf((" <0x%x> -- dropped\n", type));
			break;
		}
		m_freem(m);
		m = 0;
		break;

	case LCP_ECHO_REQ:      /* Echo-Request */
		dprintf((" magic=%x -- send Echo-Reply\n", *mtod(m, u_long *)));
		*mtod(m, u_long *) = pp->Ppp_magic;
		m = ppp_cp_prepend(m, PPP_LCP, LCP_ECHO_REPL, cp->cp_id, 0);
		break;

	case LCP_ECHO_REPL:     /* Echo-Reply */
	case LCP_DISC_REQ:      /* Discard-Request */
		dprintf((" magic=%x -- dropped\n", *mtod(m, u_long *)));
		m_freem(m);
		m = 0;
		break;
	}
	return (m);
}

/*
 * Compose a LCP Config-Request packet
 */
struct mbuf *
ppp_lcp_creq(pp)
	struct p2pcom *pp;
{
	register struct mbuf *m;
	struct ppp_option opt;

	dprintf(("LCP_CREQ --"));
	m = 0;

	/* Max-Receive-Unit */
	if (!(pp->Ppp_lcp.fsm_rej & (1<<LCP_MRU))) {
		dprintf((" {MRU: %d}", pp->Ppp_mru));
		opt.po_type = LCP_MRU;
		opt.po_len = 4;
		opt.po_data.s = htons(pp->Ppp_mru);
		m = ppp_addopt(m, &opt);
	}

	/* Asynchronous-Control-Characters-Map */
	if (!(pp->Ppp_lcp.fsm_rej & (1<<LCP_ACCM))) {
		dprintf((" {ACCM: %x}", pp->Ppp_cmap));
		opt.po_type = LCP_ACCM;
		opt.po_len = 6;
		opt.po_data.l = htonl(pp->Ppp_cmap);
		m = ppp_addopt(m, &opt);
	}

	/* Magic-Number */
	if (!(pp->Ppp_lcp.fsm_rej & (1<<LCP_MAGIC))) {
		dprintf((" {MAGIC: 0x%x}", pp->Ppp_magic));
		opt.po_type = LCP_MAGIC;
		opt.po_len = 6;
		opt.po_data.l = htonl(pp->Ppp_magic);
		m = ppp_addopt(m, &opt);
	}

	/* Protocol-Field-Compression */
	if (!(pp->Ppp_lcp.fsm_rej & (1<<LCP_PFC)) &&
	    (pp->Ppp_flags & PPP_PFC)) {
		dprintf((" {PFC}"));
		opt.po_type = LCP_PFC;
		opt.po_len = 2;
		m = ppp_addopt(m, &opt);
	}

	/* Address-and-Control-Field-Compression */
	if (!(pp->Ppp_lcp.fsm_rej & (1<<LCP_ACFC)) &&
	    (pp->Ppp_flags & PPP_ACFC)) {
		dprintf((" {ACFC}"));
		opt.po_type = LCP_ACFC;
		opt.po_len = 2;
		m = ppp_addopt(m, &opt);
	}
	dprintf(("\n"));
	m = ppp_cp_prepend(m, PPP_LCP, CP_CONF_REQ, ++(pp->Ppp_lcp.fsm_id), 0);
	return (m);
}

/*
 * Process the rejected option
 */
void
ppp_lcp_optrej(pp, opt)
	struct p2pcom *pp;
	struct ppp_option *opt;
{

	switch (opt->po_type) {
	case LCP_MRU:
		dprintf((" {MRU: len=%d mtu=%d (use 1500)}",
			opt->po_len, ntohs(opt->po_data.s)));
		/* use the RFC default value */
		pp->Ppp_mru = HDLCMTU;
		break;

	case LCP_PFC:
		dprintf((" {PFC}"));
		/* indicate that peer won't send compressed */
		pp->Ppp_flags &= ~PPP_PFC;
		break;

	case LCP_ACFC:
		dprintf((" {ACFC}"));
		/* indicate that peer won't send compressed */
		pp->Ppp_flags &= ~PPP_ACFC;
		break;

	default:
		dprintf((" {%d: len=%d}", opt->po_type, opt->po_len));
	}
}

/*
 * Process the NAK-ed option
 */
void
ppp_lcp_optnak(pp, opt)
	struct p2pcom *pp;
	struct ppp_option *opt;
{

	switch (opt->po_type) {
	case LCP_MRU:
		dprintf((" {MRU: len=%d mtu=%d}",
			opt->po_len, ntohs(opt->po_data.s)));
		/* agree on what the peer says */
		pp->Ppp_mru = ntohs(opt->po_data.s);
		break;

	case LCP_ACCM:
		dprintf((" {ACCM: len=%d m=0x%x}",
			opt->po_len, ntohl(opt->po_data.l)));
		/* agree on what the peer says */
		pp->Ppp_cmap |= ntohl(opt->po_data.l);
		break;

	case LCP_MAGIC:
		dprintf((" {MAGIC: len=%d m=0x%x}",
			opt->po_len, ntohl(opt->po_data.l)));
		if (pp->Ppp_magic == ntohl(opt->po_data.l))
			pp->Ppp_magic = ppp_lcp_magic();
		break;

	default:
		dprintf((" {%d: len=%d}", opt->po_type, opt->po_len));
	}
}

/*
 * Process the received option
 */
int
ppp_lcp_opt(pp, opt)
	struct p2pcom *pp;
	struct ppp_option *opt;
{
	u_long mask;

	switch (opt->po_type) {
	case LCP_MRU:   /* Max-Receive-Unit */
		dprintf((" {MRU: len=%d mtu=%d}",
			opt->po_len, ntohs(opt->po_data.s)));
		if (opt->po_len != 4)
			return (OPT_REJ);
		pp->p2p_if.if_mtu = ntohs(opt->po_data.s);
		if (pp->p2p_if.if_mtu < LCP_MIN_RMU) {
			opt->po_data.s = htons(LCP_MIN_RMU);
			return (OPT_NAK);
		}
		return (OPT_OK);

	case LCP_ACCM:
		dprintf((" {ACCM: len=%d m=0x%x}",
			opt->po_len, ntohl(opt->po_data.l)));
		if (opt->po_len != 6)
			return (OPT_REJ);
		mask = ntohl(opt->po_data.l);
		if (~mask & pp->Ppp_cmap) {
			opt->po_data.l = htonl(mask | pp->Ppp_cmap);
			return (OPT_NAK);
		}
		pp->Ppp_cmap = mask;
		return (OPT_OK);

	case LCP_AP:
		dprintf((" {AP}"));
		/* currently not supported */
		return (OPT_REJ);

	case LCP_QP:
		dprintf((" {QP}"));
		/* currently not supported */
		return (OPT_REJ);

	case LCP_MAGIC:
		dprintf((" {MAGIC: len=%d m=0x%x}",
			opt->po_len, ntohl(opt->po_data.l)));
		if (opt->po_len != 6)
			return (OPT_REJ);
		mask = ntohl(opt->po_data.l);
		if (pp->Ppp_magic == mask || mask == 0) {
			if (pp->Ppp_badmagic++ >= 3) {
				/* say it only once */
				if (pp->Ppp_badmagic == 4)
					printf("%s%d: link looped",
					    pp->p2p_if.if_name,
					    pp->p2p_if.if_unit);
				return (OPT_FATAL);
			}
			pp->Ppp_magic = ppp_lcp_magic();
			opt->po_data.l = htonl(pp->Ppp_magic);
			return (OPT_NAK);
		}
		return (OPT_OK);

	case LCP_PFC:
		dprintf((" {PFC: len=%d}", opt->po_len));
		if (opt->po_len != 2)
			return (OPT_REJ);
		if (!(pp->Ppp_flags & PPP_PFC))
			/* we ain't need no stinking compression */
			return (OPT_REJ);
		pp->Ppp_txflags |= PPP_PFC;
		break;

	case LCP_ACFC:
		dprintf((" {ACFC: len=%d}", opt->po_len));
		if (opt->po_len != 2)
			return (OPT_REJ);
		if (!(pp->Ppp_flags & PPP_ACFC))
			/* we ain't need no stinking compression */
			return (OPT_REJ);
		pp->Ppp_txflags |= PPP_ACFC;
		return (OPT_OK);

	default:
		dprintf((" {?%d: len=%d}", opt->po_type, opt->po_len));
		return (OPT_REJ);
	}
	/*NOTREACHED*/
}

/*
 * This level finished -- drop DTR
 */
void
ppp_lcp_tlf(pp)
	struct p2pcom *pp;
{

	pp->p2p_if.if_flags &= ~IFF_RUNNING;
	if (pp->p2p_mdmctl)
		(*pp->p2p_mdmctl)(pp, 0);
}

/*
 * Modem events handling routine
 * (0 - DCD dropped; 1 - DCD raised)
 */
int
ppp_modem(pp, flag)
	struct p2pcom *pp;
	int flag;
{
	int s;

	/*
	 * Got DCD -- send out Config-Request
	 */
	s = splimp();
	if (flag) {
		if (pp->Ppp_lcp.fsm_state == CP_INITIAL) {
			pp->Ppp_lcp.fsm_state = CP_CLOSED;
			splx(s);
			return (0);
		}
		if (pp->Ppp_lcp.fsm_state != CP_STARTING) {
			splx(s);
			return (0);
		}
		dprintf(("%s%d: got CD -- send Conf-Req\n", pp->p2p_if.if_name,
		    pp->p2p_if.if_unit));

		/* Initialize parameters */
		s = splimp();
		ppp_defaults(pp, pp->p2p_if.if_name);
		pp->Ppp_txcmap = 0xfffffff;       /* God save */
		pp->Ppp_params = pp->Ppp_dflt;
		pp->Ppp_txflags = 0;
		pp->Ppp_magic = ppp_lcp_magic();
		pp->Ppp_badmagic = 0;
		pp->Ppp_ncps = 0;
		pp->Ppp_idlecnt = 0; /* wait till the first data packet goes thru */

		/* Calculate timer parameters */
		pp->Ppp_ticks = (pp->Ppp_timeout * hz) / 10;
		if (pp->Ppp_ticks < 2)
			pp->Ppp_ticks = 2;

		ppp_cp_up(pp, FSM_LCP);

	} else {
		/*
		 * Carrier lost -- turn off upper levels and return
		 * to the beginning.
		 */
		dprintf(("%s%d: CD lost\n", pp->p2p_if.if_name,
		    pp->p2p_if.if_unit));
		pp->p2p_if.if_flags &= ~IFF_RUNNING;
		ppp_cp_down(pp, FSM_LCP);
		if_qflush(&pp->p2p_isnd);
		if_qflush(&pp->p2p_if.if_snd);
#ifdef INET
		if_qflush(&pp->Ppp_ipdefq);
#endif

		/*
		 * Keep the active state on idle timeouts and
		 * carrier losses -- we may reconnect without
		 * toggling IFF_UP.
		 */
		if (pp->p2p_if.if_flags & IFF_UP)
			pp->Ppp_lcp.fsm_state = CP_STARTING;
	}
	splx(s);
	return (0);
}

/*
 * Calculate the magic number
 */
u_long
ppp_lcp_magic()
{
	u_long l;
	extern struct timeval time;

	l = time.tv_sec ^ time.tv_usec ^ cnt.v_trap;
	return (l * 1192721);   /* randomize it a bit */
}
