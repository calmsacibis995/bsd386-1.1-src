/*-
 * Copyright (c) 1992, 1993, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: if_ex.c,v 1.5 1994/01/30 04:19:47 karels Exp $
 */

/*
 * Intel EtherExpress 16 Ethernet Adapter Driver
 *
 * This code is derived from the Intel EtherExpress driver
 * donated to BSDI by Korotin D.O. (mitia@pczz.msk.su).
 */
#include "bpfilter.h"

#include "param.h"
#include "systm.h"
#include "mbuf.h"
#include "buf.h"
#include "protosw.h"
#include "socket.h"
#include "ioctl.h"
#include "errno.h"
#include "syslog.h"
#include "device.h"

#include "net/if.h"
#include "net/netisr.h"

#ifdef INET
#include "netinet/in.h"
#include "netinet/in_systm.h"
#include "netinet/in_var.h"
#include "netinet/ip.h"
#include "netinet/if_ether.h"
#endif

#ifdef NS
#include "netns/ns.h"
#include "netns/ns_if.h"
#endif

#include "isa.h"
#include "isavar.h"
#include "icu.h"
#include "machine/cpu.h"

#if NBPFILTER > 0
#include "../net/bpf.h"
#include "../net/bpfdesc.h"
#endif

#include "if_exreg.h"

#define ETHER_MIN_LEN 64
#define ETHER_MAX_LEN 1536		/* XXX */

enum conn_type { UNK, AUI, BNC, TP };

static char *conn_name[] = { "unknown", "AUI", "BNC", "TP" };

struct ex_softc {
	struct  device ex_dev;          /* base device (must be first) */
	struct  isadev ex_id;           /* ISA device */
	struct  intrhand ex_ih;         /* interrupt vectoring */
	struct  arpcom ex_ac;           /* Ethernet common part */
#define ex_if   ex_ac.ac_if             /* network-visible interface */
#define ex_addr ex_ac.ac_enaddr         /* hardware Ethernet address */

	int     ex_oldflags;            /* old interface flags */
	u_short ex_base;                /* base i/o port */
	u_short ex_irq;                 /* irq */
	u_short ex_begin_rbd;
	u_short ex_begin_fd;
	u_short ex_end_rbd;
	u_short ex_end_fd;

	int	ex_board_type;		/* board ID */
	enum	conn_type ex_conntype;	/* connector type */

	char    ex_data[ETHER_MAX_LEN+4];

#if NBPFILTER > 0
	caddr_t ex_bpf;                 /* bpf filter */
#endif
};

/*
 * Prototypes
 */
int  exprobe __P((struct device *, struct cfdata *, void *));
void exattach __P((struct device *, struct device *, void *));
int  exinit __P((int));
int  exstart __P((struct ifnet *));
int  exioctl __P((struct ifnet *, int, caddr_t));
int  exintr __P((void *));

static u_short	ex_id __P((int, int));
static u_short ex_eprom_r __P((int, int));
static void    ex_eprom_so __P((int, int, int));
static u_short ex_eprom_si __P((int));
static void    ex_eprom_RC __P((int, u_char *));
static void    ex_eprom_LC __P((int, u_char *));
static void    ex_eprom_c __P((int));
static int     ex_wait_scb __P((int));
static int     ex_ack __P((int));
static int     ex_config __P((struct ex_softc *));
static void    ex_init_rx __P((struct ex_softc *));
static void    ex_rcv __P((struct ex_softc *));
static void    ex_reqfd __P((struct ex_softc *, unsigned));
static void    ex_readdata __P((struct ex_softc *, unsigned));
static void    exread __P((char *, int, struct ether_header *, struct ex_softc *));
static struct mbuf *exget __P((caddr_t, int, int, struct ifnet *));

#define ex_eprom_w(n)  DELAY(300*(n))

struct cfdriver excd = {
	NULL, "ex", exprobe, exattach, sizeof(struct ex_softc)
};

int
exprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;

{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register base;
	int irq;
	int n;

	base = ia->ia_iobase;
	if (!EX_IOBASEVALID(base)) {
		printf("ex%d: invalid i/o base address 0x%x\n",
		    cf->cf_unit, base);
		return (0);
	}

	if (ex_id(base+ID_REG, 1) != EX_ID)
		return (0);

	if (ia->ia_irq == IRQUNK) {
		if ((ia->ia_irq = isa_irqalloc(EX_IRQS)) == 0) {
			printf("ex%d: no irq available\n", cf->cf_unit);
			return (0);
		}
	}
	irq = ffs(ia->ia_irq) - 1;
	if (!EX_IRQVALID(irq)) {
		printf("ex%d: invalid interrupt level %d\n", cf->cf_unit, irq);
		return (0);
	}

	ia->ia_iosize = EX_NPORTS;
	return (1);
}

/*
 * Read the ID pattern from the specified port.
 * If check is true, check for the primary ID,
 * failing immediately if the pattern is wrong.
 */
