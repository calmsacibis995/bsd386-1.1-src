/*-
 * Copyright (c) 1993, 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_appp.c,v 1.7 1994/02/01 17:51:38 karels Exp $
 */

/*
 * Asynchronous HDLC/PPP line discipline.
 */

#include "param.h"
#include "proc.h"
#include "mbuf.h"
#include "buf.h"
#include "socket.h"
#include "ioctl.h"
#include "file.h"
#include "tty.h"
#include "kernel.h"
#include "conf.h"
#include "termios.h"

#include "if.h"
#include "if_types.h"
#include "netisr.h"
#include "route.h"

#if INET
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/ip.h"		/* XXX for slcompress */
#endif

#include "if_p2p.h"

#include "machine/mtpr.h"

#ifdef PPP_DEBUG
#define dprintf(x)	if (sc->ap_p2pcom.Ppp_dflt.ppp_flags & PPP_TRACE) \
				printf x
#else
#define dprintf(x)
#endif /* !PPP_DEBUG */

#include "bpfilter.h"
#if NBPFILTER > 0
/* bpf additions by Martin Grossman, grossman@bbn.com */
#include <sys/time.h>
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

/*
 * Todo: add multiple lines per interface capability
 *	(it should be pretty easy...)
 */

#undef INLINE_PUTC	/* use inline version of putc */

#define t_sc T_LINEP

extern struct timeval time;

struct ap_softc {
	struct	p2pcom ap_p2pcom;	/* p2p common structure */
#define	ap_if	ap_p2pcom.p2p_if	/* the interface structure */
	struct	tty *ap_ttyp;		/* pointer to the tty structure */
	u_short	ap_txfcs;		/* current tx checksum */
	u_short	ap_rxfcs;		/* current rx checksum */
	struct	mbuf *ap_obuf;		/* current tx mbuf */
	struct	mbuf *ap_ibuf;		/* current rx buffer */
	struct	mbuf *ap_ipack;		/* the head of rx buffer queue */
	struct	timeval ap_lastpacket;	/* time when the last byte was sent */
	u_short	ap_ilen;		/* the number of input bytes */
	u_short	ap_space;	/* bytes left in the current input buffer */
	u_short	ap_weos;	/* someone's waiting for end of session */
	u_short	ap_istate;	/* state of an input automaton */
#if NBPFILTER > 0
	caddr_t ap_bpf;
#endif
} *ap_softc;

int napppif;				/* number of interfaces */

/*
 * States of the input automaton
 */
#define API_SKIP	0		/* skipping until flag */
#define API_OK		1		/* waiting for data */
#define API_ESC		2		/* got an escape */

/*
 * The high water mark for the output queue --
 * should not be large to avoid spending long time
 * under spltty but large enough to allow effective usage of
 * hardware transmitter buffers.
 */
#define AP_HIWAT	64

/* Max idle time for flag saver */
#define AP_FLAGTIMO	100000		/* 0.1 sec */

/* Escape & flag characters */
#define AP_ESC		0x7d
#define AP_FLAG		0x7e
#define AP_QUOTE	0x20

/*
 * Checksum definitions
 */
#define AP_FCSINIT	0xffff
#define AP_FCSGOOD	0xf0b8
#define AP_FCS(fcs, c)  (((fcs)>>8) ^ fcstab[((fcs) ^ (c)) & 0xff])

/*
 * FCS lookup table as calculated by genfcstab.
 */
