/*
 * Copyright (c)1993 Foretune Co., Ltd. All rights reserved
 * Written by Sei Kigoshi <overwood@foretune.co.jp>
 *
 *	BSDI $Id: if_re.c,v 1.2 1994/01/30 05:27:36 karels Exp $
 */

/*
 * Allied Telesis RE2000/AT-1700 series Ethernet adapter driver
 */

/* #define REDEBUG */

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
#include "net/route.h"

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
#include "net/bpf.h"
#include "net/bpfdesc.h"
#endif

#include "if_rereg.h"

#ifdef REDEBUG
#define dprintf(x)	printf x
#else
#define dprintf(x)
#endif

/*
 * Ethernet software status per interface.
 */
struct re_softc {
	struct device	re_dev;		/* base device (must be first) */
	struct isadev	re_id;		/* ISA device */
	struct intrhand	re_ih;		/* interrupt vectoring */

	struct arpcom	re_ac;		/* Ethernet common part */
#define re_if	re_ac.ac_if		/* network-visible interface */
#define	re_addr	re_ac.ac_enaddr		/* hardware Ethernet address */

	caddr_t		re_base;	/* I/O port base address */
	char		re_pb[2048];	/* Input packet buffer */

#if NBPFILTER > 0 
	caddr_t		re_bpf;		/* packet filter */
#endif
};

/*
 * Prototypes
 */
int	reprobe __P((struct device *, struct cfdata *, void *));
void	reattach __P((struct device *, struct device *, void *));
int	reintr __P((struct re_softc *));
int	reinit __P((int));
int	rewatchdog __P ((int));
int	restart __P((struct ifnet *));
int	restop __P((caddr_t));
int	reioctl __P((struct ifnet *, int, caddr_t));
void	reread __P((struct re_softc *, caddr_t, int));
struct mbuf	*reget __P((caddr_t, int, int, struct ifnet *));

static u_short	re_rom_read_bits __P((caddr_t, int));
static void	re_rom_write_bits __P((caddr_t, unsigned, int));
static u_short	re_rom_read_word __P((caddr_t, int));
static void	mb_chipinit __P((caddr_t, u_char *addr, int));

struct cfdriver	recd =
	{NULL, "re", reprobe, reattach, sizeof(struct re_softc)};

/*
 * EEPROM operation
 */
static u_short
re_rom_read_bits(base, nbits)
	caddr_t base;
	int nbits;
{
	u_short retdata = 0;
	int i;

	for (i = 0; i < nbits; i++) {
		outb(base + ROM_CTL, ROM_CS_SK); ROM_DELAY();
		retdata <<= 1;
		retdata |= (inb(base + ROM_DATA) & ROM_RD) ? 0x01 : 0x00;
		outb(base + ROM_CTL, ROM_CS); ROM_DELAY();
	}
	return retdata;
}

static void
re_rom_write_bits(base, writedata, nbits)
	caddr_t base;
	unsigned writedata;
	int nbits;
{
	u_short bitmask = (1 << (nbits - 1));
	short writebit;
	int i;

	for (i = 0; i < nbits; i++) {
		writebit = (writedata & bitmask) ? ROM_WR1 : ROM_WR0;
		outb(base + ROM_DATA, writebit); ROM_DELAY();
		outb(base + ROM_CTL, ROM_CS_SK); ROM_DELAY();
		outb(base + ROM_CTL, ROM_CS); ROM_DELAY();
		bitmask >>= 1;
	}
}

static u_short
re_rom_read_word(base, offset)
	caddr_t base;
	int offset;
{
	u_short retdata;

	/* open EEPROM */
	outb(base + ROM_CTL, ROM_OFF); ROM_DELAY();
	outb(base + ROM_DATA, ROM_WR0); ROM_DELAY();
	outb(base + ROM_CTL, ROM_CS); ROM_DELAY();

	/* send 'read' command */
	re_rom_write_bits(base, 0x06, 3);

	/* send address */
	re_rom_write_bits(base, offset, 6);
	outb(base + ROM_DATA, ROM_WR0); ROM_DELAY();

	/* read data */
	retdata =  re_rom_read_bits(base, 16);

	/* close EEPROM */
	outb(base + ROM_DATA, ROM_WR0); ROM_DELAY();
	outb(base + ROM_CTL, ROM_SK); ROM_DELAY();
	outb(base + ROM_CTL, ROM_OFF); ROM_DELAY();
	return retdata;
}