static u_short
ex_id(port, check)
	int port, check;
{
	register int i, b, id;

	for (id = 0, i = 0; i < 4; i++) {
		b = inb(port);
		if (check && (b >> 4) != ((b & 1) ? 0xB : 0xA))
			return (0);
		id |= (b >> 4) << ((b & 0x3) * 4);
	}
	return (id);
}

void
exattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct ex_softc *sc = (struct ex_softc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct ifnet *ifp = &sc->ex_if;
	register base = ia->ia_iobase;
	int n, x;

	sc->ex_base = base;
	sc->ex_irq = ffs(ia->ia_irq) - 1;

	sc->ex_board_type = ex_id(base+ID_REG+0x3000, 0);

	/*
	 * Extract station address from EPROM
	 */
	outb(base+EP_REG, 0x80);
	ex_eprom_w(50);
	outb(base+EP_REG, 0xC0);
	ex_eprom_w(50);
	outb(base+EP_REG, 0x80);
	ex_eprom_w(50);
	outb(base+EP_REG, 0x00);
	ex_eprom_w(50);
	for (n = 0; n < 3; n++) {
		x = ex_eprom_r(base, (2 - n) + 0x02);
		sc->ex_addr[2*n] = (x >> 8) & 0xFF;
		sc->ex_addr[2*n+1] = x & 0xFF;
	}

	/* check connector type */
	if (sc->ex_board_type == EX_ID_GEN2) {
		x = ex_eprom_r(base, EE_W1);
		if (x & EE1_AUTOSENS)
			printf(": connector autosense not supported");
		x = ex_eprom_r(base, EE_W0);
#define right
/* the manual is not quite clear, but I think the first option is correct */
#ifdef right
		if (x & EE0_NOTAUI)
			sc->ex_conntype = BNC;
		else {
			x = ex_eprom_r(base, EE_W5);
			if (x & EE5_TP)
				sc->ex_conntype = TP;
			else
				sc->ex_conntype = AUI;
		}
#else
		if ((x & EE0_NOTAUI) == 0)
			sc->ex_conntype = AUI;
		else {
			x = ex_eprom_r(base, EE_W5);
			if (x & EE5_TP)
				sc->ex_conntype = TP;
			else
				sc->ex_conntype = BNC;
		}
#endif
	}

	/* Initialize interface structure */
	ifp->if_unit = sc->ex_dev.dv_unit;
	ifp->if_name = excd.cd_name;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;
	ifp->if_init = exinit;
	ifp->if_output = ether_output;
	ifp->if_start = exstart;
	ifp->if_ioctl = exioctl;
	ifp->if_watchdog = 0;
	if_attach(ifp);

	printf("\nex%d: EtherExpress 16 address %s", ifp->if_unit,
		ether_sprintf(sc->ex_addr));
	if (sc->ex_board_type == EX_ID_GEN2)
		printf("; %s connector", conn_name[sc->ex_conntype]);
	printf("\n");

#if NBPFILTER > 0
	bpfattach(&sc->ex_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
	isa_establish(&sc->ex_id, &sc->ex_dev);
	sc->ex_ih.ih_fun = exintr;
	sc->ex_ih.ih_arg = (void *)sc;
	intr_establish(ia->ia_irq, &sc->ex_ih, DV_NET);
}

exinit(unit)
	int unit;
{
	struct ex_softc *sc = excd.cd_devs[unit];
	register base = sc->ex_base;
	struct ifnet *ifp = &sc->ex_if;
	int s;
	int n;
	u_short x;

	if (ifp->if_addrlist == (struct ifaddr *) 0)
 		return;

	s = splimp();
	ifp->if_flags |= IFF_RUNNING;
	sc->ex_oldflags = ifp->if_flags;

	/* Reset the board */
	outb(base+EP_REG, 0x80);
	ex_eprom_w(50);
	outb(base+EP_REG, 0xC0);
	ex_eprom_w(50);
	outb(base+EP_REG, 0x80);
	ex_eprom_w(50);
	outb(base+EP_REG, 0x00);
	ex_eprom_w(50);

	/*
	 * Read memory size (??? - avg)
	 */
	outw(base+WR_REG, 0x0);
	outb(base+DX_REG, 0x0);

	outw(base+WR_REG, 0x8000);
	outb(base+DX_REG, 0x0);

	outw(base+WR_REG, 0x0);
	outb(base+DX_REG, 0xAA);

	outw(base+RD_REG, 0x8000);

	/*
	 * if (inb(base+DX_REG) == 0xAA)
	 *         ex_mem_size = 0x8000;
	 * else
	 *         ex_mem_size = 0x0000;
	 */
	(void) inb(base+DX_REG);

	/*
	 * Init SCP.
	 */
	outw(base+WR_REG, SCP_SYSBUS);       /* Set Write Addres */
	outw(base+DX_REG, 0);                /* SCP_SYSBUS */
	outw(base+WR_REG, SCP_ISCP);         /* Set Write Addres */
	outw(base+DX_REG, OFFSET_ISCP);      /* SCP_ISCP */
	outw(base+DX_REG, 0);                /* SCP_ISCP_BASE */

	/*
	 * Init ISCP.
	 */
	outw(base+WR_REG, OFFSET_ISCP);     /* Set Write Addres */
	outw(base+DX_REG, 1);               /* ISCP_BUSY */
	outw(base+DX_REG, OFFSET_SCB);      /* ISCP_SCB_OFFSET */
	outw(base+DX_REG, 0);               /* ISCP_SCB */
	outw(base+DX_REG, 0);               /* ISCP_SCB_BASE */

	/*
	 * Init SCB.
	 */
	outw(base+WR_REG, OFFSET_SCB);      /* Set Write Addres */
	outw(base+DX_REG, 0);               /* SCB_STATUS */
	outw(base+DX_REG, 0);               /* SCB_COMMAND */
	outw(base+DX_REG, 0xFFFF);          /* SCB_CBL_OFFSET */
	outw(base+DX_REG, 0xFFFF);          /* SCB_RFA_OFFSET */
	outw(base+DX_REG, 0);               /* SCB_CRCERRS */
	outw(base+DX_REG, 0);               /* SCB_ALNERRS */
	outw(base+DX_REG, 0);               /* SCB_RSCERRS */
	outw(base+DX_REG, 0);               /* SCB_OVRNERRS */

	/*
	 * Init TX buffers
	 */
	outw(base+WR_REG, OFFSET_CU);
	outw(base+DX_REG, 0x0000);          /* CU_STATUS */
	outw(base+DX_REG, AC_SW_EL);        /* CU_COMMAND */
	outw(base+DX_REG, 0xFFFF);          /* CU_LINK_OFFSET */
	outw(base+DX_REG, OFFSET_TBD);      /* CU_DATA */

	outw(base+WR_REG, OFFSET_TBD);
	outw(base+DX_REG, 0x0000);          /* TBD_ACT_COUNT */
	outw(base+DX_REG, CMD_NULL);        /* TBD_NEXT_TBD_OFFSET */
	outw(base+DX_REG, OFFSET_TXB);      /* TBD_BUFFER_ADDR */
	outw(base+DX_REG, 0x0000);          /* TBD_BUFFER_BASE */

	/*
	 * Init RX buffers
	 */
	ex_init_rx(sc);

	/*
	 * Configure the NIC
	 */
	outb(base+EP_REG, 0x80);
	outb(base+EP_REG, 0x00);
	outb(base+CA_REG, 0x01);

	for (n = 1000; n != 0; n--) {
		outw(base+RD_REG, ISCP_BUSY);
		if (((x = inb(base+DX_REG)) & 0x1) == 0)
			break;
		ex_eprom_w(1);
	}
	if (n == 0)
		printf("ex%d: init failed (stuck busy)\n", unit);

	for (n = 1000; n != 0; n--) {
		outw(base+RD_REG, SCB_STATUS);
		if ((x = inw(base+DX_REG)) == 0xA000)
			break;
		ex_eprom_w(1);
	}
	if (n == 0)
		printf("ex%d: init failed (status)\n", unit);

	if (ex_ack(base) != 0 || ex_config(sc) != 0)
		printf("ex%d: init failed (config)\n", unit);

	outw(base+WR_REG, OFFSET_SCB);
	outw(base+DX_REG, 0x0000);
	outw(base+DX_REG, 0x0010);

	outw(base+WR_REG, SCB_RFA_OFFSET);
	outw(base+DX_REG, sc->ex_begin_fd);
	outb(base+CA_REG, 1);

	if (ex_wait_scb(base) != 0 || ex_ack(base) != 0)
		printf("ex%d: init failed (offset cmd)\n", unit);

	if (sc->ex_board_type == EX_ID_GEN2) {
		x = inb(base+ECR1_REG) &~ (ECR1_NOTAUI|ECR1_TP);
		/*
		 * set type as selected in EEPROM by default,
		 * but allow IFF_LINK[01] to override if set.
		 */
		switch (ifp->if_flags & (IFF_LINK0|IFF_LINK1)) {

		case 0:
			if (sc->ex_conntype != AUI) {
				x |= ECR1_NOTAUI;
				if (sc->ex_conntype == TP)
					x |= ECR1_TP;
			}
			break;

		case IFF_LINK0:
			x |= ECR1_NOTAUI;		/* BNC */
			break;

		case IFF_LINK1:
			x |= ECR1_NOTAUI|ECR1_TP;	/* TP */
			break;

		case IFF_LINK0|IFF_LINK1:		/* force AUI */
			/* x &= ~(ECR1_NOTAUI|ECR1_TP); */
			break;
		}
		outb(base+ECR1_REG, x);
	}
	/*
	 * Setup irq
	 */
	n = sc->ex_irq;
	if (n == 9)
		n = 1;
	else if (n > 5)
		n -= 5;
	else
		n -= 1;
	outb(base+IS_REG, n | 0x8);

	/* Start transmission */
	exstart(ifp);
	splx(s);
}

int
exstart(ifp)
	struct ifnet *ifp;
{
	struct ex_softc *sc = excd.cd_devs[ifp->if_unit];
	register base = sc->ex_base;
	struct mbuf *m;
	u_short buff_addr;
	u_short msg_size;
	struct mbuf *mp;
	struct ether_header *eh;
	u_short size;
	u_short *userdata;

	if (ifp->if_flags & IFF_OACTIVE)
		return (0);

	IF_DEQUEUE(&ifp->if_snd, m);
	if (m == (struct mbuf *) 0)
		return (0);

#if NBPFILTER > 0
	/*
	 * Feed outgoing packet to bpf
	 */
	if (sc->ex_bpf)
		bpf_mtap(sc->ex_bpf, m);
#endif

	ifp->if_flags |= IFF_OACTIVE;

	eh = mtod(m, struct ether_header *);
	
	ex_wait_scb(base);

	outw(base+WR_REG, OFFSET_CU);
	outw(base+DX_REG, 0x0000);
	outw(base+DX_REG, AC_SW_EL | AC_SW_I | AC_TRANSMIT);
	outw(base+DX_REG, CMD_NULL);
	outw(base+DX_REG, OFFSET_TBD);

	outsw(base+DX_REG, eh->ether_dhost, ETHER_ADDR_LEN / 2);
	outw(base+DX_REG, eh->ether_type);

	buff_addr = OFFSET_RXB;
	msg_size = 0;

	size = 0;
	for (mp = m; mp != 0; mp = mp->m_next) {
		if (mp == m) {
			msg_size = (u_short) (mp->m_len -
			    sizeof(struct ether_header));
			userdata = (u_short *) (mtod(mp, u_char *) +
			    sizeof(struct ether_header));
		} else {
			msg_size = (u_short) mp->m_len;
			userdata = mtod(mp, u_short *);
		}

		if (msg_size == 0)
			continue;
			
		outw(base+WR_REG, buff_addr);

		if ((msg_size % 2) != 0) {
			outb(base+DX_REG, *((u_char *) userdata));
			outw(base+WR_REG, buff_addr + 1);
			userdata = (u_short *) (((u_char *) userdata) + 1);
		}

		if (msg_size >= 2)
			outsw(base+DX_REG, userdata, msg_size / 2);

		buff_addr += msg_size;
		size += msg_size;
	}

	if (size < ETHERMIN) {
		if (((ETHERMIN - size) % 2) != 0) {
			outw(base+WR_REG, buff_addr);
			outb(base+DX_REG, 0x00);
			size++;
			buff_addr++;
		}
		if (size < ETHERMIN) {
			outw(base+WR_REG, buff_addr);
			for (msg_size = (ETHERMIN - size) / 2; msg_size != 0;
			    msg_size--);
				outw(base+DX_REG, 0x0000);
		}
		size = ETHERMIN;
	}

	outw(base+WR_REG, OFFSET_TBD);
	outw(base+DX_REG, size | 0x8000);
	outw(base+DX_REG, CMD_NULL);
	outw(base+DX_REG, OFFSET_RXB);
	outw(base+DX_REG, 0);

	ex_wait_scb(base);

	outw(base+WR_REG, SCB_COMMAND);
	outw(base+DX_REG, SCB_CU_STRT);

	outb(base+CA_REG, 1);

	m_freem(m);
	ifp->if_opackets++;
	return (0);
}

int
exioctl(ifp, cmd, data)
	struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	struct ifaddr *ifa;
	struct ex_softc *sc = excd.cd_devs[ifp->if_unit];
	int s;

	ifa = (struct ifaddr *) data;
	s = splimp();

	switch(cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch(ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			exinit(ifp->if_unit);
			((struct arpcom *) ifp)->ac_ipaddr =
			     IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *) ifp,
			    &IA_SIN(ifa)->sin_addr);
			break;
#endif
#ifdef NS
		case AF_NS:
			{
				register struct ns_addr *ina = &(IA_SNS(ifa)->sns_addr);

				if (ns_nullhost(*ina))
					ina->x_host = *(union ns_host *)(sc->ex_addr);
				else {
					ifp->if_flags &= ~IFF_RUNNING;
					bcopy((caddr_t)ina->x_host.c_host,
					      (caddr_t)sc->ex_addr, sizeof(sc->ex_addr));
				}
			}
			/*FALL THROUGH*/
#endif
		default:
			exinit(ifp->if_unit);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    ifp->if_flags & IFF_RUNNING) {
			ifp->if_flags &= ~IFF_RUNNING;
			sc->ex_oldflags = ifp->if_flags;
			/* XXX -- the board should be disabled */
		} else if ((ifp->if_flags ^ sc->ex_oldflags) & (IFF_UP|IFF_PROMISC))
			exinit(ifp->if_unit);
		break;

	default:
		splx(s);
		return (EINVAL);
	}

	splx(s);
	return (0);
}

