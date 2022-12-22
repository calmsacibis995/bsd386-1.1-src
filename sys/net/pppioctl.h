/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: pppioctl.h,v 1.1 1993/02/21 00:08:19 karels Exp $
 */

/*
 * Point-to-point protocol ioctls
 * (net/if.h should be included first)
 */

/* PPP parameters for ioctls */
struct ppp_ioctl {
	u_long  ppp_cmap;       /* The map of "bad" control characters */
	u_short ppp_mru;        /* Suggested MRU on the line (not exceeding 1500) */
	u_short ppp_idletime;   /* The inactivity timer (in seconds) */
	u_char  ppp_flags;      /* PPP option flags */
	u_char  ppp_maxconf;    /* max config retries */
	u_char  ppp_maxterm;    /* max term retries */
	u_char  ppp_timeout;    /* The restart timer period (1/10 sec) */
};

/* A kludge to fit the ppp_ioctl to the struct ifreq */
#define ifr_pppioctl(ifr)    ((struct ppp_ioctl *)&(((struct ifreq *)(ifr))->ifr_data))

/* PPP option flags */
#define PPP_PFC         0x1     /* Enable protocol field compression */
#define PPP_ACFC        0x2     /* Enable address and control field compression */
#define PPP_TCPC        0x4     /* Enable Van Jacobson's TCP compression */
#define PPP_FTEL        0x8     /* Enable "fast telnet" hack */
#define PPP_TRACE       0x80    /* Enable protocol tracing output */

/* IOCTLs for ppp interfaces */
#define PPPIOCGPAR _IOWR('i', 100, struct ifreq) /* Get current interface PPP params */
#define PPPIOCSPAR _IOWR('i', 101, struct ifreq) /* Set interface PPP params */
#define PPPIOCNPAR _IOWR('i', 102, struct ifreq) /* Get negotiated interface PPP params */

#define PPPIOCWAIT _IOWR('i', 103, struct ifreq) /* Wait until outgoing packet comes */
						 /* to an interface with dropped connection */

/* Async PPP tty ioctls */
#define PPPIOCGUNIT _IOW('P', 100, int)  /* Get interface # for the line */
#define PPPIOCSUNIT _IOWR('P', 101, int) /* Set interface # for the line */
#define PPPIOCWEOS  _IO('P', 102)        /* Wait for the end of session */
