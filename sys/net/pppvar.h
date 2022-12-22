/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: pppvar.h,v 1.2 1993/03/01 20:53:12 karels Exp $
 */

/*
 * The software state of a PPP link
 */

/*
 * Undefine PPP_DEBUG if no debugging code is needed;
 * define it as 0 if debugging should off on default,
 * or as 1 if debugging is default.
 */
/* #define PPP_DEBUG 0 */

#include "pppioctl.h"
#include "slcompress.h"

/*
 * PPP finite state machine definitions
 */
struct ppp_fsm {
	u_char  fsm_state;      /* Control protocol state */
	u_char  fsm_id;         /* Control protocol id */
	u_short fsm_rej;        /* Rejected options */
	u_short fsm_rc;         /* Restart counter */
};

struct ppp_sc {
	struct ppp_ioctl  ppp_dflt;     /* default link parameters */
	struct ppp_ioctl  ppp_params;   /* negotiated link parameters */
	u_long            ppp_txcmap;   /* active TX chars map */

	/* Timer parameters */
	u_short           ppp_ticks;    /* timeout in ticks */
	u_short           ppp_idlecnt;  /* idle time counter */

	/* LCP state */
	struct ppp_fsm    ppp_lcp;      /* line control protocol state */
	u_long            ppp_magic;    /* our magic number */
	u_char            ppp_badmagic; /* number of retries with magic */
	u_char            ppp_ncps;     /* the number of active NCPs */
	u_char            ppp_txflags;  /* TX modes accepted by peer */
	u_char            ppp_wdrop;    /* Someone's waiting for dropped */

	/* IPCP state */
	struct ppp_fsm    ppp_ipcp;     /* internet control protocol state */
	struct slcompress ppp_ctcp;     /* compressed tcp state */
	struct ifqueue    ppp_ipdefq;   /* queue for deferred outgoing IP */
};

/* FSM id numbers */
#define FSM_LCP         0
#define FSM_IPCP        1

/* Option parsing result */
#define OPT_OK          0
#define OPT_REJ         1
#define OPT_NAK         2
#define OPT_FATAL       3

/*
 * Shorthand macros
 */
#define Ppp_dflt        p2p_ppp.ppp_dflt
#define Ppp_params      p2p_ppp.ppp_params
#define Ppp_cmap        p2p_ppp.ppp_params.ppp_cmap
#define Ppp_mru         p2p_ppp.ppp_params.ppp_mru
#define Ppp_idletime    p2p_ppp.ppp_dflt.ppp_idletime
#define Ppp_flags       p2p_ppp.ppp_params.ppp_flags
#define Ppp_maxconf     p2p_ppp.ppp_dflt.ppp_maxconf
#define Ppp_maxterm     p2p_ppp.ppp_dflt.ppp_maxterm
#define Ppp_timeout     p2p_ppp.ppp_dflt.ppp_timeout
#define Ppp_txcmap      p2p_ppp.ppp_txcmap
#define Ppp_rxcmap      p2p_ppp.ppp_dflt.ppp_cmap

#define Ppp_ticks       p2p_ppp.ppp_ticks
#define Ppp_idlecnt     p2p_ppp.ppp_idlecnt

#define Ppp_lcp         p2p_ppp.ppp_lcp
#define Ppp_txflags     p2p_ppp.ppp_txflags
#define Ppp_ncps        p2p_ppp.ppp_ncps
#define Ppp_badmagic    p2p_ppp.ppp_badmagic
#define Ppp_magic       p2p_ppp.ppp_magic
#define Ppp_wdrop       p2p_ppp.ppp_wdrop

#define Ppp_ipcp        p2p_ppp.ppp_ipcp
#define Ppp_ctcp        p2p_ppp.ppp_ctcp
#define Ppp_ipdefq      p2p_ppp.ppp_ipdefq

/*
 * Function prototypes
 * should be defined only in ppp_*.c!
 */
#if defined(KERNEL) && defined(PPP_ADDRESS)
#ifdef __STDC__
struct p2pcom;
#endif

/* ppp_frame.c */
extern int ppp_output __P((struct ifnet *, struct mbuf *, struct sockaddr *, struct rtentry *));
extern int ppp_input __P((struct p2pcom *, struct mbuf *));
extern void ppp_idlepoll __P((void));

/* ppp_fsm.c */
extern struct mbuf *ppp_addopt __P((struct mbuf *, struct ppp_option *));
extern struct mbuf *ppp_cp_prepend __P((struct mbuf *, int, int, int, int));
extern void ppp_cp_in __P((struct p2pcom *, struct mbuf *, int));
extern void ppp_cp_timeout __P((struct p2pcom *, int));
extern void ppp_cp_up __P((struct p2pcom *, int));
extern void ppp_cp_down __P((struct p2pcom *, int));
extern void ppp_cp_close __P((struct p2pcom *, int));

/* ppp_lcp.c */
extern int ppp_ioctl __P((struct p2pcom *, int, caddr_t));
extern int ppp_init __P((struct p2pcom *));
extern void ppp_shutdown __P((struct p2pcom *));
extern void ppp_lcp_tlu __P((struct p2pcom *));
extern void ppp_lcp_tld __P((struct p2pcom *));
extern void ppp_lcp_timeout __P((struct p2pcom *));
extern struct mbuf *ppp_lcp_xcode __P((struct p2pcom *, struct mbuf *,
					struct ppp_cp_hdr *));
extern struct mbuf *ppp_lcp_creq __P((struct p2pcom *));
extern void ppp_lcp_optrej __P((struct p2pcom *, struct ppp_option *));
extern void ppp_lcp_optnak __P((struct p2pcom *, struct ppp_option *));
extern int ppp_lcp_opt __P((struct p2pcom *, struct ppp_option *));
extern void ppp_lcp_tlf __P((struct p2pcom *));
extern int ppp_modem __P((struct p2pcom *, int));
extern u_long ppp_lcp_magic __P((void));

/* ppp_ipcp.c */
extern void ppp_ipcp_timeout __P((struct p2pcom *));
extern void ppp_ipcp_tlf __P((struct p2pcom *));
extern void ppp_ipcp_tlu __P((struct p2pcom *));
extern struct mbuf *ppp_ipcp_creq __P((struct p2pcom *));
extern void ppp_ipcp_optrej __P((struct p2pcom *, struct ppp_option *));
extern void ppp_ipcp_optnak __P((struct p2pcom *, struct ppp_option *));
extern int ppp_ipcp_opt __P((struct p2pcom *, struct ppp_option *));

#ifdef PPP_DEBUG
#define dprintf(x)      if (pp->Ppp_dflt.ppp_flags & PPP_TRACE) \
				printf x
#else
#define dprintf(x)
#endif /* !PPP_DEBUG */

#endif /* KERNEL */