/*
 * Read EPROM register
 */
static u_short
ex_eprom_r(base, reg)
	register base;
	int reg;
{
	u_short	res;
	u_char x;

	x = inb(base+EP_REG);
	x |= 0x80;
	x &= 0xBF;
	outb(base+EP_REG, x);

	x = inb(base+EP_REG);
	x &= 0xB2;
	x |= 0x02;
	outb(base+EP_REG, x);

	ex_eprom_so(base, EEXP_EPROM_READ, 3);
	ex_eprom_so(base, reg, 6);
	res = ex_eprom_si(base);
	ex_eprom_c(base);
	return (res);
}

static void
ex_eprom_so(base, var, n)
	register base;
	int var;
	int n;
{
	u_char x;
	u_short	mask;

	x = inb(base+EP_REG) & 0xB3;
	for (mask = 1 << (n - 1); mask != 0; mask >>= 1) {
		x &= 0xFB;

		if ((var & mask) != 0)
			x |= 0x4;
		outb(base+EP_REG, x);

		ex_eprom_w(1);
		ex_eprom_RC(base, &x);
		ex_eprom_LC(base, &x);
	}
	outb(base+EP_REG, x & 0xFB);
}

static u_short
ex_eprom_si(base)
	register base;
{
	u_char x;
	u_short	n;
	u_short	res;

	x = inb(base+EP_REG) & 0xB3;
	for (res = 0, n = 0; n < 16; n++) {
		res <<= 1;

		ex_eprom_RC(base, &x);
		x = inb(base+EP_REG) & 0xBB;

		if ((x & 0x8) != 0)
			res |= 1;

		ex_eprom_LC(base, &x);
	}
	return (res);
}

