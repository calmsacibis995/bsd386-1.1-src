/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: ppp_fsm.c,v 1.3 1993/04/17 17:57:18 karels Exp $
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
 * PPP Control Protocols Finite State Machine
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

#if INET
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"		/* XXX for slcompress */
#endif

#include "machine/mtpr.h"
#include "ppp_proto.h"
#include "if_p2p.h"

#ifdef PPP_DEBUG
#define newstate(fs, state)	{ (fs)->fsm_state = (state); \
				  dprintf(("state <- " #state "\n")); }
#else
#define newstate(fs, state)	(fs)->fsm_state = (state)
#endif

/*
 * The table of protocol events/actions
 */
struct ppp_fsm_tab {
	u_short	ft_type;	/* protocol number */
	u_short	ft_offset;	/* the byte offset of a state structure in ppp_sc */
	char	*ft_name;	/* protocol name */

	struct	mbuf * (*ft_creq) __P((struct p2pcom *));
					/* Config-Request composition */
	struct	mbuf * (*ft_xcodes) __P((struct p2pcom *, struct mbuf *,
					struct ppp_cp_hdr *));
					/* extra codes handling routine */
	void	(*ft_tlu) __P((struct p2pcom *));
					/* top levels up */
	void	(*ft_tld) __P((struct p2pcom *));
					/* top levels down */
	void	(*ft_tlf) __P((struct p2pcom *));
					/* this level finished */
	void	(*ft_optnak) __P((struct p2pcom *, struct ppp_option *));
					/* process nak-ed options */
	void	(*ft_optrej) __P((struct p2pcom *, struct ppp_option *));
					/* process rejected options */
	int	(*ft_opt) __P((struct p2pcom *, struct ppp_option *));
					/* process peer's options */
	void	(*ft_timeout) __P((struct p2pcom *));
					/* timeout handler */
};

static struct ppp_fsm_tab ppp_fsm_tab[] = {
	/* Line Control Protocol -- FSM_LCP = 0 */
	{
		PPP_LCP,
		(u_short) &(((struct p2pcom *)0)->p2p_ppp.ppp_lcp),
		"LCP",

		ppp_lcp_creq,
		ppp_lcp_xcode,
		ppp_lcp_tlu,
		ppp_lcp_tld,
		ppp_lcp_tlf,
		ppp_lcp_optnak,
		ppp_lcp_optrej,
		ppp_lcp_opt,
		ppp_lcp_timeout
	},

	/* IP Control Protocol -- FSM_IPCP = 1 */
	{
#ifdef INET
		PPP_IPCP,
		(u_short) &(((struct p2pcom *)0)->p2p_ppp.ppp_ipcp),
		"IPCP",

		ppp_ipcp_creq,
		(struct mbuf *(*)())0,  /* xcode */
		ppp_ipcp_tlu,
		(void (*)())0,		/* tld */
		ppp_ipcp_tlf,
		ppp_ipcp_optnak,
		ppp_ipcp_optrej,
		ppp_ipcp_opt,
		ppp_ipcp_timeout,
#else
		PPP_LCP, 0, "?", 0, 0, 0, 0, 0, 0, 0, 0, 0
#endif
	}
};

/*
 * Prepend the PPP frame and CP headers
 */
struct mbuf *
ppp_cp_prepend(m, type, code, id, xlen)
	register struct mbuf *m;
	int type;
	int code;
	int id;
	int xlen;
{
	register struct ppp_header *ph;
	register struct ppp_cp_hdr *lh;

	xlen += sizeof(struct ppp_header) + sizeof(struct ppp_cp_hdr);
	if (m == 0) {
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		m->m_pkthdr.len = m->m_len = xlen;
		m->m_pkthdr.rcvif = 0;
	} else
		M_PREPEND(m, xlen, M_DONTWAIT);
	if (m == 0)
		return (0);

	ph = mtod(m, struct ppp_header *);
	ph->phdr_addr = PPP_ADDRESS;
	ph->phdr_ctl = PPP_CONTROL;
	ph->phdr_type = htons(type);
	lh = (struct ppp_cp_hdr *) (ph + 1);
	lh->cp_code = code;
	lh->cp_id = id;
	lh->cp_length = htons(m->m_pkthdr.len - sizeof(struct ppp_header));
	return (m);
}

/*
 * Add an option to the list
 */
struct mbuf *
ppp_addopt(m0, opt)
	struct mbuf *m0;
	struct ppp_option *opt;
{
	register struct mbuf *m;
	struct mbuf *m1;

	if (m0 == 0) {
		MGETHDR(m0, M_DONTWAIT, MT_DATA);
		if (m0 == 0)
			return (0);
		/* leave some space for headers */
		m0->m_data += sizeof(struct ppp_header) +
			      sizeof(struct ppp_cp_hdr);
		m0->m_pkthdr.len = m0->m_len = 0;
	}
	m = m0;
	while (m->m_next)
		m = m->m_next;
	if (m->m_len + opt->po_len + m->m_data >= &m->m_dat[MLEN]) {
		MGET(m1, M_DONTWAIT, MT_DATA);
		if (m1 == 0) {
			m_freem(m0);
			return (0);
		}
		m->m_next = m1;
		m = m1;
	}
	bcopy(&opt->po_type, m->m_data + m->m_len, opt->po_len);
	m->m_len += opt->po_len;
	m0->m_pkthdr.len += opt->po_len;
	return (m0);
}

#define CP_DATA(m) (mtod((m), caddr_t) + sizeof(struct ppp_header) +\
		    sizeof(struct ppp_cp_hdr))

/*
 * Process an incoming control packet
 */
void
ppp_cp_in(pp, m, type)
	struct p2pcom *pp;
	struct mbuf *m;
	int type;
{
	struct ppp_cp_hdr *cph;
	struct ppp_fsm *fs;
	struct ppp_fsm_tab *ft;
	struct ppp_option opt;
	struct mbuf *m0;
	int l, off, ol;
	u_long mask;
	int reject, nak;
	u_char cpid;
	static char *pkcodes[] = {
		"?", "Configure-Request", "Configure-Ack",
		"Configure-Nak", "Configure-Reject",
		"Terminate-Request", "Terminate-Ack",
		"Code-Reject", "Protocol-Reject",
		"Echo-Request", "Echo-Reply", "Discard-Request"
	};

	/*
	 * Look for a protocol in the table
	 */
	ft = ppp_fsm_tab;
	for (l = (sizeof ppp_fsm_tab) / (sizeof ppp_fsm_tab[0]); l--; ft++) {
		if (ft->ft_type == type)
			goto found;
	}

	/*
	 * Handle bad (unknown) protocol
	 */
	if (pp->Ppp_lcp.fsm_state != CP_OPENED) {
		dprintf(("%s%d: illegal protocol %x (len=%d) -- discarded\n",
			pp->p2p_if.if_name, pp->p2p_if.if_unit, type,
			m->m_pkthdr.len));
		m_freem(m);
		return;
	}

	/* Truncate the rejected packet */
	l = pp->Ppp_mru - sizeof(struct ppp_cp_hdr) + 2;
	if (m->m_pkthdr.len > l) {
		off = 0;
		for (m0 = m; m0 != 0; m0 = m0->m_next) {
			off += m->m_len;
			if (off >= l)
				break;
		}
		if (m->m_next)
			m_freem(m->m_next);
		m->m_next = 0;
		m->m_len -= off - l;
		m->m_pkthdr.len = l;
	}
	dprintf(("%s%d: illegal protocol %x (len=%d) -- send Protocol-Reject\n",
		pp->p2p_if.if_name, pp->p2p_if.if_unit, type, m->m_pkthdr.len));

	/* Prepend it with the frame and LCP headers and 2-byte proto code */
	m = ppp_cp_prepend(m, PPP_LCP, LCP_PROTO_REJ, 0, 2);
	if (m == 0)
		return;
	*(u_short *)CP_DATA(m) = htons(type);
	goto sendout;

	/* Protocol is known -- get the pointer to fsm data */
found:  fs = (struct ppp_fsm *) ((char *)pp + ft->ft_offset);

	/*
	 * Check and skip the CP header
	 */
	cph = mtod(m, struct ppp_cp_hdr *);
	l = ntohs(cph->cp_length);

	dprintf(("%s%d: RCV %s%d %s id=%d len=%d ",
		pp->p2p_if.if_name, pp->p2p_if.if_unit, ft->ft_name,
		cph->cp_code, (cph->cp_code < 12) ? pkcodes[cph->cp_code] : "?",
		cph->cp_id, l));

	if (l > m->m_pkthdr.len || l < 0) {
		dprintf((" BAD LENGTH (total=%d)\n", l));
		m_freem(m);
		return;
	}
	m->m_data += sizeof(struct ppp_cp_hdr);
	m->m_len -= sizeof(struct ppp_cp_hdr);
	m->m_pkthdr.len -= sizeof(struct ppp_cp_hdr);
	l -= sizeof(struct ppp_cp_hdr);

	/* Ignore incoming packets if link is supposed to be down */
	if (fs->fsm_state == CP_INITIAL || fs->fsm_state == CP_STARTING) {
		m_freem(m);
		return;
	}

	/*
	 * Select by incoming packet code
	 */
	cpid = cph->cp_id;
	switch (cph->cp_code) {
	default:			/* Unknown code */
		if (ft->ft_xcodes != 0) {
			m = (*ft->ft_xcodes)(pp, m, cph);
			if (m != (struct mbuf *)(-1))
				goto sendout;
		}

		/* Report unknown code and senf Conf-Reject */
		m->m_data -= sizeof(struct ppp_cp_hdr);
		m->m_len += sizeof(struct ppp_cp_hdr);
		m->m_pkthdr.len += sizeof(struct ppp_cp_hdr);
		dprintf(("\n"));
		printf("%s%d: unknown ctl code %d received (len=%d)\n",
			pp->p2p_if.if_name, pp->p2p_if.if_unit, ft->ft_name,
			cph->cp_code, m->m_pkthdr.len);
		m = ppp_cp_prepend(m, type, CP_CODE_REJ, 0, 0);
		break;

	case CP_CONF_REQ:		/* Configure-Request */
		switch (fs->fsm_state) {
		case CP_CLOSING:
		case CP_STOPPING:
			dprintf((" -- ignored\n"));
			m_freem(m);
			return;

		case CP_CLOSED:
			dprintf((" -- send Terminate-Ack\n"));
			m_freem(m);
			m = ppp_cp_prepend(0, type, CP_TERM_ACK, cpid, 0);
			goto sendout;

		case CP_STOPPED:
			/* Initialize restart counter */
			fs->fsm_rc = pp->Ppp_maxconf;
			/* FALLTHROUGH */

		case CP_OPENED:
			/* Set conservative TX ccm */
			if (type == PPP_LCP)
				pp->Ppp_txcmap = 0xffffffff;
			dprintf((" -- send Conf-Req AGAIN\n"));

			/*
			 * The messy case of resynching from OPENED state
			 */
			if (ft->ft_tld)
				(*ft->ft_tld)(pp);

			/* Send out our own Configure-Request */
			m0 = (*ft->ft_creq)(pp);
			timeout(ft->ft_timeout, pp, pp->Ppp_ticks);
			if (m0 != 0) {
			    if (IF_QFULL2(&pp->p2p_if.if_snd, &pp->p2p_isnd)) {
				IF_DROP(&pp->p2p_if.if_snd);
				m_freem(m0);
			    } else {
				pp->p2p_if.if_obytes += m->m_pkthdr.len;
				IF_ENQUEUE(&pp->p2p_isnd, m0);
				if ((pp->p2p_if.if_flags & IFF_OACTIVE) == 0)
					(*pp->p2p_if.if_start)(&pp->p2p_if);
			    }
			}
		}

		/* Parse options */
		off = 0;
		m0 = 0;
		reject = nak = 0;
		while (off < l) {
			/* get the next option from the list */
			ol = LCP_OPT_MAXSIZE;
			if (ol + off > l)
				ol = l - off;
			m_copydata(m, off, ol, &opt.po_type);
			off += opt.po_len;

			/* check the length */
			if (opt.po_len > LCP_OPT_MAXSIZE) {
				dprintf((" {LONG! len=%d}", opt.po_len));
				goto reject;
			}

			/* process the option */
			switch ((*ft->ft_opt)(pp, &opt)) {
			case OPT_OK:
				continue;

			case OPT_REJ:
			reject:
				dprintf((" REJECT"));
				reject++;
				if (nak && m0) {
					m_freem(m0);
					m0 = 0;
				}
				break;

			case OPT_NAK:
				dprintf((" NAK"));
				nak++;
				break;

			case OPT_FATAL:
				dprintf((" FATAL\n"));
				if (m0)
					m_freem(m0);
				m_freem(m);
				ppp_cp_close(pp, ft - ppp_fsm_tab);
				return;
			}
			m0 = ppp_addopt(m0, &opt);
		}

		/* Send Nak/Reject/Ack depending on circumstances */
		if (reject) {
			m_freem(m);
			if (m0 == 0)
				return;
			dprintf((" -> send Conf-Rej\n"));
			m = ppp_cp_prepend(m0, type, CP_CONF_REJ, cpid, 0);
			if (fs->fsm_state != CP_ACKRCVD)
				newstate(fs, CP_REQSENT);
		} else if (nak) {
			m_freem(m);
			if (m0 == 0)
				return;
			m = ppp_cp_prepend(m0, type, CP_CONF_NAK, cpid, 0);
			dprintf((" -> send Conf-Nak (len=%d)\n", m->m_pkthdr.len));
			if (fs->fsm_state != CP_ACKRCVD)
				newstate(fs, CP_REQSENT);
		} else {
			m = ppp_cp_prepend(m, type, CP_CONF_ACK, cpid, 0);
			dprintf((" -> send Conf-Ack (len=%d)\n", m->m_pkthdr.len));

			/*
			 * We SHOULD queue Conf-Ack first, and only then
			 * turn on upper levels.
			 */
			if (m == 0)
				return;
			if (IF_QFULL2(&pp->p2p_if.if_snd, &pp->p2p_isnd)) {
				IF_DROP(&pp->p2p_if.if_snd);
				m_freem(m);
				return;
			}
			pp->p2p_if.if_obytes += m->m_pkthdr.len;
			IF_ENQUEUE(&pp->p2p_isnd, m);
			if ((pp->p2p_if.if_flags & IFF_OACTIVE) == 0)
				(*pp->p2p_if.if_start)(&pp->p2p_if);

			/* Change FSM state and issue Up to upper levels */
			if (fs->fsm_state == CP_ACKRCVD) {
				newstate(fs, CP_OPENED);
				if (ft->ft_tlu)
					(*ft->ft_tlu)(pp);
			} else
				newstate(fs, CP_ACKSENT);
			return;
		}
		break;

	case CP_CONF_ACK:		/* Configure-Ack */
		m_freem(m);
		if (cpid != fs->fsm_id) {
			dprintf((" -- wrong id %d (exp %d)\n",
				cpid, fs->fsm_id));
			return;
		}

		/* Clear timeouts */
		untimeout(ft->ft_timeout, pp);

		switch (fs->fsm_state) {
		case CP_CLOSED:
		case CP_STOPPED:
			dprintf((" -- send Terminate-Ack\n"));
			m = ppp_cp_prepend(0, type, CP_TERM_ACK, cpid, 0);
			break;

		default:
			dprintf((" -- ignored\n"));
			return;

		case CP_REQSENT:
			dprintf((" -- init restart cntr\n"));
			newstate(fs, CP_ACKRCVD);
			/* Initialize restart counter */
			fs->fsm_rc = pp->Ppp_maxconf;
			return;

		case CP_OPENED:
			if (ft->ft_tld)
				(*ft->ft_tld)(pp);
			/* FALLTHROUGH */

		case CP_ACKRCVD:
			dprintf((" -- send Conf-Req\n"));
			newstate(fs, CP_REQSENT);
			/* Send Config-Request */
			m = (*ft->ft_creq)(pp);
			timeout(ft->ft_timeout, pp, pp->Ppp_ticks);
			break;

		case CP_ACKSENT:
			newstate(fs, CP_OPENED);
			/* Initialize restart counter */
			fs->fsm_rc = pp->Ppp_maxconf;
			if (ft->ft_tlu)
				(*ft->ft_tlu)(pp);
			return;
		}
		break;

	case CP_CONF_NAK:		/* Configure-Nak */
	case CP_CONF_REJ:		/* Configure-Reject */
		if (cpid != fs->fsm_id) {
			m_freem(m);
			dprintf((" -- wrong id %d (exp %d)\n",
				cpid, fs->fsm_id));
			return;
		}

		/* Clear timeouts */
		untimeout(ft->ft_timeout, pp);

		switch (fs->fsm_state) {
		default:
			m_freem(m);
			dprintf((" -- ignored\n"));
			return;

		case CP_CLOSED:
		case CP_STOPPED:
			m_freem(m);
			dprintf((" -- send Terminate-Ack\n"));
			m = ppp_cp_prepend(0, type, CP_TERM_ACK, cpid, 0);
			goto sendout;

		case CP_REQSENT:
		case CP_ACKSENT:
			/* Initialize restart counter */
			fs->fsm_rc = pp->Ppp_maxconf;
			break;

		case CP_OPENED:
			if (ft->ft_tld)
				(*ft->ft_tld)(pp);
			/*FALLTHROUGH*/
		case CP_ACKRCVD:
			newstate(fs, CP_REQSENT);
			break;
		}

		/* Parse options */
		off = 0;
		while (off < l) {
			/* get the next option from the list */
			ol = LCP_OPT_MAXSIZE;
			if (ol + off > l)
				ol = l - off;
			m_copydata(m, off, ol, &opt.po_type);
			off += opt.po_len;

			if (cph->cp_code == CP_CONF_REJ) {
				fs->fsm_rej |= 1 << opt.po_type;
				(*ft->ft_optrej)(pp, &opt);
			} else
				(*ft->ft_optnak)(pp, &opt);

		}
		m_freem(m);

		/* Send the Configure-Request */
		dprintf((" -- send Conf-Req\n"));
		m = (*ft->ft_creq)(pp);
		timeout(ft->ft_timeout, pp, pp->Ppp_ticks);
		break;

	case CP_TERM_REQ:
		switch (fs->fsm_state) {
		case CP_OPENED:
			fs->fsm_rc = 1;		/* Zero the restart counter */
			if (ft->ft_tld)
				(*ft->ft_tld)(pp);
			newstate(fs, CP_STOPPING);
			break;

		case CP_ACKRCVD:
		case CP_ACKSENT:
			newstate(fs, CP_REQSENT);
			break;
		}

		/* Set conservative TX ccm */
		if (type == PPP_LCP)
			pp->Ppp_txcmap = 0xffffffff;

		dprintf((" -- send Term-Ack\n"));
		m = ppp_cp_prepend(m, type, CP_TERM_ACK, cpid, 0);
		break;

	case CP_TERM_ACK:
		m_freem(m);

		/* Clear timeouts */
		untimeout(ft->ft_timeout, pp);

		switch (fs->fsm_state) {
		case CP_OPENED:
			dprintf((" -- send Conf-Req\n"));
			if (ft->ft_tld)
				(*ft->ft_tld)(pp);
			m = (*ft->ft_creq)(pp);
			timeout(ft->ft_timeout, pp, pp->Ppp_ticks);
			goto sendout;

		case CP_ACKRCVD:
			newstate(fs, CP_REQSENT);
			/* FALLTHROUGH */
		default:
			dprintf((" -- no action\n"));
			break;

		case CP_CLOSING:
			newstate(fs, CP_CLOSED);
			goto drop_conn;

		case CP_STOPPING:
			newstate(fs, CP_STOPPED);
		drop_conn:
			if_qflush(&pp->p2p_isnd);
			if_qflush(&pp->p2p_if.if_snd);

			/* Clear timeouts */
			untimeout(ft->ft_timeout, pp);

			/* This level finished! */
			if (ft->ft_tlf)
				(*ft->ft_tlf)(pp);
			break;
		}
		return;

	case CP_CODE_REJ:	/* Code-Reject */
		/* We've got a problem! */
		dprintf(("\n"));
		printf("%s%d: %s Code-Reject received (for code %d)\n",
			pp->p2p_if.if_name, pp->p2p_if.if_unit, ft->ft_name,
			mtod(m, struct ppp_cp_hdr *)->cp_code);
		m_freem(m);
		return;
	}

sendout:
	if (m == 0)
		return;
	/* Queue the packet to the high-priority queue */
	if (IF_QFULL2(&pp->p2p_if.if_snd, &pp->p2p_isnd)) {
		IF_DROP(&pp->p2p_if.if_snd);
		m_freem(m);
	} else {
		pp->p2p_if.if_obytes += m->m_pkthdr.len;
		IF_ENQUEUE(&pp->p2p_isnd, m);
		if ((pp->p2p_if.if_flags & IFF_OACTIVE) == 0)
			(*pp->p2p_if.if_start)(&pp->p2p_if);
	}
}

/*
 * Restart timeout
 */
void
ppp_cp_timeout(pp, fsm)
	struct p2pcom *pp;
	int fsm;
{
	struct ppp_fsm *fs;
	extern int ppp_input();
	int s;
	struct mbuf *m;
	struct ppp_fsm_tab *ft = &ppp_fsm_tab[fsm];

	s = splimp();
	if (pp->p2p_input != ppp_input) {	/* Protocol has been changed */
		splx(s);
		return;
	}

	fs = (struct ppp_fsm *) ((char *)pp + ft->ft_offset);
	dprintf(("%s%d: %s[rc=%d] TO", pp->p2p_if.if_name, pp->p2p_if.if_unit,
		ft->ft_name, fs->fsm_rc));

	if (fs->fsm_rc == 1) {	/* TO- */
		dprintf(("- Drop Connection\n"));
		fs->fsm_rc = 0;

		/* Go to the new state */
		if (fs->fsm_state == CP_CLOSING) {
			newstate(fs, CP_CLOSED);
		} else
			newstate(fs, CP_STOPPED);

		/* This level finished! */
		if (ft->ft_tlf)
			(*ft->ft_tlf)(pp);

	} else {		/* TO+ */
		if (fs->fsm_rc > 0)
			fs->fsm_rc--;

		switch (fs->fsm_state) {
		default:
			dprintf(("+ -- illegal state %d\n", fs->fsm_state));
			splx(s);
			return;

		case CP_CLOSING:
		case CP_STOPPING:
			/* Send the termination request */
			dprintf(("+ - send Term-Req\n"));
			m = ppp_cp_prepend(0, ft->ft_type, CP_TERM_REQ, 0, 0);
			break;

		case CP_ACKRCVD:
			newstate(fs, CP_REQSENT);
			/* FALL THROUGH */
		case CP_REQSENT:
		case CP_ACKSENT:
			dprintf(("+ - send Conf-Req\n"));
			m = (*ft->ft_creq)(pp);
		}
		timeout(ft->ft_timeout, pp, pp->Ppp_ticks);

		/* Queue the request */
		if (IF_QFULL2(&pp->p2p_if.if_snd, &pp->p2p_isnd)) {
			IF_DROP(&pp->p2p_if.if_snd);
			m_freem(m);
		} else {
			pp->p2p_if.if_obytes += m->m_pkthdr.len;
			IF_ENQUEUE(&pp->p2p_isnd, m);
			if ((pp->p2p_if.if_flags & IFF_OACTIVE) == 0)
				(*pp->p2p_if.if_start)(&pp->p2p_if);
		}
	}
	splx(s);
}

/*
 * The Up event from lower level
 */
void
ppp_cp_up(pp, fsm)
	struct p2pcom *pp;
	int fsm;
{
	struct ppp_fsm *fs;
	struct mbuf *m;
	struct ppp_fsm_tab *ft = &ppp_fsm_tab[fsm];

	dprintf(("ppp_cp_up: %s\n", ft->ft_name));
	fs = (struct ppp_fsm *) ((char *)pp + ft->ft_offset);

	/* Initialize restart counter */
	fs->fsm_rc = pp->Ppp_maxconf;
	fs->fsm_rej = 0;

	/* Send out Config-Req */
	newstate(fs, CP_REQSENT);
	m = (*ft->ft_creq)(pp);
	timeout(ft->ft_timeout, pp, pp->Ppp_ticks);
	if (m == 0)
		return;

	/* Queue the packet to the high-priority queue */
	if (IF_QFULL2(&pp->p2p_if.if_snd, &pp->p2p_isnd)) {
		IF_DROP(&pp->p2p_if.if_snd);
		m_freem(m);
	} else {
		pp->p2p_if.if_obytes += m->m_pkthdr.len;
		IF_ENQUEUE(&pp->p2p_isnd, m);
		if ((pp->p2p_if.if_flags & IFF_OACTIVE) == 0)
			(*pp->p2p_if.if_start)(&pp->p2p_if);
	}
}

/*
 * The Down event from lower level
 */
void
ppp_cp_down(pp, fsm)
	struct p2pcom *pp;
	int fsm;
{
	struct ppp_fsm *fs;
	struct ppp_fsm_tab *ft = &ppp_fsm_tab[fsm];

	dprintf(("ppp_cp_down: %s\n", ft->ft_name));
	fs = (struct ppp_fsm *) ((char *)pp + ft->ft_offset);
	switch (fs->fsm_state) {
	case CP_CLOSING:
	case CP_CLOSED:
		newstate(fs, CP_INITIAL);
		break;

	case CP_OPENED:
		if (ft->ft_tld)
			(*ft->ft_tld)(pp);
		/* FALL THROUGH */
	default:
		newstate(fs, CP_STARTING);
	}

	/* Clear timeouts */
	untimeout(ft->ft_timeout, pp);
}

/*
 * Close the protocol
 */
void
ppp_cp_close(pp, fsm)
	struct p2pcom *pp;
	int fsm;
{
	struct ppp_fsm *fs;
	struct mbuf *m;
	struct ppp_fsm_tab *ft = &ppp_fsm_tab[fsm];

	dprintf(("ppp_cp_close: %s\n", ft->ft_name));
	fs = (struct ppp_fsm *) ((char *)pp + ft->ft_offset);
	switch (fs->fsm_state) {
	case CP_STARTING:
		newstate(fs, CP_INITIAL);
		break;

	case CP_STOPPED:
		newstate(fs, CP_CLOSED);
		break;

	case CP_OPENED:
		if (ft->ft_tld)
			(*ft->ft_tld)(pp);
		/*FALLTHROUGH*/

	case CP_REQSENT:
	case CP_ACKRCVD:
	case CP_ACKSENT:
		/* Initialize restart counter */
		fs->fsm_rc = pp->Ppp_maxterm;
		newstate(fs, CP_CLOSING);

		/* Set conservative txcmap */
		if (fsm == FSM_LCP)
			pp->Ppp_txcmap = 0xffffffff;

		/* Send terminate-request */
		dprintf(("Send Term-Req\n"));
		m = ppp_cp_prepend(0, ft->ft_type, CP_TERM_REQ, 0, 0);
		untimeout(ft->ft_timeout, pp);
		timeout(ft->ft_timeout, pp, pp->Ppp_ticks);

		/* Queue the packet to the normal priority queue */
		if (IF_QFULL2(&pp->p2p_if.if_snd, &pp->p2p_isnd)) {
			IF_DROP(&pp->p2p_if.if_snd);
			m_freem(m);
		} else {
			pp->p2p_if.if_obytes += m->m_pkthdr.len;
			IF_ENQUEUE(&pp->p2p_if.if_snd, m);
			if ((pp->p2p_if.if_flags & IFF_OACTIVE) == 0)
				(*pp->p2p_if.if_start)(&pp->p2p_if);
		}
		break;

	case CP_STOPPING:
		newstate(fs, CP_CLOSING);
		break;
	}
}