static u_short fcstab[256] = {
	0x0000,	0x1189,	0x2312,	0x329b,	0x4624,	0x57ad,	0x6536,	0x74bf,
	0x8c48,	0x9dc1,	0xaf5a,	0xbed3,	0xca6c,	0xdbe5,	0xe97e,	0xf8f7,
	0x1081,	0x0108,	0x3393,	0x221a,	0x56a5,	0x472c,	0x75b7,	0x643e,
	0x9cc9,	0x8d40,	0xbfdb,	0xae52,	0xdaed,	0xcb64,	0xf9ff,	0xe876,
	0x2102,	0x308b,	0x0210,	0x1399,	0x6726,	0x76af,	0x4434,	0x55bd,
	0xad4a,	0xbcc3,	0x8e58,	0x9fd1,	0xeb6e,	0xfae7,	0xc87c,	0xd9f5,
	0x3183,	0x200a,	0x1291,	0x0318,	0x77a7,	0x662e,	0x54b5,	0x453c,
	0xbdcb,	0xac42,	0x9ed9,	0x8f50,	0xfbef,	0xea66,	0xd8fd,	0xc974,
	0x4204,	0x538d,	0x6116,	0x709f,	0x0420,	0x15a9,	0x2732,	0x36bb,
	0xce4c,	0xdfc5,	0xed5e,	0xfcd7,	0x8868,	0x99e1,	0xab7a,	0xbaf3,
	0x5285,	0x430c,	0x7197,	0x601e,	0x14a1,	0x0528,	0x37b3,	0x263a,
	0xdecd,	0xcf44,	0xfddf,	0xec56,	0x98e9,	0x8960,	0xbbfb,	0xaa72,
	0x6306,	0x728f,	0x4014,	0x519d,	0x2522,	0x34ab,	0x0630,	0x17b9,
	0xef4e,	0xfec7,	0xcc5c,	0xddd5,	0xa96a,	0xb8e3,	0x8a78,	0x9bf1,
	0x7387,	0x620e,	0x5095,	0x411c,	0x35a3,	0x242a,	0x16b1,	0x0738,
	0xffcf,	0xee46,	0xdcdd,	0xcd54,	0xb9eb,	0xa862,	0x9af9,	0x8b70,
	0x8408,	0x9581,	0xa71a,	0xb693,	0xc22c,	0xd3a5,	0xe13e,	0xf0b7,
	0x0840,	0x19c9,	0x2b52,	0x3adb,	0x4e64,	0x5fed,	0x6d76,	0x7cff,
	0x9489,	0x8500,	0xb79b,	0xa612,	0xd2ad,	0xc324,	0xf1bf,	0xe036,
	0x18c1,	0x0948,	0x3bd3,	0x2a5a,	0x5ee5,	0x4f6c,	0x7df7,	0x6c7e,
	0xa50a,	0xb483,	0x8618,	0x9791,	0xe32e,	0xf2a7,	0xc03c,	0xd1b5,
	0x2942,	0x38cb,	0x0a50,	0x1bd9,	0x6f66,	0x7eef,	0x4c74,	0x5dfd,
	0xb58b,	0xa402,	0x9699,	0x8710,	0xf3af,	0xe226,	0xd0bd,	0xc134,
	0x39c3,	0x284a,	0x1ad1,	0x0b58,	0x7fe7,	0x6e6e,	0x5cf5,	0x4d7c,
	0xc60c,	0xd785,	0xe51e,	0xf497,	0x8028,	0x91a1,	0xa33a,	0xb2b3,
	0x4a44,	0x5bcd,	0x6956,	0x78df,	0x0c60,	0x1de9,	0x2f72,	0x3efb,
	0xd68d,	0xc704,	0xf59f,	0xe416,	0x90a9,	0x8120,	0xb3bb,	0xa232,
	0x5ac5,	0x4b4c,	0x79d7,	0x685e,	0x1ce1,	0x0d68,	0x3ff3,	0x2e7a,
	0xe70e,	0xf687,	0xc41c,	0xd595,	0xa12a,	0xb0a3,	0x8238,	0x93b1,
	0x6b46,	0x7acf,	0x4854,	0x59dd,	0x2d62,	0x3ceb,	0x0e70,	0x1ff9,
	0xf78f,	0xe606,	0xd49d,	0xc514,	0xb1ab,	0xa022,	0x92b9,	0x8330,
	0x7bc7,	0x6a4e,	0x58d5,	0x495c,	0x3de3,	0x2c6a,	0x1ef1,	0x0f78
};

/* Prototypes */
void apppattach __P((int));
int  apppopen __P((dev_t, struct tty *));
int  apppclose __P((struct tty *));
int  apppioctl __P((struct tty *, int, caddr_t, int));
int  apppifmdmctl __P((struct p2pcom *, int));
int  apppifioctl __P((struct ifnet *, int, caddr_t));
int  apppstart __P((struct tty *));
int  apppifstart __P((struct ifnet *));
int  apppinput __P((int, struct tty *));
int  apppmodem __P((struct tty *, int));
int  apppwatchdog __P((int));

/*
 * Called from boot code to establish asynchronous ppp interfaces.
 */