static void
ex_eprom_RC(base, regv)
	register base;
	u_char *regv;
{
	*regv |= EE_SK;
	outb(base+EP_REG, *regv);
	ex_eprom_w(1);
}

static void
ex_eprom_LC(base, regv)
	register base;
	u_char *regv;
{
	*regv &= ~(EE_SK);
	outb(base+EP_REG, *regv);
	ex_eprom_w(1);
}

static void
ex_eprom_c(base)
	register base;
{
	u_char x;

	x = inb(base+EP_REG) & 0xBF;
	outb(base+EP_REG, x & 0xF9);
	ex_eprom_RC(base, &x);
	ex_eprom_LC(base, &x);
}

/*
 * Wait until SCB ready
 */
static int
ex_wait_scb(base)
	register base;
{
	int n;

	for (n = 0; n < 0xFFFF; n++) {
		outw(base+RD_REG, SCB_COMMAND);
		if (inw(base+DX_REG) == 0)
			return (0);
	}
	return (1);
}

/*
 * Acknowledge interrupt
 */
static int
ex_ack(base)
	register base;
{
	u_short x;

	outw(base+RD_REG, SCB_STATUS);
	if ((x = (inw(base+DX_REG) & SCB_SW_INT)) == 0)
		return (0);
	outw(base+WR_REG, SCB_COMMAND);
	outw(base+DX_REG, x);
	outb(base+CA_REG, 1);
	return (ex_wait_scb(base));
}