/*
 * Probe the RE2000 to see if it's there
 */
int
reprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = (struct isa_attach_args *)aux;
	caddr_t base = (caddr_t)ia->ia_iobase;
	u_char addr[ETH_ADDR_LEN];
	u_short addrbuf;
	static u_short validbase[] =
		{0x240, 0x260, 0x280, 0x2a0, 0x300, 0x320, 0x340, 0x380, 0};
	static int irqtype[16] =
		{0, 0, 1, 2, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0};
	static int irqtab[3][4] = {
		{3, 4, 5, 9},		/* Low IRQ type */
		{3, 11, 5, 15},		/* Standard IRQ type */
		{10, 11, 14, 15}	/* High IRQ type */
	};
	int i, irq, type;

	/* Check I/O base address */
	for (i = 0;; i++) {
		if (validbase[i] == 0) {
			printf("re%d: invalid I/O base address 0x%03x\n",
				cf->cf_unit, base);
			return 0;
		}
		if (base == (caddr_t)validbase[i])
			break;
	}

	/* Try to read station address from EEPROM */
	for (i = 0; i < ETH_ADDR_LEN / 2; i++) {
		addrbuf = re_rom_read_word(base, i + 4);
		addr[i * 2] = (addrbuf >> 8) & 0xff;
		addr[i * 2 + 1] = addrbuf & 0xff;
	}

	/* Check vendor code */
	if (addr[0] != RE_VID0 || addr[1] != RE_VID1 ||
	    addr[2] != RE_VID2 || addr[3] != RE_PID)
		return 0;		/* not found */

	/* Lock the I/O base address */
	outb(base + RE_CTL2, 0x28);	/* select register bank 2 */
	outb(base + BMPR13, 0x00);	/* lock */

	/* Read board type from EEPROM */
	i = re_rom_read_word(base, 0x0f);
	if (irqtype[i & 0x0f] < 0) {
		printf("re%d: unknown board type 0x%04x\n", cf->cf_unit, i);
		return 0;
	}
	i = irqtype[i & 0x0f];
	irq = irqtab[i][(inb(base + RE_CONFIG) >> 6) & 0x03];
	if (ia->ia_irq == IRQUNK) {
		if ((ia->ia_irq = isa_irqalloc(1 << irq)) == 0) {
			printf("re%d: IRQ %d not available\n",
				cf->cf_unit, irq);
			return 0;
		}
	}
	if (irq != ffs(ia->ia_irq) - 1)
		return 0;		/* not found */
	ia->ia_iosize = RE_NPORT;
	return 1;			/* RE2000 found */
}

/*
 * Initialize MB86965A
 */