void
apppattach(n)
	int n;
{
	register struct ap_softc *sc;
	register int i;

	napppif = n;
	ap_softc = sc = malloc(n * sizeof *ap_softc, M_DEVBUF, M_WAIT);
	for (i = 0; i < napppif; sc++, i++) {
		sc->ap_if.if_name = "ppp";
		sc->ap_if.if_unit = i;
		sc->ap_if.if_mtu = HDLCMTU;
		sc->ap_if.if_flags = IFF_POINTOPOINT | IFF_MULTICAST |
		    IFF_LINK0; /* hardwire PPP */
		sc->ap_if.if_type = IFT_PPP;
		sc->ap_if.if_ioctl = apppifioctl;
		sc->ap_if.if_start = apppifstart;
		sc->ap_if.if_watchdog = apppwatchdog;
		sc->ap_if.if_snd.ifq_maxlen = 32;
		sc->ap_p2pcom.p2p_isnd.ifq_maxlen = 32;
		sc->ap_p2pcom.p2p_mdmctl = apppifmdmctl;
		sc->ap_weos = 0;
		if_attach(&sc->ap_if);
#if NBPFILTER > 0
		bpfattach(&sc->ap_bpf, &sc->ap_if, DLT_PPP, 0);
#endif
	}
}

/*
 * Line specific open routine.
 * Attach the given tty to the first available appp unit.
 */
/* ARGSUSED */
int
apppopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	register struct ap_softc *sc;
	register int np;

	if (tp->t_line == PPPDISC)
		return (0);

	tp->t_sc = 0;

	ttyflush(tp, FREAD | FWRITE);
	return (0);
}

/*
 * Line specific close routine.
 * Detach the tty from the sl unit.
 * Mimics part of ttyclose().
 */
int
apppclose(tp)
	struct tty *tp;
{
	register struct ap_softc *sc;
	int s;
	int line;

	ttyflush(tp, FREAD|FWRITE);
	s = spltty();		/* actually, max(spltty, splnet) */
	tp->t_line = 0;
	sc = (struct ap_softc *)tp->t_sc;

	if (sc) {
		/* Signal the interface up-->down transition to upper levels */
		sc->ap_if.if_timer = 0;
		if (sc->ap_p2pcom.p2p_modem)
			(*sc->ap_p2pcom.p2p_modem)(&sc->ap_p2pcom, 0);

		/* Final cleanup */
		if (sc->ap_ipack)
			m_freem(sc->ap_ipack);
		sc->ap_ipack = 0;
		if (sc->ap_obuf)
			m_freem(sc->ap_obuf);
		sc->ap_obuf = 0;
		sc->ap_if.if_flags &= ~IFF_OACTIVE;
		ttyflush(tp, FREAD|FWRITE);

		/* Detach the interface */
		sc->ap_ttyp = 0;
		tp->t_sc = 0;
		tty_net_devices(-1);	/* subtract one tty-net device */
	}
	splx(s);
	return (0);
}

/*
 * Line specific (tty) ioctl routine.
 * Provide a way to get the async ppp unit number.
 */
/* ARGSUSED */
int
apppioctl(tp, cmd, data, flag)
	struct tty *tp;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct ap_softc *sc;
	int s, error;
	int unit;

	error = 0;
	s = spltty();
	sc = (struct ap_softc *) tp->t_sc;
	switch (cmd) {
	case PPPIOCGUNIT:
		if (sc == 0) {
			*(int *)data = -1;
			break;
		}
		*(int *)data = sc->ap_if.if_unit;
		break;

	case PPPIOCSUNIT:
		unit = *(int *)data;
		if (unit == -1) {
			for (unit = 0; unit < napppif; unit++) {
				if (ap_softc[unit].ap_ttyp == 0)
					break;
			}
		}
		if (unit < 0 || unit >= napppif) {
			error = ENXIO;
			break;
		}
		if (sc != 0) {
			if (unit == sc->ap_if.if_unit)
				break;

			/* Flush working mbufs */
			if (sc->ap_ipack)
				m_freem(sc->ap_ipack);
			if (sc->ap_obuf)
				m_freem(sc->ap_obuf);

			/* Signal the interface up-->down transition to upper levels */
			if (sc->ap_p2pcom.p2p_modem)
				(*sc->ap_p2pcom.p2p_modem)(&sc->ap_p2pcom, 0);
		}
		sc = &ap_softc[unit];
		if (sc->ap_ttyp != 0) {
			error = EBUSY;
			break;
		}

		tp->t_sc = (caddr_t) sc;
		sc->ap_ttyp = tp;
		sc->ap_if.if_baudrate = tp->t_ospeed;
		sc->ap_txfcs = AP_FCSINIT;
		sc->ap_istate = API_SKIP;
		sc->ap_space = 0;
		sc->ap_ipack = 0;
		sc->ap_ibuf = sc->ap_obuf = 0;
		sc->ap_ilen = 0;
		tty_net_devices(1);	/* add one tty-net device */

		/* Signal the interface down-->up transition to upper levels */
		if (sc->ap_p2pcom.p2p_modem &&
		    (tp->t_cflag & CLOCAL || tp->t_state & TS_CARR_ON))
			(*sc->ap_p2pcom.p2p_modem)(&sc->ap_p2pcom, 1);
		break;

	case PPPIOCWEOS:
		dprintf(("PPPIOCWEOS %x\n", &sc->ap_weos));
		sc->ap_weos = 1;
		error = tsleep((caddr_t) &sc->ap_weos,
			PCATCH|(PZERO+10), "ppp_weos", 0);
		dprintf(("END OF WAITING\n"));
		break;

	default:
		error = -1;
		break;
	}
	splx(s);
	return (error);
}