/*
 * Configure the receiver parameters
 */
static int
ex_config(sc)
	struct ex_softc *sc;
{
	register base = sc->ex_base;
	int n;
	u_short *maddr;

	if (ex_wait_scb(base) != 0 || ex_ack(base) != 0)
		return (1);
	
	outw(base+WR_REG, SCB_STATUS);
	outw(base+DX_REG, 0x0000);
	outw(base+DX_REG, SCB_CU_STRT);
	outw(base+DX_REG, OFFSET_CU);

	outw(base+WR_REG, CU_STATUS);
	outw(base+DX_REG, 0x0000);
	outw(base+DX_REG, AC_CONFIG | AC_SW_EL);
	outw(base+DX_REG, CMD_NULL);

	outw(base+WR_REG, CU_DATA);
	outw(base+DX_REG, 0x080C);
	outw(base+DX_REG, 0x2600);
	outw(base+DX_REG, 0x6000);
	outw(base+DX_REG, 0xF200);
	if (sc->ex_if.if_flags & IFF_PROMISC)
		outw(base+DX_REG, 0x0001);
	else
		outw(base+DX_REG, 0x0000);
	outw(base+DX_REG, 0x0040);
	outb(base+CA_REG, 1);

	if (ex_wait_scb(base) != 0 || ex_ack(base) != 0)
		return (1);

	outw(base+WR_REG, SCB_STATUS);
	outw(base+DX_REG, 0x0000);
	outw(base+DX_REG, SCB_CU_STRT);
	outw(base+DX_REG, OFFSET_CU);

	outw(base+WR_REG, CU_STATUS);
	outw(base+DX_REG, 0);
	outw(base+DX_REG, AC_IASETUP | AC_SW_EL);
	outw(base+DX_REG, CMD_NULL);

	outw(base+WR_REG, CU_DATA);
	maddr = (u_short *) &sc->ex_addr;
	for (n = 0; n < ETHER_ADDR_LEN / 2; n++)
		outw(base+DX_REG, *maddr++);
	outb(base+CA_REG, 1);

	return (ex_wait_scb(base) != 0 || ex_ack(base) != 0);
}

