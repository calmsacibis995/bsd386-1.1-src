/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_p2p.h,v 1.1 1993/02/21 00:13:03 karels Exp $
 */

/*
 * The point-to-point protocols' common data structure
 */

#include "if_c_hdlcvar.h"
#include "pppvar.h"

struct p2pcom {
	struct	ifnet p2p_if;		/* network-visible interface */
	int	p2p_oldflags;		/* the old flags */
	int	(*p2p_input) __P((struct p2pcom *, struct mbuf *));
	int	(*p2p_modem) __P((struct p2pcom *, int));  /* modem event */
	int	(*p2p_mdmctl) __P((struct p2pcom *, int)); /* modem control */
	struct	ifqueue p2p_isnd;	/* queue for interactive packets */

	struct	p2pcom *p2p_next;	/* next structure in protocol's list */
	struct	p2pcom **p2p_prevp;	/* prev. structure in protocol's list */
	/* XXX  The following should be separately allocated */
	union {
		struct	slarp_sc slarp;	/* SLARP data */
		struct	ppp_sc ppp;	/* PPP data */
	} p2p_u;
};

#define	p2p_slarp	p2p_u.slarp
#define	p2p_ppp		p2p_u.ppp


#define	HDLCMTU		1500		/* MTU for sync lines */
#define	HDLCMAX		(HDLCMTU + 8)	/* Max packet size (with link-level overhead) */

/* Check for limit of 2 queues */
#define IF_QFULL2(normal_q, fast_q) \
	((normal_q)->ifq_len + (fast_q)->ifq_len >= (normal_q)->ifq_maxlen)

#ifdef KERNEL
extern int  p2p_init __P((struct p2pcom *));
extern void p2p_shutdown __P((struct p2pcom *));
extern int  p2p_ioctl __P((struct ifnet *, int, caddr_t));
#endif