/*
 * The interface "modem control" routine
 */
int
apppifmdmctl(pp, flag)
	struct p2pcom *pp;
	int flag;
{
	struct ap_softc *sc = (struct ap_softc *) pp;
	struct tty *tp = sc->ap_ttyp;
	int s;

	if (tp == 0)
		return (0);

	/*
	 * If we've got a direct line or carrier is already on,
	 * signal about it.
	 */
	if (flag) {
		if (sc->ap_p2pcom.p2p_modem &&
		    (tp->t_cflag & CLOCAL || tp->t_state & TS_CARR_ON))
			(*sc->ap_p2pcom.p2p_modem)(&sc->ap_p2pcom, 1);
		return (0);
	} else {
		/* Dropping carrier... signal the user process! */
		s = spltty();
		if (sc->ap_weos) {
			dprintf(("WEOS!!! %x\n", &sc->ap_weos));
			wakeup((caddr_t) &sc->ap_weos);
			sc->ap_weos = 0;
		}
		splx(s);
	}
	return (0);
}

/*
 * Process a packet interface ioctl request.
 */
int
apppifioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{

	ifp->if_flags |= IFF_LINK0;	/* enforce PPP! */
	return (p2p_ioctl(ifp, cmd, data));
}

/*
 * The inline version of putc
 */
#ifdef INLINE_PUTC
#define PUTC(c, q) { \
	if ((q).c_cl == NULL || (q).c_cc == 0) { \
		(q).c_cl = (q).c_cf = (q).c_cq; \
		(q).c_cc = 0; \
	} else if (++(q).c_cl >= (q).c_ce) \
		(q).c_cl = (q).c_ce; \
	*((q).c_cl) = (c); \
	(q).c_cc++; \
}
#else /* !INLINE_PUTC */
#define PUTC(c, q)	(void) putc((c), &(q));
#endif /* !INLINE_PUTC */

/*
 * Quote and queue output character
 */
#define ESC_PUTC(c, q, m)	if (((c) < 040 && ((1<<(c)) & (m))) || \
				    (c) == AP_FLAG || (c) == AP_ESC) { \
					(c) ^= AP_QUOTE; \
					PUTC(AP_ESC, q); \
				} \
				PUTC((c), q)

/*
 * Start output on a serial line
 */