/*
 * Initialize receive buffers
 */
static void
ex_init_rx(sc)
	struct ex_softc *sc;
{
	register base = sc->ex_base;
	u_short x;
	int i;
	u_short rbd_p;
	u_short rxb_p;
	u_short fd_p;

	sc->ex_begin_rbd = OFFSET_RXB + N_RBD * 0x5F0;
	sc->ex_begin_fd = sc->ex_begin_rbd + N_RBD * 0xA;

	fd_p = sc->ex_begin_fd;
	for (i = 0; i < N_FD; i++) {
		outw(base+WR_REG, fd_p);
		outw(base+DX_REG, 0x0000);          /* FD_STATUS */
		outw(base+DX_REG, 0x0000);          /* FD_COMMAND */
		outw(base+DX_REG, fd_p + 0x16);     /* FD_LINK_OFFSET */
		outw(base+DX_REG, CMD_NULL);        /* FD_RDB_OFFSET */

		fd_p += 0x16;
	}
	sc->ex_end_fd = fd_p - 0x16;

	outw(base+WR_REG, FD_RBD(sc->ex_begin_fd));
	outw(base+DX_REG, sc->ex_begin_rbd);

	outw(base+WR_REG, FD_COMMAND(sc->ex_end_fd));
	outw(base+DX_REG, 0x8000);
	outw(base+DX_REG, 0xFFFF);

	rxb_p = OFFSET_RXB;
	rbd_p = sc->ex_begin_rbd;
	for (i = 0; i < N_RBD; i++) {
		outw(base+WR_REG, rbd_p);
		outw(base+DX_REG, 0);
		outw(base+DX_REG, rbd_p + 0xA);
		outw(base+DX_REG, rxb_p);
		outw(base+DX_REG, 0);
		outw(base+DX_REG, 0x5F0);

		rbd_p += 0xA;
		rxb_p += 0x5F0;
	}

	sc->ex_end_rbd = rbd_p - 0xA;
	outw(base+WR_REG, RBD_NEXT_RBD(sc->ex_end_rbd));
	outw(base+DX_REG, 0xFFFF);

	outw(base+RD_REG, RBD_SIZE(sc->ex_end_rbd));
	x = inw(base+DX_REG);
	outw(base+WR_REG, RBD_SIZE(sc->ex_end_rbd));
	outw(base+DX_REG, x | 0x8000);
}

/*
 * Interrupt routine
 */
int
exintr(arg)
	void *arg;
{
	struct ex_softc *sc = (struct ex_softc *) arg;
	register base = sc->ex_base;
	u_short scb_status;
	u_short cmd_status;
	int n;
	struct mbuf *m;
	int iv;

	ex_wait_scb(base);

	outw(base+WR_REG, SCB_STATUS);
	scb_status = inw(base+DX_REG);              /* SCB_STATUS */

	if ((scb_status & SCB_SW_INT) == 0)
		return (0);

	ex_ack(base);

	if ((scb_status & (SCB_SW_FR | SCB_SW_RNR)) != 0)
		ex_rcv(sc);

	ex_wait_scb(base);

	if ((scb_status & (SCB_SW_CNA | SCB_SW_CX)) != 0) {
		sc->ex_if.if_flags &= ~IFF_OACTIVE;
		exstart(&sc->ex_if);
	}

	iv = inb(base+IS_REG);
	outb(base+IS_REG, iv & 0xF7);
	outb(base+IS_REG, iv | 0x08);

	return (1);
}

/*
 * Receive the packet
 */
static void
ex_rcv(sc)
	struct ex_softc *sc;
{
	register base = sc->ex_base;
	u_short fd_status;
	u_short fd_link;
	u_short f_rbd;
	u_short l_rbd;
	u_short rbd_status;
	u_short fd;
	u_short scb_status;

	for (fd = sc->ex_begin_fd; fd != 0; fd = sc->ex_begin_fd) {
		outw(base+RD_REG, FD_STATUS(fd));
		fd_status = inw(base+DX_REG);

		if ((fd_status & AC_SW_C) != 0) {
			outw(base+RD_REG, FD_LINK(fd));
			fd_link = inw(base+DX_REG);         /* FD_LINK */
			f_rbd   = inw(base+DX_REG);         /* FD_RBD */

			sc->ex_begin_fd = (fd_link == 0xffff) ? 0 : fd_link;

			if (f_rbd != 0xFFFF) {
				l_rbd = f_rbd;
				for (;;) {
					outw(base+RD_REG, RBD_STATUS(l_rbd));
					rbd_status = inw(base+DX_REG);

					if ((rbd_status & RBD_SW_EOF) == RBD_SW_EOF)
						break;
					/* !!! one more if */

					outw(base+RD_REG, RBD_NEXT_RBD(l_rbd));
					l_rbd = inw(base+DX_REG);
				}
				if ((fd_status & AC_SW_OK) != 0)
					ex_readdata(sc, fd);
			}
			ex_reqfd(sc, fd);
		} else
			break;
	}

