/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_p2psubr.c,v 1.2 1993/12/19 04:12:03 karels Exp $
 */

/*
 * Common code for point-to-point synchronous serial links
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
#include "if_dl.h"
#include "if_types.h"

#if INET
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"		/* XXX for slcompress */
#endif

#include "if_p2p.h"


/*
 * Get link-level protocol # from ifnet.
 * Currently it is IFF_LINKn flags (actually only LINK0).
 * It will be changed later.
 */
#define	P2P_PROTO_TYPE(pp)	(((pp)->p2p_if.if_flags & IFF_LINK0) ? 1 : 0)


#ifdef CISCO_HDLC
extern int c_hdlc_init(), c_hdlc_output(), c_hdlc_input();
extern void c_hdlc_shutdown();
#endif
#ifdef PPP
extern int ppp_init(), ppp_output(), ppp_input(), ppp_ioctl(), ppp_modem();
extern void ppp_shutdown();
#endif

/*
 * The table of available point-to-point encapsulation protocols
 */
struct p2pprotosw {
	int	(*pp_init)();		/* protocol initialization routine */
	void	(*pp_shutdown)();	/* protocol shutdown routine */
	int	(*pp_output)();		/* output encapsulation routine */
	int	(*pp_input)();		/* the input parsing routine */
	int	(*pp_ioctl)();		/* protocol-specific ioctl */
	int	(*pp_modem)();		/* modem event */
	int	pp_type;		/* interface type for ifnet */
} p2pprotosw[] = {
	/* CISCO HDLC */
#ifdef CISCO_HDLC
{	c_hdlc_init,	c_hdlc_shutdown, c_hdlc_output,
	c_hdlc_input,	0,		0,		IFT_PTPSERIAL,
},
#else
{	0,		0,		0,
	0,		0,		0,		0,
},
#endif

	/* PPP */
#ifdef PPP
{	ppp_init,	ppp_shutdown,	ppp_output,
	ppp_input,	ppp_ioctl,	ppp_modem,	IFT_PPP,
},
#else
{	0,		0,		0,
	0,		0,		0,		0,
},
#endif
};

#define NP2PPROT (sizeof p2pprotosw / sizeof p2pprotosw[0])

/*
 * Point-to-point interface initialization.
 * Checks the protocol number and calls the
 * protocol initialization routine.
 */
int
p2p_init(pp)
	struct p2pcom *pp;
{
	register struct ifnet *ifp = &pp->p2p_if;
	short type;

	type = P2P_PROTO_TYPE(pp);
	if (type >= NP2PPROT || p2pprotosw[type].pp_init == 0)
		return (EPFNOSUPPORT);
	if (ifp->if_output != p2pprotosw[type].pp_output) {
		bzero((caddr_t) &pp->p2p_u, sizeof pp->p2p_u);
		ifp->if_output = p2pprotosw[type].pp_output;
		pp->p2p_input = p2pprotosw[type].pp_input;
		pp->p2p_modem = p2pprotosw[type].pp_modem;
		ifp->if_type = p2pprotosw[type].pp_type;
	}
	return ((*p2pprotosw[type].pp_init)(pp));
}

/*
 * Point-to-point interface shutdown.
 * Checks the protocol number and calls the
 * protocol initialization routine.
 */
void
p2p_shutdown(pp)
	struct p2pcom *pp;
{
	short type;

	type = P2P_PROTO_TYPE(pp);
	if (type >= NP2PPROT)
		panic("p2p_shutdown");
	(*p2pprotosw[type].pp_shutdown)(pp);
}

/*
 * Handle protocol-specific ioctl
 */
int
p2p_ioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	struct p2pcom *pp = (struct p2pcom *) ifp;
	struct ifreq *ifr;
	short type, otype;
	int error;
	int s;

	/*
	 * Call the protocol-specific ioctl handler
	 */
	type = P2P_PROTO_TYPE(pp);
	if (type >= NP2PPROT)
		panic("p2p_ioctl");
	s = splimp();
	if (p2pprotosw[type].pp_ioctl) {
		error = (*p2pprotosw[type].pp_ioctl)(pp, cmd, data);
		if (error != EINVAL)
			goto out;
	}

	/*
	 * Process the common ioctls
	 */
	error = 0;
	switch (cmd) {
	default:
		error = EINVAL;
		goto out;

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		break;

	case SIOCSIFDSTADDR:
	case SIOCSIFFLAGS:
		break;

	/*
	 * These should probably be done at a lower level,
	 * but this should be ok for now.
	 */
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		ifr = (struct ifreq *)data;
		if (ifr == 0) {
			error = EAFNOSUPPORT;		/* XXX */
			break;
		}
		switch (ifr->ifr_addr.sa_family) {
#ifdef INET
		case AF_INET:
			break;
#endif
		default:
			error = EAFNOSUPPORT;
			break;
		}
		break;
	}

	/*
	 * IFF_UP has been changed -- initialize/shutdown
	 * the link-level protocol.
	 */
	if ((pp->p2p_oldflags ^ ifp->if_flags) & IFF_UP) {
		if (ifp->if_flags & IFF_UP) {
			if (error = p2p_init(pp))
				goto out;
		} else
			p2p_shutdown(pp);
	}
	pp->p2p_oldflags = ifp->if_flags;

	/*
	 * Switching the protocol "on the fly"
	 * (shouldn't normally happen...)
	 */
	if ((ifp->if_flags & IFF_UP) && ifp->if_output &&
	    p2pprotosw[type].pp_output != ifp->if_output) {
		for (otype = 0; otype < NP2PPROT; otype++)
			if (p2pprotosw[otype].pp_output == ifp->if_output)
				break;
		(*p2pprotosw[otype].pp_shutdown)(pp);
		if (error = p2p_init(pp))
			goto out;
	}
out:
	splx(s);
	return (error);
}