int
apppstart(tp)
	register struct tty *tp;
{
	register struct ap_softc *sc = (struct ap_softc *) tp->t_sc;
	register struct mbuf *m;
	int s, s1;
	int n;
	register u_char c;
	u_long t;

	/*
	 * Let the stream to flow -- call oproc ASAP
	 */
	s = spltty();
	if (tp->t_outq.c_cc > 0)
		(*tp->t_oproc)(tp);

	/*
	 * No interface attached -- get out
	 */
	if (sc == 0) {
		splx(s);
		return (0);
	}

	/*
	 * Get a packet and send it to the interface.
	 */
	for (;;) {
		if (m = sc->ap_obuf) {
			do {
				n = AP_HIWAT - tp->t_outq.c_cc;
				if (n <= 0)
					goto out;
				if (n >= m->m_len) {
					n = m->m_len;
					sc->ap_obuf = m->m_next;
				}
				m->m_len -= n;
				while (n-- > 0) {
					c = *(m->m_data++);
					sc->ap_txfcs = AP_FCS(sc->ap_txfcs, c);
					ESC_PUTC(c, tp->t_outq, sc->ap_p2pcom.Ppp_txcmap);
				}
				if (m->m_len == 0)
					m_free(m);
			} while (m = sc->ap_obuf);

			/*
			 * If we're at the end of a packet, send a checksum
			 * and terminating flag.
			 * That yields garabge characters at the beginning
			 * of connection but who cares.
			 */
			c = ~(sc->ap_txfcs);
			ESC_PUTC(c, tp->t_outq, sc->ap_p2pcom.Ppp_txcmap);
			c = ~(sc->ap_txfcs) >> 8;
			ESC_PUTC(c, tp->t_outq, sc->ap_p2pcom.Ppp_txcmap);
			PUTC(AP_FLAG, tp->t_outq);
		}

		/* Get the following packet */
		s1 = splimp();
		IF_DEQUEUE(&sc->ap_p2pcom.p2p_isnd, m);
		if (m == 0)
			IF_DEQUEUE(&sc->ap_if.if_snd, m);
		if (m == 0) {
			sc->ap_if.if_flags &= ~IFF_OACTIVE;
			splx(s1);
			break;
		}
		splx(s1);
		sc->ap_if.if_flags |= IFF_OACTIVE;
		sc->ap_if.if_opackets++;
		sc->ap_obuf = m;
#if NBPFILTER > 0
		if(sc->ap_bpf)
			bpf_mtap(sc->ap_bpf, m);
#endif
		sc->ap_txfcs = AP_FCSINIT;

		/*
		 * If there was a long period since the last packet's end,
		 * send a flag.
		 */
		if (tp->t_outq.c_cc == 0) {
			t = time.tv_sec - sc->ap_lastpacket.tv_sec;
			if (t <= 1) {
				if (t == 1)
					t = 1000000;
				else
					t = 0;
				t += time.tv_usec - sc->ap_lastpacket.tv_usec;
				if (t < AP_FLAGTIMO)
					goto noflag;
			}
			PUTC(AP_FLAG, tp->t_outq);
		noflag: ;
		}
	}

out:	if (tp->t_outq.c_cc > 0) {
		sc->ap_if.if_timer = 2;
		(*tp->t_oproc)(tp);
	} else {
		sc->ap_if.if_timer = 0;
		sc->ap_lastpacket = time;
	}
	splx(s);
	return (0);
}

/*
 * Start output on an interface
 */
int
apppifstart(ifp)
	struct ifnet *ifp;
{
	register struct ap_softc *sc = (struct ap_softc *) ifp;
	register struct mbuf *m;

	if (sc->ap_ttyp)
		apppstart(sc->ap_ttyp);
	return (0);
}

/*
 * Interface watchdog
 */
int
apppwatchdog(unit)
	int unit;
{
	register struct ap_softc *sc = &ap_softc[unit];

	printf("ppp%d: intr lost\n", unit);
	if (sc->ap_ttyp)
		apppstart(sc->ap_ttyp);
	return (0);
}

/*
 * Tty interface receiver interrupt.
 */