	outw(base+RD_REG, SCB_STATUS);
	scb_status = inw(base+DX_REG);

	if ((scb_status & SCB_RUS_READY) == SCB_RUS_READY)
		return;

	if (sc->ex_begin_fd == 0)
		return;

	outw(base+RD_REG, sc->ex_begin_fd);
	fd_status = inw(base+DX_REG);

	if ((fd_status & AC_SW_C) != 0)
		return;

	outw(base+WR_REG, FD_RBD(sc->ex_begin_fd));
	outw(base+DX_REG, sc->ex_begin_rbd);

	outw(base+WR_REG, SCB_COMMAND);
	outw(base+DX_REG, SCB_CU_STRT);
	outw(base+WR_REG, SCB_RFA_OFFSET);
	outw(base+DX_REG, sc->ex_begin_fd);
	outb(base+CA_REG, 1);
}

static void
ex_reqfd(sc, fd)
	struct ex_softc *sc;
	unsigned fd;
{
	register base = sc->ex_base;
	u_short f_rbd;
	u_short l_rbd;
	u_short stat;
	u_short size;

	outw(base+RD_REG, FD_RBD(fd));
	f_rbd = inw(base+DX_REG);
	if (f_rbd == 0xffff)
		f_rbd = 0;

	outw(base+WR_REG, FD_STATUS(fd));
	outw(base+DX_REG, 0x0000);          /* FD_STATUS */
	outw(base+DX_REG, AC_SW_EL);        /* FD_COMMAND */
	outw(base+DX_REG, 0xFFFF);          /* FD_LINK */
	outw(base+DX_REG, 0xFFFF);          /* FD_RBD */

	if (sc->ex_begin_fd == 0) {
		sc->ex_begin_fd = fd;
		sc->ex_end_fd = fd;
	}

	outw(base+WR_REG, FD_LINK(sc->ex_end_fd));
	outw(base+DX_REG, fd);
	outw(base+WR_REG, FD_COMMAND(sc->ex_end_fd));
	outw(base+DX_REG, 0);
	sc->ex_end_fd = fd;

	if (f_rbd != 0) {
		l_rbd = f_rbd;
		for (;;) {
			outw(base+RD_REG, RBD_STATUS(l_rbd));
			stat = inw(base+DX_REG);

			if ((stat & RBD_SW_EOF) == RBD_SW_EOF)
				break;
			/* !!! one more if */

			outw(base+WR_REG, RBD_STATUS(l_rbd));
			outw(base+DX_REG, 0);

			outw(base+RD_REG, RBD_NEXT_RBD(l_rbd));
			l_rbd = inw(base+DX_REG);
		}
		/* !!! */
		outw(base+WR_REG, RBD_NEXT_RBD(l_rbd));
		outw(base+DX_REG, CMD_NULL);
		/* !!! */

		outw(base+WR_REG, RBD_STATUS(l_rbd));
		outw(base+DX_REG, 0);

		outw(base+RD_REG, RBD_SIZE(l_rbd));
		size = inw(base+DX_REG);
		outw(base+WR_REG, RBD_SIZE(l_rbd));
		outw(base+DX_REG, size | AC_SW_EL);

		outw(base+WR_REG, RBD_NEXT_RBD(sc->ex_end_rbd));
		outw(base+DX_REG, f_rbd);

		outw(base+RD_REG, RBD_SIZE(sc->ex_end_rbd));
		size = inw(base+DX_REG);
		outw(base+WR_REG, RBD_SIZE(sc->ex_end_rbd));
		outw(base+DX_REG, size & ~(AC_SW_EL));

		sc->ex_end_rbd = l_rbd;
	}
}

/*
 * Read data from the board and pass upstream
 */