static void
mb_chipinit(base, addr, promisc)
	caddr_t base;
	u_char *addr;
	int promisc;
{
	int i;

	/* initialize MB86965A */
	outb(base + RE_CTL1, 0xd2);	/* reset command */

	/* set station address to MB86965A */
	outb(base + RE_CTL2, 0x20);	/* select regster bank 0 */
	for (i = 0; i < ETH_ADDR_LEN; i++)
		outb(base + RE_LAR + i, addr[i]);

	/* clear multicast hash filter */
	outb(base + RE_CTL2, 0x24);	/* select register bank 1 */
	for (i = 0; i < 8; i++)
		outb(base + RE_MAR + i, 0x00);

	/* mask all intr */
	outb(base + RE_CTL2, 0x28);	/* select register bank 2 */
	outb(base + RE_TX_MASK, 0x00);
	outb(base + RE_RX_MASK, 0x00);
	outb(base + RE_TR_MASK, 0x00);

	outb(base + RE_CTL1, 0x52);	/* running ... */
	outb(base + RE_TX_MODE, 0x02);
	outb(base + RE_RX_MODE, promisc ? 0x03 : 0x02);
	outb(base + RE_TR_MODE, 0x00);
	outb(base + RE_TX_STAT, 0xff);	/* clear TX error status */
	outb(base + RE_RX_STAT, 0xff);	/* clear RX error status */
	outb(base + RE_TR_STAT, 0xfa);	/* clear TR error status */
	outb(base + BMPR11, 0x05);	/* 16coll auto retransmit mode */
	outb(base + BMPR12, 0x00);	/* disable all DMA */
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
void
reattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct re_softc *sc = (struct re_softc *)self;
	struct isa_attach_args *ia = (struct isa_attach_args *)aux;
	struct ifnet *ifp = &sc->re_if;
	caddr_t base = sc->re_base = (caddr_t)ia->ia_iobase;
	u_short addrbuf;
	int i, type;
	static char *boardname[] = {
		"RE2001/AT-1700T",	/* 10BASE-T	*/
		"RE2003/AT-1700BT",	/* 10BASE-T/2	*/
		"RE2009/AT-1700FT",	/* 10BASE-T/FL	*/
		"RE2005/AT-1700AT"	/* 10BASE-T/5	*/
	};

	/* read station address from EEPROM */
	for (i = 0; i < ETH_ADDR_LEN / 2; i++) {
		addrbuf = re_rom_read_word(base, i + 4);
		sc->re_addr[i * 2] = (addrbuf >> 8) & 0xff;
		sc->re_addr[i * 2 + 1] = addrbuf & 0xff;
	}
	mb_chipinit(base, sc->re_addr, 0);

	/* Initialize interface structure */
	ifp->if_unit     = sc->re_dev.dv_unit;
	ifp->if_name     = recd.cd_name;
	ifp->if_mtu      = ETHERMTU;
	ifp->if_flags    = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;
	ifp->if_init     = reinit;
	ifp->if_output   = ether_output;
	ifp->if_start    = restart;
	ifp->if_ioctl    = reioctl;
	ifp->if_watchdog = rewatchdog;
	if_attach(ifp);

	/* Banner */
	printf("\nre%d: ", ifp->if_unit);
	if ((type = (re_rom_read_word(base, 0x0f) >> 8) & 0xff) < 4)
		printf("%s ", boardname[type]);
	else
		printf("RE2000/AT-1700(type 0x%02x) ", type);
	printf("address %s \n", ether_sprintf(sc->re_addr));

#if NBPFILTER > 0
	bpfattach(&sc->re_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	isa_establish(&sc->re_id, &sc->re_dev);
	sc->re_ih.ih_fun = reintr;
	sc->re_ih.ih_arg = (void *)sc;
	intr_establish(ia->ia_irq, &sc->re_ih, DV_NET);
	outb(base + RE_RX_MASK, 0x80);	/* enable PKT_RDY intr */
}

/*
 * Timeout
 */
int
rewatchdog(unit)
	int unit;
{
	struct re_softc *sc = (struct re_softc *)recd.cd_devs[unit];

#ifdef REDEBUG
	log(LOG_WARNING, "re%d: soft reset\n", unit);
#endif
	if (sc->re_if.if_flags & IFF_OACTIVE)
		reinit(unit);
}

/*
 * Initialization of interface
 */
int
reinit(unit)
	int unit;
{
	struct re_softc *sc = recd.cd_devs[unit];
	struct ifnet *ifp = &sc->re_if;
	caddr_t base = sc->re_base;
	int s;

	if (ifp->if_addrlist == (struct ifaddr *)0)
		return 0;

	s = splimp();
	/* Initialize MB86965A Registers */
	mb_chipinit(base, sc->re_addr, ifp->if_flags & IFF_PROMISC);

	(void)inw(base + RE_BUF_IO);	/* dummy read 2 words */
	(void)inw(base + RE_BUF_IO); 
	outb(base + RE_RX_STAT, 0xff);	/* and clear error */
	outb(base + RE_RX_MASK, 0x80);	/* enable PKT_RDY intr */

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	restart(ifp);
	splx(s);
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue,
 * and map it to the interface before starting the output.
 * called only at splimp or interrupt level.
 */
int
restart(ifp)
	struct ifnet *ifp;
{
	struct re_softc *sc = recd.cd_devs[ifp->if_unit];
	struct mbuf *m0, *m;
	caddr_t base = sc->re_base;
	int len, sublen;

	if (sc->re_if.if_flags & IFF_OACTIVE) {
		dprintf(("re%d: OACTIVE\n", ifp->if_unit));
		return 0;
	}

	IF_DEQUEUE(&sc->re_if.if_snd, m0);
	if (m0 == 0)
		return 0;
	sc->re_if.if_flags |= IFF_OACTIVE;	/* prevent entering restart */
	sc->re_if.if_timer = 1;			/* shouldn't take over 1 sec */
#if NBPFILTER > 0
	/*
	 * If bpf is listening on this interface, let it
	 * see the packet before we commit it to the wire.
	 */
	if (sc->re_bpf)
		bpf_mtap(sc->re_bpf, m0);
#endif
	/*
	 * Calculate the length of a packet.
	 */
	for (len = 0, m = m0; m != 0; m = m->m_next)
		len += m->m_len;
	sublen = 0;
	if (len + (len & 1) < ETHERMIN + sizeof(struct ether_header)) {
		sublen = ETHERMIN + sizeof(struct ether_header) -
			(len + (len & 1));
	}

	outb(base + RE_TX_MASK, 0x00);
	/* Write packet length */
	outw(base + RE_BUF_IO, len + (len & 1) + sublen);

	/* Write contents of packet */
	for (m = m0; m != 0; ) {
		if (m->m_len >= 2)
			outsw(base + RE_BUF_IO, m->m_data, m->m_len / 2);
		if (m->m_len & 1) {
			u_short oddword = 0;
			oddword = *(mtod(m, caddr_t) + m->m_len - 1) & 0xff;
			if (m = m->m_next) {
				oddword |= *(mtod(m, caddr_t)) << 8;
				m->m_data++;
				m->m_len--;
			}
			outw(base + RE_BUF_IO, oddword);
		} else {
			m = m->m_next;
		}
	}
	if (sublen) {
		static char dummy[ETHERMIN + sizeof(struct ether_header)];
		outsw(base + RE_BUF_IO, dummy, sublen / 2);
	}

	/* Write packet count */
	outw(base + RE_TX_CNT, 0x81);
	outb(base + RE_TX_MASK, 0x9f);	/* enable TMT intr */
	m_freem(m0);
	return 0;
}

/*
 * Controller interrupt.
 */
int
reintr(sc)
	struct re_softc *sc;
{
	u_char rsts, osts;
	u_short sts, len;
	caddr_t base = sc->re_base;

	rsts = inb(base + RE_RX_STAT);
	outb(base + RE_RX_STAT, 0xff);
	osts = inb(base + RE_TX_STAT);
	outb(base + RE_TX_STAT, 0xff); 
	if (!(inb(base + RE_RX_MODE) & 0x40)) {
		outb(base + RE_RX_MASK, 0x00);

		/* Read packet */
		while (!(inb(base + RE_RX_MODE) & 0x40)) {
			sc->re_if.if_ipackets++;
			sts = inw(base + RE_BUF_IO) & 0x3f;
			len = inw(base + RE_BUF_IO);
			if (len > 30 && len <= ETHERMTU + 100) {
				insw(base + RE_BUF_IO, sc->re_pb,
					len / 2 + (len & 1));
				reread(sc, (caddr_t)sc->re_pb,
					len - sizeof(struct ether_header));
			} else {
				log(LOG_ERR,
					"re%d: bad input packet length %d\n",
					sc->re_if.if_unit, len);
				sc->re_if.if_ierrors++;
				reinit(sc->re_if.if_unit);
			}
		}
		outb(base + RE_RX_MASK, 0x80);
	}
	if (osts & 0x1f) { 		/* Any error ? */
		int errcnt = 0;
		if (osts & 0x04) {	/* Collision */
			sc->re_if.if_collisions +=
				(inb(base + RE_COLL_CNT) >> 4) & 0x0f;
		}
		if (osts & 0x02) { 	/* 16 Collisions */
			sc->re_if.if_collisions += 16;
			errcnt = 1;
		}
		if (osts & 0x01) {	/* Bus write error */
			log(LOG_ERR,
				"re%d: TX buffer overflow\n",
				sc->re_if.if_unit);
			errcnt = 1;
		}
		if (osts & 0x10)	/* Short packet */
			errcnt = 1;
		sc->re_if.if_oerrors += errcnt;
		/* Clear error flags */
	} 
	if (osts & 0x80) {	/* TMT OK ? */
		sc->re_if.if_opackets++;
		sc->re_if.if_timer = 0;
		sc->re_if.if_flags &= ~IFF_OACTIVE;
		restart(&sc->re_if);
	}
	return 1;
}

/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
void
reread(sc, buf, len)
	register struct re_softc *sc;
	caddr_t buf;
	int len;
{
	register struct ether_header *eh;
    	struct mbuf *m, *reget();
	int off, resid;

	/*
	 * Deal with trailer protocol: if type is trailer type
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	eh = (struct ether_header *)buf;
	eh->ether_type = ntohs((u_short)eh->ether_type);
#define	redataaddr(eh, off, type)	((type)(((caddr_t)((eh)+1)+(off))))
	if (eh->ether_type >= ETHERTYPE_TRAIL &&
	    eh->ether_type < ETHERTYPE_TRAIL + ETHERTYPE_NTRAILER) {
		off = (eh->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU)
			return;		/* sanity */
		eh->ether_type = ntohs(*redataaddr(eh, off, u_short *));
		resid = ntohs(*(redataaddr(eh, off + 2, u_short *)));
		if (off + resid > len)
			return;		/* sanity */
		len = off + resid;
	} else
		off = 0;

	if (len <= 0)
		return;
#if NBPFILTER > 0
	/*
	 * Check if there's a bpf filter listening on this interface.
	 * If so, hand off the raw packet to bpf, which must deal with
	 * trailers in its own way.
	 */
	if (sc->re_bpf) {
		eh->ether_type = htons((u_short)eh->ether_type);
		bpf_tap(sc->re_bpf, buf, len + sizeof(struct ether_header));
		eh->ether_type = ntohs((u_short)eh->ether_type);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no bpf listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 *
		 * XXX This test does not support multicasts.
		 */
		if ((sc->re_if.if_flags & IFF_PROMISC) &&
		    bcmp(eh->ether_dhost, sc->re_addr, 
		    sizeof(eh->ether_dhost)) != 0 &&
		    bcmp(eh->ether_dhost, etherbroadcastaddr, 
		    sizeof(eh->ether_dhost)) != 0)
			return;
	}
#endif
	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; neget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	m = reget(buf, len, off, &sc->re_if);
	if (m == 0)
		return;
	ether_input(&sc->re_if, eh, m);
}

/*
 * Pull read data off a interface.
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We copy the trailer information and then all the normal
 * data into mbufs.  When full cluster sized units are present
 * we copy into clusters.
 */
struct mbuf *
reget(buf, totlen, off0, ifp)
	caddr_t buf;
	int totlen, off0;
	struct ifnet *ifp;
{
	struct mbuf *top, **mp, *m;
	int off = off0, len;
	register caddr_t cp = buf;
	char *epkt;

	buf += sizeof(struct ether_header);
	cp = buf;
	epkt = cp + totlen;

	if (off) {
		cp += off + 2 * sizeof(u_short);
		totlen -= 2 * sizeof(u_short);
	}

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return 0;
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
				return 0;
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
			/*
			 * Place initial small packet/header at end of mbuf.
			 */
			if (len < m->m_len) {
				if (top == 0 && len + max_linkhdr <= m->m_len)
					m->m_data += max_linkhdr;
				m->m_len = len;
			} else
				len = m->m_len;
		}
		bcopy(cp, mtod(m, caddr_t), (unsigned)len);
		cp += len;
		*mp = m;
		mp = &m->m_next;
		totlen -= len;
		if (cp == epkt)
			cp = buf;
	}
	return top;
}