int
apppinput(c, tp)
	register int c;
	register struct tty *tp;
{
	register struct ap_softc *sc = (struct ap_softc *) tp->t_sc;
	register struct mbuf *m;
	struct mbuf *m0;
	int s;
	extern struct ttytotals ttytotals;

	ttytotals.tty_nin++;
	if (sc == 0 || (sc->ap_if.if_flags & (IFF_UP|IFF_RUNNING)) == 0)
		return (0);
	if (!(tp->t_state & TS_CARR_ON))
		return (0);

	/* Process the flag character */
	if (c == AP_FLAG) {
		/* If we've got an input packet, check the FCS */
		m0 = sc->ap_ipack;
		sc->ap_ipack = 0;
		if (m0 != 0) {
			if (sc->ap_rxfcs != AP_FCSGOOD ||
			    sc->ap_istate != API_OK ||
			    sc->ap_ilen < 3) {
				dprintf(("F"));
				m_freem(m0);
				sc->ap_if.if_ierrors++;
			} else {
				/* Get rid of the checksum */
				m = sc->ap_ibuf;
				if (m->m_len > 2)
					m->m_len -= 2;
				else {
					/* Find the previous mbuf */
					c = 2 - m->m_len;
					m_free(m);
					for (m = m0; m->m_next != sc->ap_ibuf;
						m = m->m_next);
					m->m_len -= c;
					m->m_next = 0;
				}

				/* Push the packet upwards */
				m0->m_pkthdr.rcvif = &sc->ap_if;
				m0->m_pkthdr.len = sc->ap_ilen - 2;
				sc->ap_if.if_ipackets++;
#if NBPFILTER > 0
				if(sc->ap_bpf)
					bpf_mtap(sc->ap_bpf, m0);
#endif
				(*sc->ap_p2pcom.p2p_input)(&sc->ap_p2pcom, m0);
			}
		}
		sc->ap_rxfcs = AP_FCSINIT;
		sc->ap_istate = API_OK;
		sc->ap_ilen = 0;
		sc->ap_space = 0;
		return (0);
	}

	/* Error recovery -- discard everything until the first flag */
	if (sc->ap_istate == API_SKIP)
		return (0);

	/* Process escape character */
	if (c == AP_ESC) {
		if (sc->ap_istate == API_OK)
			sc->ap_istate = API_ESC;
		else
			sc->ap_istate = API_SKIP;
		return (0);
	}

	/* Discard garbage characters */
	if (c < 040 && ((1 << (c)) & sc->ap_p2pcom.Ppp_rxcmap)) {

		/* Oh, no, wait a second... Check for XON/XOF */
		if (tp->t_iflag & IXON) {
			if (CCEQ(tp->t_cc[VSTOP], c)) {
				if (tp->t_state & TS_TTSTOP)
					return (0);
				tp->t_state |= TS_TTSTOP;
				(*cdevsw[major(tp->t_dev)].d_stop)(tp, 0);
			} else if (CCEQ(tp->t_cc[VSTART], c)) {
				if (!(tp->t_state & TS_TTSTOP))
					return (0);
				tp->t_state &= ~TS_TTSTOP;
				apppstart(tp);
			}
		}
		return (0);
	}

	/* Second character of an escape sequence */
	if (sc->ap_istate == API_ESC) {
		c ^= AP_QUOTE;
		sc->ap_istate = API_OK;
	}

	/* Good. One step forward in computing FCS */
	sc->ap_rxfcs = AP_FCS(sc->ap_rxfcs, c);
	sc->ap_ilen++;

	/*
	 * If there is no input buffer or no space in the current buffer
	 * allocate the buffer.
	 */
	if (sc->ap_space == 0) {
		if (sc->ap_ipack == 0) {
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {	/* Drop -- no space available */
				dprintf(("MGETHDR -- no space\n"));
				sc->ap_istate = API_SKIP;
				return (0);
			}
			m->m_len = 0;
			sc->ap_ipack = sc->ap_ibuf = m;
			sc->ap_space = MHLEN;
		} else {
			/* We've got a long packet... try to allocate cluster */
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {	/* No space available */
				dprintf(("MGET -- no space\n"));
				m_freem(sc->ap_ipack);
				sc->ap_ipack = 0;
				sc->ap_istate = API_SKIP;
				return (0);
			}
			MCLGET(m, M_DONTWAIT);
			m->m_len = 0;
			sc->ap_ibuf->m_next = m;
			sc->ap_ibuf = m;
			sc->ap_space = (m->m_flags & M_EXT) ? MCLBYTES : MLEN;
		}
	} else
		m = sc->ap_ibuf;
	m->m_data[m->m_len++] = c;
	sc->ap_space--;
	return (0);
}

/*
 * Handle modem control transition on a tty.
 */
int
apppmodem(tp, flag)
	register struct tty *tp;
	int flag;
{
	register struct ap_softc *sc = (struct ap_softc *) tp->t_sc;
	int s;

	dprintf(("apppmodem(%d) sc=%x\n", flag, sc));
	if (flag) {
		/* Signal the interface down-->up transition to upper levels */
		if (sc && sc->ap_p2pcom.p2p_modem &&
		    !(tp->t_state & TS_CARR_ON))
			(*sc->ap_p2pcom.p2p_modem)(&sc->ap_p2pcom, 1);

		tp->t_state |= TS_CARR_ON;

	} else if (!(tp->t_cflag & CLOCAL) && (tp->t_state & TS_CARR_ON)) {
		if (sc) {
			/*
			 * Signal the interface up-->down transition
			 * to upper levels
			 */
			sc->ap_if.if_timer = 0;
			if (sc->ap_p2pcom.p2p_modem)
				(*sc->ap_p2pcom.p2p_modem)(&sc->ap_p2pcom, 0);

			s = spltty();
			if (sc && sc->ap_weos) {
				dprintf(("WEOS!!! %x\n", &sc->ap_weos));
				wakeup((caddr_t) &sc->ap_weos);
				sc->ap_weos = 0;
			}
			splx(s);
		}
		tp->t_state &= ~TS_CARR_ON;
		return (0);
	}
	return (1);
}