static void
ex_readdata(sc, fd)
	struct ex_softc *sc;
	unsigned fd;
{
	register base = sc->ex_base;
	u_short rbd;
	u_short rbd_status;
	u_short buf;
	u_short *p1;
	u_char *p2;
	int n;
	int size;
	int msiz;
	char *ptr;
	struct ifnet *ifp = &sc->ex_if;
	struct ether_header *eh;
	static int logged;

	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		return;

	outw(base+RD_REG, FD_RBD(fd));
	rbd = inw(base+DX_REG);

	if (rbd == 0 || rbd == 0xFFFF)
		return;

	outw(base+RD_REG, RBD_BUFFER_ADDR(rbd));
	buf = inw(base+DX_REG);

	if (buf == 0 || buf == 0xFFFF)
		return;
	eh = (struct ether_header *) sc->ex_data;

	outw(base+RD_REG, FD_LENGTH(fd));
	eh->ether_type = ntohs(inw(base+DX_REG));

	outw(base+RD_REG, FD_DSR_ADDR(fd));
	insw(base+DX_REG, (u_short *) eh->ether_dhost, ETHER_ADDR_LEN / 2);

	outw(base+RD_REG, FD_SRC_ADDR(fd));
	insw(base+DX_REG, (u_short *) eh->ether_shost, ETHER_ADDR_LEN / 2);

	/*
	 * Copy Edata.
	 */
	ptr = sc->ex_data + sizeof(struct ether_header);
	outw(base+RD_REG, FD_RBD(fd));
	rbd = inw(base+DX_REG);
	size = 0;
	for (;;) {
		if (rbd == 0 || rbd == 0xFFFF)
			break;

		outw(base+RD_REG, RBD_STATUS(rbd));
		rbd_status = inw(base+DX_REG);

		msiz = rbd_status & RBD_SW_COUNT;
		outw(base+RD_REG, RBD_BUFFER_ADDR(rbd));
		buf = inw(base+DX_REG);

		p1 = (u_short *) ptr;
		if (size + msiz > ETHER_MAX_LEN) {
			if (logged++ == 0)
				log(LOG_ERR, "ex%d: packet too long\n",
				    sc->ex_if.if_unit);
			if ((msiz = ETHER_MAX_LEN - size) <= 0)
				goto skip;	/* not sure this is necessary */
		}
		outw(base+RD_REG, buf);
		insw(base+DX_REG, p1, msiz / 2);

		if ((msiz % 2) != 0) {
			 p2 = (u_char *) ptr + msiz - 1;
			*p2 = inb(base+DX_REG);
		}
		ptr += msiz;
		size += msiz;
	skip:
		if ((rbd_status & RBD_SW_EOF) == RBD_SW_EOF)
			break;

		outw(base+RD_REG, RBD_NEXT_RBD(rbd));
		rbd = inw(base+DX_REG);
	}

	ifp->if_ipackets++;
	exread(sc->ex_data, size, eh, sc);
}

static void
exread(buf, len, eh, sc)
	char *buf;
	int len;
	struct ether_header *eh;
	struct ex_softc *sc;
{
	struct mbuf *m;
	int off;
	int resid;

#define exdataaddr(eh,off,type) \
	((type)(((caddr_t)((eh)+1)+(off))))

	if (eh->ether_type >= ETHERTYPE_TRAIL &&
	    eh->ether_type < (ETHERTYPE_TRAIL + ETHERTYPE_NTRAILER)) {
		off = (eh->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU)
			return;
		eh->ether_type = ntohs(*exdataaddr(eh, off, u_short *));
		resid = ntohs(*(exdataaddr(eh, off + 2, u_short *)));
		if (off + resid > len)
			return;
		len = off + resid;
	} else
		off = 0;

	if (len == 0)
		return;

#if NBPFILTER > 0
        /*
         * Check if there's a bpf filter listening on this interface.
         * If so, hand off the raw packet to bpf, which must deal with
         * trailers in its own way.
         */
	if (sc->ex_bpf) {
                eh->ether_type = htons((u_short)eh->ether_type);
		bpf_tap(sc->ex_bpf, buf, len + sizeof(struct ether_header));
                eh->ether_type = ntohs((u_short)eh->ether_type);

                /*
                 * Note that the interface cannot be in promiscuous mode if
		 * there are no bpf listeners.  And if ex are in promiscuous
		 * mode, ex have to check if this packet is really ours.
                 *
                 * This test does not support multicasts.
                 */
		if ((sc->ex_if.if_flags & IFF_PROMISC)
		    && bcmp(eh->ether_dhost, sc->ex_addr,
                            sizeof(eh->ether_dhost)) != 0
                    && bcmp(eh->ether_dhost, etherbroadcastaddr, 
                            sizeof(eh->ether_dhost)) != 0)
		return;
        }
#endif

	m = exget(buf, len, off, &sc->ex_if);
	if (m == 0)
		return;

	ether_input(&sc->ex_if, eh, m);
}

static struct mbuf *
exget(buf, totlen, off0, ifp)
	caddr_t buf;
	int totlen;
	int off0;
	struct ifnet *ifp;
{
	struct mbuf *top;
	struct mbuf **mp;
	struct mbuf *m;
	int off;
	int len;
	caddr_t cp;
	char *epkt;

	off = off0;
	cp = buf;
	
	buf += sizeof(struct ether_header);
	cp = buf;
	epkt = cp + totlen;

	if (off) {
		cp += off + 2 * sizeof(u_short);
		totlen -= 2 * sizeof(u_short);
	}

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return (0);
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	m->m_len = MHLEN;

	top = 0;
	mp = &top;
	while (totlen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return (0);
			}
			m->m_len = MLEN;
		}
		len = min(totlen, epkt - cp);
		if (len >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if (m->m_flags & M_EXT)
				m->m_len = len = min(len, MCLBYTES);
			else
				len = m->m_len;
		} else {
			if (len < m->m_len) {
				if (top == 0 && len + max_linkhdr <= m->m_len)
					m->m_data += max_linkhdr;
				m->m_len = len;
			} else
				len = m->m_len;
		}
		bcopy(cp, mtod(m, caddr_t), (unsigned) len);
		cp += len;
		*mp = m;
		mp = &m->m_next;
		totlen -= len;
		if (cp == epkt)
			cp = buf;
	}
	return (top);
}