/*
 * Process an ioctl request.
 */
int
reioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct re_softc *sc = recd.cd_devs[ifp->if_unit];
	int s = splimp(), error = 0;

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			reinit(ifp->if_unit);	/* before arpwhohas */
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *)ifp,
				&IA_SIN(ifa)->sin_addr);
			break;
#endif
#ifdef NS
		case AF_NS:
		{
			register struct ns_addr *ina =
				&(IA_SNS(ifa)->sns_addr);

			if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *)(sc->re_addr);
			else {
				/* 
				 * The manual says we can't change the address 
				 * while the receiver is armed,
				 * so reset everything
				 */
				ifp->if_flags &= ~IFF_RUNNING; 
				bcopy((caddr_t)ina->x_host.c_host,
					(caddr_t)sc->re_addr,
					sizeof(sc->re_addr));
			}
		}
			/*FALL THROUGH*/
#endif
		default:
			reinit(ifp->if_unit);
			break;
		}
		break;
	case SIOCSIFFLAGS:
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) == IFF_RUNNING) {
			ifp->if_flags &= ~IFF_RUNNING;
			restop(sc->re_base);
		} else {
			reinit(ifp->if_unit);
		}
		break;
	default:
		error = EINVAL;
	}
	splx(s);
	return error;
}

/*
 * Take interface offline.
 */
int
restop(base)
	caddr_t base;
{
	outb(base + RE_CTL1, 0xd2);
	outb(base + RE_CTL2, 0x00);

	return 0;
}
