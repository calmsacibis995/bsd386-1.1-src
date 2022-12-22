/*	BSDI $Id: if_tn.c,v 1.3 1994/01/12 18:02:31 karels Exp $	*/

/*
 * TNIC-1500 (Am79C960 "PCnet-ISA") Ethernet driver.
 * Copyright 1993 South Coast Computing Services, Inc.
 * All rights reserved.  License is granted to use only
 * in the following circumstances:
 *	1) with cards purchased from South Coast
 *   or	2) with a licensed copy of BSD/386, version 1.1 or higher
 *   or 3) for personal, noncommercial use.
 *
 * 1. Redistributions of source code must retain this entire banner
 *    setting forth the original license terms without modification.
 * 2. Redistribution in binary form is permitted ONLY when accompanied
 *    by source code.
 * 3. If you modify the code you must add a banner describing the
 *    modifications and identifying yourself as the party responsible.
 * 4. No permission to use the name of South Coast Computing Services,
 *    except as required by item one, is implied by this license.
 *
 *	South Coast Computing Services, Inc.
 *	P.O. Box 270355
 *	Houston, TX  77277-0355  U.S.A
 *	+1 713 661 3301
 *	+1 713 661 0633 FAX
 *
 * THIS SOFTWARE IS PROVIDED BY SOUTH COAST AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL SOUTH COAST OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *  TNIC-1500 family ethernet cards from South Coast Computing Services.
 *  (Manufactured by Transition Engineering, using AMD's PCnet-ISA chip.)
 *
 *	See ic/am79c960.h for a running commentary on the chip's
 *	software interface.  If_tnreg.h contains some TNIC-specific
 *	information plus tunable parameters.  Please refer to the
 *	PCnet-ISA data sheet for more intimate information about the chip.
 *
 *	The chip manages mailboxes through bus-master DMA much
 *	like the Adaptec SCSI controller.  Once the chip is
 *	initialized, very little interaction with the I/O space
 *	interface is required.
 *
 *	We have enabled automatic packet padding and pad stripping.
 *	(The latter includes FCS (CRC) stripping.)
 *
 *	The chip supports software selection of the medium for
 *	multi-media boards, but the TNIC-1500 variants all use
 *	hardware jumpers, so we don't have any support for the
 *	software selection in place yet.
 *
 *	Perhaps most significantly, we are running the device with
 *	the transmit-complete interrupt masked off.  With 128 entries
 *	in the transmit ring there is no great urgency to follow
 *	along right behind the chip.  So we scavenge used mbufs and
 *	recondition used Tx ring slots opportunistically in tnstart,
 *	avoiding considerable interrupt overhead.  The transmit
 *	interrupt is enabled if the transmit ring size is set below
 *	eight.
 *
 *	A polled mode of operation is available as well.  If the
 *	interrupt jumper is removed (and the config file line
 *	specifies irq ?) the driver will poll the interface HZ
 *	times per second.  Using polled mode greatly decreases
 *	interrupt response overhead at the expense of increased
 *	latency.  When the overall system is not capable of handling
 *	sustained high packet rates, polled mode has the beneficial
 *	property of dropping excess packets without spending any
 *	CPU time on them at all.
 *
 *	12/14/93 added support for memory > 16M.  This requires
 *	keeping the ring buffer full when the interface is not
 *	in use so we can hang onto as many mbufs < 16M as possible.
 *	Receive ring changed to headless clusters instead of clustered
 *	mbufs.  Only one packet at a time may use the Tx bounce buffer.
 *	That may cause a serious slowdown in polled mode.  We may need
 *	to reenable the Tx interrupt when not in polled mode to support
 *	bouncing well.  Either that or allocate more than one buffer.
 */

/*
 * vix 11jan93 [added primative multicast support]
 * vix 11jan93 [tried to solve a transmit performance problem]
 * vix 10jan93 [received from steve; converted to BSD style]
 */

#define TN_RBUFSIZ	(MCLBYTES-2)	/* HACK for NFS packet alignment */
#define	ETHER_FRAMESIZE	1536

#include "bpfilter.h"

#include "param.h"
#include "mbuf.h"
#include "socket.h"
#include "device.h"
#include "malloc.h"
#include "vm/vm.h"
#include "vm/pmap.h"
#include "ioctl.h"
#include "errno.h"
#include "syslog.h"

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

#include "ic/am79c960.h"
#include "if_tnreg.h"
#include "isa.h"
#include "isavar.h"
#include "icu.h"

#include "machine/cpu.h"

#if NBPFILTER > 0
#include "net/bpf.h"
#include "net/bpfdesc.h"
#endif

/*
 * If you configure with option TNDEBUG or #define TNDEBUG here
 * you will get some debugging code that can be activated by saying
 * "ifconfig tn0 debug" and deactivated with "ifconfig tn0 -debug".
 *
 * BE FOREWARNED that it puts out a whole lot of trace information
 * using kernel printf's.  Needs some more work, may get it if another
 * bug comes up.
 */

#ifdef TNDEBUG
static int tndebug = 0;
#define dprintf(x) {if (tndebug) printf x;}
#else
#define dprintf(x)
#endif

/*
 *  Certain timing coincidences which happen in normal operation when
 *  two or more TNIC-1500s are installed cause error messages to be logged.
 *  Since these may be normal we use LINK2 to disable them.
 */

#define iflog(p,a) {if (!(p->tn_if.if_flags & IFF_LINK2)) log a;}

/*
 *  We keep track of the maximum number of ring slots used (per ring)
 *  to assist the user in selecting good ring sizes.
 */

int tn_txhiwat, tn_rxhiwat;

/*
 * The xx_softc structure is a conventional mechanism for representing
 * the state of an if_xx interface.  The structure gathers together
 * all of the bits of information and hooks required by higher and
 * lower levels of the kernel plus the bits of state needed to govern
 * the operation of the driver itself.
 */

struct tn_softc {
	/*
	 * common stuff first
	 */
	struct	device tn_dev;		/* base device */
	struct	isadev tn_id;		/* ISA device */
	struct	intrhand tn_ih;		/* interrupt vectoring */
	struct	arpcom tn_ac;		/* Ethernet common part */
#define	tn_if	tn_ac.ac_if		/* network-visible interface */
#define	tn_addr	tn_ac.ac_enaddr		/* hardware Ethernet address */
#if NBPFILTER > 0
	caddr_t	tn_bpf;			/* packet filter */
#endif

	/*
	 * TNIC-specific stuff from here down
	 */

	u_short	tn_base;	/* location of board in I/O space */
	u_short	tn_irq;		/* IRQ jumper of this board */
	int	tn_flags;	/* from the kernel config file */
	int	tn_oiflags;	/* old if_flags, for diffing */
	int	tn_lflags;	/* local flags */
#define	TNLF_POLLING	0x0001
#define	TNLF_POLLING_OA	0x0002

	/*
	 * Tn_ib, tn_rb, and tn_tb are allocated from from a single page of
	 * physical memory to guarantee contiguity.
	 *
	 * The code that dynamically maintains the rings lives in tnstart
	 * (tx ring) and tnintr (rx ring). Look there to acquire an
	 * understanding of the queueing invariants governing these
	 * variables.
	 */

	struct	tn_iblock *tn_ib;	/* initialization block */
	struct	tn_rblock *tn_rb;	/* receive ring */
	char	*tn_rmcl[TN_NRRING];	/* headless clusters for Rx ring*/
	int	tn_rhead;		/* read queue head */
	struct	tn_tblock *tn_tb;	/* transmit ring */
	struct	mbuf *tn_tmbuf[TN_NTRING]; /* mbufs in Tx ring */
					/* ...null when bounced! */
	int	tn_ttail;		/* tail (insertion point) of Tx q */
	int	tn_tqlen;		/* number of Tx buffers occupied */
	struct	mbuf *tn_txwaiting;	/* packet waiting for tx room */
					/* ...or waiting for bounce buffer */

	/*
	 * transmit bounce buffer -- we assume the softc struct
	 * is allocated in contiguous physical memory below 16M.
	 */

	char	tn_txbounce[ETHER_FRAMESIZE];
	int	tn_txbouncecnt;		/* number of mailboxes using buffer */
					/* ...(all in one packet right now */
};

int	tnprobe __P((struct device *, struct cfdata *, void *));
void	tnattach __P((struct device *, struct device *, void *));
int	tnintr __P((struct tn_softc *));
int	tnwatchdog __P((int));
int	tninit __P((int));
int	tnioctl __P((struct ifnet *, int, caddr_t));
int	tnstart __P((struct ifnet *));

void	tn_poll __P((struct tn_softc *sc));
void	tn_poll_oactive __P((struct tn_softc *sc));
int	tn_work __P((struct tn_softc *sc, int wantdebug));
int	tn_transmit __P((struct tn_softc *sc));

void	tnstop __P((struct tn_softc *));

void	tn_txscavenge __P((struct tn_softc *sc));
int	tn_outpacket __P((struct tn_softc *sc, struct mbuf *m0));
void	tn_rxscavenge __P((struct tn_softc *sc));
struct	mbuf *tn_rxgetone __P((struct tn_softc *sc));
void	tn_erroranalysis __P((struct tn_softc *sc, u_int csr));
void	tnforceintr __P((void *aux));

struct	cfdriver tncd =
	{ NULL, "tn", tnprobe, tnattach, sizeof(struct tn_softc) };

/*
 * Probe the Am79C960 to see if it's there.  Return 1 if it is,
 * zero if not.  If it looks like it might be there but disappoints
 * us we may squawk, but generally we must be careful and silent.
 *
 * Because we have no way of discovering the DRQ jumper strapping,
 * the config file entry must be right.  When configuring two or
 * more TNIC-1500 cards pay close attention to the order in which
 * the probes are called out -- we want to find the right address/DRQ
 * combinations without coming across the wrong ones.
 */

tnprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	u_short w;
	int i, base = ia->ia_iobase;
	caddr_t p, b;

#ifdef TNDEBUG
	if (sizeof(struct tn_iblock) != 24 ||
	    sizeof(struct tn_rblock) != 8 ||
	    sizeof(struct tn_tblock) != 8) {
		printf("TN header/compiler crocked!\n");
		return (0); /* if we panic it will just make fixing it hard. */
	}
#endif

	/* see if the address PROM is there */
	if ((inw(base+TNIO_APROM) & 0x0000ffff) == 0x0000ffff) {
		/* "physical" ethernet addresses can't start with ff */
		return (0);
	}

	/* Something is there -- let's see if it looks like an AMD chip. */
	inw(base+TNIO_RESET);
	outw(base+TNIO_RAP, TNRA_CSR);
	w = inw(base+TNIO_RDP);
	if (w != TNCSR_STOP) {
		dprintf(("TN csr0 looks like %x\n", w));
		/* still not sure we have the right board -- let it slide. */
		return (0);
	}
	outw(base+TNIO_RAP, TNRA_IM);
	if (inw(base+TNIO_RDP) == w) {
		/* This is too weird -- squawk. */
		printf("TN RAP not working, probably not a TNIC card\n");
		return (0);
	}

	/* Probably an Amd chip, then.  Discover irq setting. */
	if (ia->ia_irq == IRQUNK) {
		i = isa_discoverintr(tnforceintr, aux);
		if (i > 1)
			ia->ia_irq = i;
		else
			ia->ia_irq = 0;
	}
#ifdef NOTNPOLLING
	if (!ia->ia_irq || ia->ia_irq == IRQUNK) {
		printf("TN no irq\n");
		return (0);
	}
#endif

	/* Leave the board reset -- attach does the major initialization. */
	inw(base+TNIO_RESET);
	ia->ia_iosize = TN_NREG;
	return (1);
}


/*
 * Force an interrupt for autoconfiguration (called indirectly from
 * within tnprobe).
 */

void 
tnforceintr(aux)
	void *aux;
{
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int base = ia->ia_iobase;

	/*
	 * The DMA controller channel should not be configured yet, so
	 * attempting to initialize the chip should cause a memory error,
	 * which should cause an interrupt.
	 */

	inw(base+TNIO_RESET);
	outw(base+TNIO_RAP, TNRA_CSR);
	outw(base+TNIO_RDP, TNCSR_IENA | TNCSR_INIT);
}

/*
 * Note: the documentation on the chip implies that you can
 *	initialize it once, then stop and start it at will
 *	for minor reconfiguration and status reporting.  According
 *	to Amd tech support, this is not a good idea.  They recommend
 *	reinitializing the chip after stopping it for any reason.
 */

/*
 * Tnattach is run after probe completes.  It does the major
 * work of initializing the softc structure, registering the
 * device with higher and lower kernel layers, and cofiguring
 * the hardware.  The device initialization happens for real
 * in tninit when ifconfig calls tnioctl to UP the interface.
 *
 * Tnattach leaves the device partially initialized.  In particular,
 * the Rx ring clusters are allocated here prior to ifconfig up so that
 * we can be pretty sure we'll get low memory.
 */

void 
tnattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct tn_softc *sc = (struct tn_softc *) self;
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct ifnet *ifp = &sc->tn_if;
	register int base = ia->ia_iobase;
	u_short ea[3];
	int i;
	vm_offset_t pa;
	caddr_t va;

	/*
	 * Misc initializations
	 */
	sc->tn_base = base;
	sc->tn_irq = ia->ia_irq;
	sc->tn_flags = sc->tn_dev.dv_flags;
	sc->tn_lflags = 0;
	sc->tn_txwaiting = 0;

	/*
	 * Copy the ethernet address from the PROM into the softc struct.
	 */
	ea[0] = inw(base+TNIO_APROM + 0);
	ea[1] = inw(base+TNIO_APROM + 2);
	ea[2] = inw(base+TNIO_APROM + 4);
	bcopy(ea, sc->tn_addr, 6);

	/*
	 * Allocate a page for the descriptor rings (must be contiguous),
	 * then carve it up into an iblock and the two descriptor rings.
	 */
	MALLOC(va, caddr_t, NBPG, M_DEVBUF, M_NOWAIT);
	sc->tn_ib = (void *) va;
	va += sizeof(struct tn_iblock);

	/* recall that rings must be eight-byte alligned */
	while ((u_long) va & 7)
		va++;
	sc->tn_rb = (void *) va;
	va += TN_NRRING * sizeof(struct tn_rblock);

	while ((u_long) va & 7)
		va++;
	sc->tn_tb = (void *) va;
	va += TN_NTRING * sizeof(struct tn_rblock);

	/* see if we somehow crossed a page boundary */
	if (((u_long) va & PG_FRAME) != ((u_long) sc->tn_ib & PG_FRAME))
		panic("TN memory allocation");

	/*
	 * Initialize the initialization block.
	 */
	sc->tn_ib->mode = 0;
	sc->tn_ib->rdra = pmap_extract(pmap_kernel(), (vm_offset_t) sc->tn_rb);
	sc->tn_ib->rlen = TN_RRING_CODE;
	sc->tn_ib->tdra = pmap_extract(pmap_kernel(), (vm_offset_t) sc->tn_tb);
	sc->tn_ib->tlen = TN_TRING_CODE;

	/*
	 * Establish ring invariants -- Tx empty, Rx primed.
	 * Rx is primed in two passes so we don't free and reallocate
	 * the same high-mem cluster over and over again.  Once
	 * the low mem clusters are allocated we never allow the
	 * number of them that we hold to decrease.
	 */
	for (i = 0; i < TN_NTRING; i++) {
		sc->tn_tmbuf[i] = 0;
		sc->tn_tb[i].flags = 0;
	}
	for (i = 0; i < TN_NRRING; i++) {
		MCLALLOC(sc->tn_rmcl[i], M_DONTWAIT);
		sc->tn_rb[i].flags = 0;
	}
	for (i = 0; i < TN_NRRING; i++) {
		pa = pmap_extract(pmap_kernel(), (vm_offset_t) sc->tn_rmcl[i]);
		if (pa & ~0x00ffffff) {
			MCLFREE(sc->tn_rmcl[i]);
			sc->tn_rmcl[i] = 0;
			log(LOG_WARNING, "tn%d: cluster in high memory\n", 
			    sc->tn_dev.dv_unit);
		}
	}

	/* Enable DMA channel (technique from i386/isa/aha.c). */
	at_dma_cascade(ia->ia_drq);

	/*
	 * Initialize the hardware.
	 */

	/* reset it */
	inw(base+TNIO_RESET);

	/* load iblock physical address */
	pa = pmap_extract(pmap_kernel(), (vm_offset_t) sc->tn_ib);
	outw(base+TNIO_RAP, TNRA_IALOW);
	outw(base+TNIO_RDP, pa);
	outw(base+TNIO_RAP, TNRA_IAHI);
	outw(base+TNIO_RDP, pa >> 16);

	/*
	 * Register the device with the protocol stack.
	 */
	ifp->if_unit = sc->tn_dev.dv_unit;
	ifp->if_name = "tn";
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX |
#ifdef MULTICAST
			IFF_MULTICAST |
#endif
			IFF_NOTRAILERS;
	ifp->if_init = tninit;
	ifp->if_output = ether_output;
	ifp->if_start = tnstart;
	ifp->if_ioctl = tnioctl;
	ifp->if_watchdog = tnwatchdog;
	if_attach(ifp);
#if NBPFILTER > 0
	bpfattach(&sc->tn_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	/* Print banner (probe has printed the first line without a \\n). */

	printf("\ntn%d: address %s\n",
	       sc->tn_dev.dv_unit, ether_sprintf(sc->tn_addr));

	/*
	 * Register with lower levels of kernel. 
	 */
	isa_establish(&sc->tn_id, &sc->tn_dev);
	if (ia->ia_irq) {
		sc->tn_ih.ih_fun = tnintr;
		sc->tn_ih.ih_arg = (void *) sc;
		intr_establish(ia->ia_irq, &sc->tn_ih, DV_NET);
	}

	/*
	 * This completes the boot-time initialization. We pick up again in
	 * tninit when ifconfig is run.
	 */
}

/*
 * tnstop: Take interface offline.  Because many of the control
 * bits in on-chip registers other than CSR0 are accessible
 * only when the chip is "stopped", this gets used in preparation
 * for a re-init from ioctl as well as for "ifconfig tn0 down".
 *
 * We recycle mbufs bound to the rings, recording an error for
 * any undelivered packets.  This allows us to start fresh in tninit.
 */

void 
tnstop(sc)
	struct tn_softc *sc;
{
	int i, s;

	s = splimp();

	if (sc->tn_lflags & TNLF_POLLING) {
		untimeout(tn_poll, sc);
		sc->tn_lflags &= ~TNLF_POLLING;
	}
	if (sc->tn_lflags & TNLF_POLLING_OA) {
		untimeout(tn_poll_oactive, sc);
		sc->tn_lflags &= ~TNLF_POLLING_OA;
	}

	inw(sc->tn_base+TNIO_RESET);
	sc->tn_if.if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	sc->tn_if.if_timer = 0;

	/* handle any completed packets, in or out */
	tn_txscavenge(sc);
	tn_rxscavenge(sc);

	/* drop any pending output packets */
	for (i = 0; i < TN_NTRING; i++) {
		if (sc->tn_tmbuf[i]) {
			m_free(sc->tn_tmbuf[i]);
			sc->tn_tmbuf[i] = 0;
			if (sc->tn_tb[i].flags & TNTXF_ENP)
				sc->tn_if.if_oerrors++;
		}
		sc->tn_tb[i].flags = 0;
	}

	/* record partial packets and clear flags, but keep clusters */
	for (i = 0; i < TN_NRRING; i++) {
		if (sc->tn_rb[i].flags & TNRB_ENP) {
			/* half-received */
			sc->tn_if.if_iqdrops++;
		}
		sc->tn_rb[i].flags = 0;
	}

	splx(s);
	dprintf(("tn%d: stopped\n", sc->tn_dev.dv_unit));
}

/*
 * Tnwatchdog gets called if the board's output queue fills
 * up, causing IFF_OACTIVE to get set, then fails to drain
 * for some reason.  This is a serious situation.
 *
 * The call to watchdog is mediated by the if_timer, which
 * is set by tnstart and decremented (if positive) by lbolt
 * once a second.  If it is ever decremented to zero, watchdog
 * gets kicked.
 *
 * There's an outside chance this could happen if the tx ring
 * is very small and the traffic is very light simply because
 * one packet fills the ring and we don't run tnstart again
 * before the timer expires.  So we try running tnstart ourselves,
 * then if IFF_OACTIVE is still set we've got big trouble.
 *
 * Since it isn't clear what might cause this condition, it
 * isn't clear what we should do to correct it.  So we just
 * stop and restart the interface, hoping that will help.
 */

int
tnwatchdog(unit)
	int unit;
{
	struct tn_softc *sc = tncd.cd_devs[unit];
	struct ifnet *ifp = &sc->tn_if;
	int s, a, b;

	s = splimp();
	b = sc->tn_ttail;
	tn_output(sc);
	a = sc->tn_ttail;
	splx(s);
	if (sc->tn_tqlen == 0 || a != b)
		return;		/* we made progress */

	iflog(sc, (LOG_WARNING, "tn%d: timed out\n", unit));
#ifdef TNDEBUG
	tn_dump(sc);
#endif
	tnstop(sc);
	tninit(unit);
	return (0);
}

/*
 * Tninit applies any new information in the interface structure
 * to the hardware configuration and (re)initializes the board.
 * The interface must be stopped (tnstop) when tninit is called.
 */

int
tninit(unit)
	int unit;
{
	struct tn_softc *sc = tncd.cd_devs[unit];
	struct ifnet *ifp = &sc->tn_if;
	int base = sc->tn_base;
	int i, s;
	u_short us;
	vm_offset_t pa;

	dprintf(("TN init called\n"));
	if (ifp->if_flags & IFF_RUNNING)
		printf("tn%d: init called on running interface\n", unit);

	s = splimp();

	/* set test/feature control mode register */
	outw(base+TNIO_RAP, TNRA_TF);
	outw(base+TNIO_RDP, TNTF_APADX | TNTF_ASTRP | TNTF_DPOLL |
#ifdef TNTIMER
			    TNTF_TIMER | TNTF_DMAPLUS |
#endif
			    TNTF_MPCOM | TNTF_RCVCCOM | TNTF_TXSTRTM);

	/* Set up the FIFO control register */
	outw(base+TNIO_RAP, TNRA_FIFO);
	outw(base+TNIO_RDP, TNFI_RX32 | TNFI_TXS112 | TNFI_TXW16 | 40);

#ifdef TNTIMER
	/* Set the bus on-time timer (100 ns units) (gated by TNTF_TIMER) */
	outw(base+TNIO_RAP, TNRA_TIMER);
	outw(base+TNIO_RDP, 150);
#endif

	/* ISA configuration words from device config flags -- see man page */
	if (sc->tn_flags & 0xf0000000) {
		outw(base+TNIO_RAP, TNIRA_MSRDA);
		outw(base+TNIO_IDP, (sc->tn_flags >> 28) & 15);
	}
	if (sc->tn_flags & 0x0f000000) {
		outw(base+TNIO_RAP, TNIRA_MSWRA);
		outw(base+TNIO_IDP, (sc->tn_flags >> 24) & 15);
	}
	if (sc->tn_flags & 0x000000ff) {
		outw(base+TNIO_RAP, TNIRA_LED(1));
		outw(base+TNIO_IDP, (sc->tn_flags >> 0) & 0xff);
	}
	if (sc->tn_flags & 0x0000ff00) {
		outw(base+TNIO_RAP, TNIRA_LED(2));
		outw(base+TNIO_IDP, (sc->tn_flags >> 8) & 0xff);
	}
	if (sc->tn_flags & 0x00ff0000) {
		outw(base+TNIO_RAP, TNIRA_LED(3));
		outw(base+TNIO_IDP, (sc->tn_flags >> 16) & 0xff);
	}

	/*
	 * Initialize the Tx ring. 
	 */

	/* Mbufs will be bound to slots as output happens -- see tnstart. */
	sc->tn_tqlen = sc->tn_ttail = 0;
	sc->tn_txbouncecnt = 0;
	for (i = 0; i < TN_NTRING; i++) {
		sc->tn_tmbuf[i] = 0;
		sc->tn_tb[i].flags = 0;
		sc->tn_tb[i].err = 0;
	}

	/* Reestablish starting conditions on Rx ring */
	sc->tn_rhead = 0;
	for (i = 0; i < TN_NRRING; i++) {
		if (sc->tn_rmcl[i]) {
			/* if it's not null, it's in low mem */
			pa = pmap_extract(pmap_kernel(),
					  (vm_offset_t) sc->tn_rmcl[i]);
			sc->tn_rb[i].addr = pa;
			sc->tn_rb[i].flags = TNRB_OWN;
			sc->tn_rb[i].nsize = -TN_RBUFSIZ;
			sc->tn_rb[i].bcnt = 0;
		} else
			sc->tn_rb[i].flags = 0;
	}

	/* Update ethernet physical address in iblock */
	bcopy(sc->tn_addr, sc->tn_ib->enaddr, 6);

	/* Update the promiscuity flag, whether it needs it or not. */
	if (ifp->if_flags & IFF_PROMISC)
		sc->tn_ib->mode |= TNMD_PROM;
	else
		sc->tn_ib->mode &= ~TNMD_PROM;

	/*
	 * XXX	it sure would be nice to set up laddr[], which is a 64-bit
	 *	multicast mask, so that the card could do first-order rejection
	 *	using its CRC-based address filter.  someday, perhaps.  for
	 *	now, we set all 64 bits if the interface is running in 
	 *	multicast mode.
	 */

#ifdef MULTICAST
	/* set up multicast filter */
	if ((sc->tn_ac.ac_multiaddrs != 0) ||
	    (ifp->if_flags & IFF_ALLMULTI))
		us = 0xffff;
	else
		us = 0x0000;
	for (i = 0; i < 4; i++)
		sc->tn_ib->laddr[i] = us;
#endif

	/* cause board to read the iblock */
	outw(base+TNIO_RAP, TNRA_CSR);
	outw(base+TNIO_RDP, TNCSR_INIT);

	/* wait for initialization to complete */
	for (i = 100000; i && !(inw(base+TNIO_RDP) & TNCSR_IDON); --i)
		;
	if (!i) {
		/* panic("TN init froze"); */
		outw(base+TNIO_RAP, TNRA_CSR);
		printf("tn%d: init froze! (csr=%x)\n",
		       sc->tn_dev.dv_unit, inw(base+TNIO_RDP));
		tnstop(sc);
		splx(s);
		return (0);
	}

	/* set stop mode and ack the IDON interrupt */
	outw(base+TNIO_RDP, TNCSR_IDON | TNCSR_STOP);

	/* belt-and-suspenders verification of the initialization */
	outw(base+TNIO_RAP, 12);
	i = inw(base+TNIO_RDP) & 0x0000ffff;
	if (i != *((u_short *) sc->tn_addr)) {
		/* panic("TN initialization didn't stick"); */
		outw(base+TNIO_RAP, TNRA_CSR);
		printf("tn%d: got %x after init, csr=%x\n", i,
		       sc->tn_dev.dv_unit, inw(base+TNIO_RDP));
		tnstop(sc);
		splx(s);
		return (0);
	}

	/* Here's our opportunity to set the interrupt mask register. */
	outw(base+TNIO_RAP, TNRA_IM);
	outw(base+TNIO_RDP,
#if TN_NTRING >= 8
			    TNIM_TINT |
#endif
			    TNIM_EMBA);

	/* Spin the prop and see if she catches. */
	if (sc->tn_irq) {
		outw(base+TNIO_RAP, TNRA_CSR);
		outw(base+TNIO_RDP, TNCSR_STRT | TNCSR_IENA);
	} else {
		outw(base+TNIO_RAP, TNRA_CSR);
		outw(base+TNIO_RDP, TNCSR_STRT);
		timeout(tn_poll, sc, 1);
		sc->tn_lflags |= TNLF_POLLING;
	}

	/* Mark the interface structure appropriately */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);
	return (0);
}


/*
 * Tnstart is called from the protocol stack when output packets
 * are queued and the interface isn't busy (as indicated by the
 * IFF_OACTIVE flag).  We need to distinguish the call down from
 * the stacks from calls originating within this driver, so we
 * have move the real work into tn_output().
 *
 * Tnstart is called at splimp!  All other users of tn_output
 * must call it at splimp as well!
 */

int
tnstart(ifp)
	struct ifnet *ifp;
{
	struct tn_softc *sc = tncd.cd_devs[ifp->if_unit];

	/* service the output queue */
	tn_output(sc);

	/* if we're in polled mode, service input as well */
	if (!sc->tn_irq)
		tn_rxscavenge(sc);
}

/*
 * tn_output is responsible for servicing the output side of the
 * interface.  It is called via tnstart from the protocol stacks,
 * from tn_poll in polled mode operation, and from tnintr in interrupt
 * mode.  Must be called at splimp().
 */

int
tn_output(sc)
	struct tn_softc *sc;
{
	struct mbuf *m0, *m, *n;
	int b, f;
	vm_offset_t pa;

	/*
	 * If the transmitter has panicked, restart the chip.
	 */
	outw(sc->tn_base+TNIO_RAP, TNRA_CSR);
	if (!(inw(sc->tn_base+TNIO_RDP) & TNCSR_TXON)) {
		iflog(sc, (LOG_WARNING, "tn%d: transmitter disabled\n",
			   sc->tn_dev.dv_unit));
		/* can happen on BUFF and UFLO errors:  restart. */
		tnstop(sc);
		tninit(sc->tn_dev.dv_unit);
	}

	/*
	 * Clean out completed packets so we have plenty of room. 
	 */
	tn_txscavenge(sc);

	/*
	 * Transfer as many queued packets as possible into the
	 * transmission ring.  We use tn_txwaiting as a holding cell for a
	 * packet that we may have dequeued but then discovered that we
	 * didn't have enough room for it.
	 */
	for (;;) {
		/* Get a packet, by hook or by crook. */
		if (m0 = sc->tn_txwaiting)
			sc->tn_txwaiting = NULL;
		else {
			IF_DEQUEUE(&sc->tn_if.if_snd, m0);
			if (!m0)
				break;	/* no more work for us */
		}

		/* try to transmit it */
		if (!tn_outpacket(sc, m0)) {
			/* wouldn't fit -- save it for later */
			sc->tn_txwaiting = m0;
			break;
		}
	}

	/*
	 * On the way out, update the watchdog timer and the active flag.
	 * We set IFF_OACTIVE only if another immediate call to tnstart
	 * would be futile.
	 */

#if TN_NTRING >= 8
	if (sc->tn_tqlen > TN_NTRING - 4)
#else
	if (sc->tn_tqlen)
#endif
	{
		sc->tn_if.if_flags |= IFF_OACTIVE;
		if (!(sc->tn_lflags & TNLF_POLLING_OA))
			tn_poll_oactive(sc);
	} else {
		sc->tn_if.if_flags &= ~IFF_OACTIVE;
		if (sc->tn_lflags & TNLF_POLLING_OA) {
			untimeout(tn_poll_oactive, sc);
			sc->tn_lflags &= ~TNLF_POLLING_OA;
		}
	}

	sc->tn_if.if_timer = sc->tn_tqlen ? 2 : 0;
	return (0);
}


/*
 *	Tn_outpacket -- attempt to put a packet into the transmit ring.
 *
 *	Returns true if the packet has been accepted, false if it
 *	was not (e.g. ring full).  The packet may have been accepted
 *	but not transmitted if it was malformed.
 */

int 
tn_outpacket(sc, m0)
	struct tn_softc *sc;
	struct mbuf *m0;
{
	struct mbuf *m, *n;
	int b, bn, len, bouncy;
	char *bounceptr = sc->tn_txbounce;
	vm_offset_t pa;

	/* Count mbufs in our chain and check for bounces. */
	bouncy = len = 0;
	for (m = m0; m; m = m->m_next) {
		len++;
		pa = pmap_extract(pmap_kernel(), (vm_offset_t) m->m_data);
		if (pa & ~0x00ffffff)
			bouncy++;
	}

	/* If the packet is malformed or too fragmented, drop it */
	if (!(m0->m_flags & M_PKTHDR) ||
	    m0->m_pkthdr.len > ETHERMTU + (sizeof(struct ether_header)) ||
	    len > TN_NTRING) {
		printf("tn%d: dropping packet of %d bytes\n", sc->tn_dev.dv_unit, m0->m_pkthdr.len);
		m_freem(m0);
		sc->tn_if.if_oerrors++;
		return (1);
	}

	/*
	 * If there are not enough free Tx slots,
	 * or if we need to wait for the bounce buffer,
	 * tnintr (or tn_poll) will call us back.
	 */

	if (sc->tn_tqlen + len > TN_NTRING || (bouncy && sc->tn_txbouncecnt))
		return (0);

#if NBPFILTER > 0
	/* Let the bpf subsystem have a peek at our packet */
	if (sc->tn_bpf)
		bpf_mtap(sc->tn_bpf, m0);
#endif

	/*
	 * Bind the mbufs to ring slots, then give them to the chip. We
	 * flip the ownership bit on the first buffer last to prevent any
	 * possibility of confusing the chip.  If a bounce is necessary,
	 * copy the data out to the bounce buffer and free the source mbuf
	 * right away.  tn_tmbuf[?] is null in that case.
	 */

	b = sc->tn_ttail;
	for (m = m0; m; m = n) {
		if (!m->m_len) {
			n = m_free(m);
			continue;
		}

		/* sanity check */
		if (sc->tn_tmbuf[b])
			panic("TN tx mbuf leak");

		dprintf(("TN loading %d bytes into slot %d\n", m->m_len, b));

		/* Bind the mbuf to the ring slot. */
		sc->tn_tb[b].nbcnt = -(m->m_len);
		sc->tn_tb[b].err = 0;

		pa = pmap_extract(pmap_kernel(), (vm_offset_t) m->m_data);
		if (pa & ~0x00ffffff) {
			bcopy(m->m_data, bounceptr, m->m_len);
			pa = pmap_extract(pmap_kernel(),
					  (vm_offset_t) bounceptr);
			bounceptr += m->m_len;
			MFREE(m, n);
			sc->tn_txbouncecnt++;
			sc->tn_tmbuf[b] = 0;
		} else {
			sc->tn_tmbuf[b] = m;
			n = m->m_next;
		}
		sc->tn_tb[b].addr = pa;

		/* Unless it's the first mbuf, flip the OWN bit. */
		sc->tn_tb[b].flags = (m == m0) ? 0 : TNTXF_OWN;

		/* Step to next slot. */
		sc->tn_tqlen++;
		bn = b;
		b = (b + 1) & (TN_NTRING - 1);
	}

	/* Turn over (to the chip) the first buffer last. */
	sc->tn_tb[bn].flags |= TNTXF_ENP;
	sc->tn_tb[sc->tn_ttail].flags |= TNTXF_STP | TNTXF_OWN;

	/* update tn_ttail after we are done using it to track first slot */
	sc->tn_ttail = b;

	/* Tickle the transmit ring polling request bit. */
	outw(sc->tn_base+TNIO_RAP, TNRA_CSR);
	outw(sc->tn_base+TNIO_RDP, TNCSR_TDMD | TNCSR_IENA);

	if (bounceptr - sc->tn_txbounce > sizeof(sc->tn_txbounce))
		panic("bounce overflow\n");

	return (1);
}

/*
 * Tn_txscavenge walks the Tx ring, recycling mbufs from completed
 * packets and noting any error indications.
 */

void 
tn_txscavenge(sc)
	struct tn_softc *sc;
{
	int b, f;
	int unit = sc->tn_dev.dv_unit;
	struct mbuf *m;

	if (sc->tn_tqlen > tn_txhiwat)
		tn_txhiwat = sc->tn_tqlen;
	dprintf(("TN qlen = %d (pre)\n", sc->tn_tqlen));

	/* for every occupied slot that the cpu owns ... */
	for (b = (TN_NTRING + sc->tn_ttail - sc->tn_tqlen) & (TN_NTRING - 1);
	     sc->tn_tqlen && !(sc->tn_tb[b].flags & TNTXF_OWN);
	     b = (b + 1) & (TN_NTRING - 1)) {
		dprintf(("TN tint b = %d\n", b));

		/* act on the error and other flags */
		if (sc->tn_tb[b].err & TNTXE_BUFF) {
			/* not included in TNTXF_ERR */
			printf("tn%d: tx buffer error\n", unit);
		}
		f = sc->tn_tb[b].flags;
		if (f & TNTXF_ENP)
			sc->tn_if.if_opackets++;
		if (f & TNTXF_ERR) {
			if (sc->tn_tb[b].err & TNTXE_UFLO)
				printf("tn%d: Tx underflow\n", unit);
			if (sc->tn_tb[b].err & TNTXE_LCOL)
				printf("tn%d: Late collision\n", unit);
			if (sc->tn_tb[b].err & TNTXE_LCAR)
				printf("tn%d: Loss of Carrier\n", unit);
			/* if (sc->tn_tb[b].err & TNTXE_RTRY) */

			sc->tn_if.if_oerrors++;
		}
		if (f & TNTXF_ONE)
			sc->tn_if.if_collisions += 1;
		if (f & TNTXF_MORE)
			sc->tn_if.if_collisions += 4;	/* use counter??? */

		/* recycle the mbuf or reclaim the bounce buffer */
		if (sc->tn_tmbuf[b]) {
			MFREE(sc->tn_tmbuf[b], m);
			sc->tn_tmbuf[b] = 0;
		} else if (--sc->tn_txbouncecnt < 0)
			panic("TN mbuf lost");

		/* and decrement the number of outstanding tx buffers */
		sc->tn_tqlen--;
	}

	dprintf(("TN qlen = %d (post)\n", sc->tn_tqlen));
}

/*
 *	tn_erroranalysis -- analyse errors from a CSR0 value and update
 *	the appropriate counters.
 */

void 
tn_erroranalysis(sc, csr)
	struct tn_softc *sc;
	u_int csr;
{
	/*
	 * If the master error flag is set, do the case analysis and record
	 * the error.
	 */

	if (csr & TNCSR_ERR) {
		if (csr & TNCSR_MISS) {
			sc->tn_if.if_iqdrops++;
		}
		if (csr & TNCSR_MERR) {
			/* Note: MERR may be normal when floppy is in use. */
			iflog(sc, (LOG_WARNING, "tn%d: memory error\n",
				   sc->tn_dev.dv_unit));
		}
		if (csr & TNCSR_BABL) {
			outw(sc->tn_base+TNIO_RAP, TNRA_CSR);
			outw(sc->tn_base+TNIO_RDP, TNCSR_STOP);
			printf("tn%d: halted on error %x\n",
			       sc->tn_dev.dv_unit, csr);
			return;
		}
		if (csr & TNCSR_CERR) {
			printf("tn%d: SQE test fail\n", sc->tn_dev.dv_unit);
		}
	}
}

/*
 *	tn_rxscavenge -- pull packets from the Rx ring
 *	(using tn_rxgetone) and pass them up the line.
 */

void
tn_rxscavenge(sc)
	struct tn_softc *sc;
{
	struct mbuf *m0, *m, **mpp, *n;
	struct ether_header *eh;
	int tlen;

	while (m0 = tn_rxgetone(sc)) {
		sc->tn_if.if_ipackets++;

#if NBPFILTER > 0
		if (sc->tn_bpf) {
			/* let the filter see the packet ... */

			bpf_mtap(sc->tn_bpf, m0);

			/* ... and if nobody else wants it, drop it */
			eh = (struct ether_header *) m0->m_data;
			if ((sc->tn_if.if_flags & IFF_PROMISC) &&
			    bcmp(eh->ether_dhost, sc->tn_addr, 6) &&
#ifdef MULTICAST
			    !ETHER_IS_MULTICAST(eh->ether_dhost)
#else
			    bcmp(eh->ether_dhost, etherbroadcastaddr, 6)
#endif
			    ) {
				m_freem(m0);
				continue;
			}
		}
#endif

		dprintf(("TN got good packet, tlen = %d\n", m0->m_pkthdr.len));

		/* Remove the ethernet header */
		eh = (struct ether_header *) m0->m_data;
		m_adj(m0, sizeof(struct ether_header));
		eh->ether_type = ntohs((u_short) eh->ether_type);

		/* TRAILER code -- untested. */

		if (eh->ether_type >= ETHERTYPE_TRAIL &&
		    eh->ether_type < ETHERTYPE_TRAIL + ETHERTYPE_NTRAILER) {
			u_short thdr[2];
			int off, resid;

			dprintf(("TN doing TRAILER\n"));

			off = (eh->ether_type - ETHERTYPE_TRAIL) * 512;
			if (off > m0->m_pkthdr.len) {
				m_freem(m0);
				continue;
			}
			m_copydata(m0, off, sizeof(thdr), thdr);
			eh->ether_type = ntohs(thdr[0]);
			resid = ntohs(thdr[1]);
			if (off + resid > m0->m_pkthdr.len ||
			    off + resid <= 0) {
				m_freem(m0);
				continue;
			}
			m0->m_pkthdr.len = off + resid;
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m && resid > MHLEN) {
				MCLGET(m, M_DONTWAIT);
				if (!(m->m_flags & M_EXT)) {
					m_free(m);
					m = 0;
				}
			}
			if (!m) {
				m_freem(m0);
				continue;
			}
			m->m_pkthdr = m0->m_pkthdr;
			m0->m_flags &= ~M_PKTHDR;
			m_copydata(m0, off + sizeof(thdr), resid, m->m_data);
			m->m_len = resid;
			m_adj(m0, -(resid + sizeof(thdr)));
			m->m_next = m0;
			m0 = m;
		}
		
		/* END OF TRAILER CODE */

		/* Pass the packet up the stack */
		ether_input(&sc->tn_if, eh, m0);
	}
}

struct mbuf *
tn_rxgetone(sc)
	struct tn_softc *sc;
{
	struct mbuf *m0, *m, **mpp, *n;
	int ok, b0, bN, b, tlen, f;
	struct ether_header *eh;
	char *temp_ptr;
	vm_offset_t pa;

	/* b tracks next ring slot to check */
	b = sc->tn_rhead;

	/* Loop until we get a good packet or run out of mailboxes */
	for (;;) {
		/* b0 remembers the beginning of the candidate packet */
		b0 = b;
		ok = 1;

		/* f accumulates error flags */
		f = sc->tn_rb[b].flags;
		if (!sc->tn_rmcl[b] || (f & TNRB_OWN)) {
			/* not ready for us yet */
			return 0;
		}

		if (!(f & TNRB_STP)) {
			/* non-packet mailbox */
			b = (b + 1) & (TN_NRRING - 1);
			ok = 0;
			goto error_handler;
		}

		/*
		 * Scan the mailboxes to see if we have a full packet worth
		 * of error-free buffers.  Gather error flags as we go.
		 */

		do {
			/* accumulate flags */
			f |= sc->tn_rb[b].flags;
			if (!sc->tn_rmcl[b] || (f & TNRB_OWN)) {
				/*
				 * not finished -- catch complete packet on
				 * the next call.
				 */
				return 0;
			}

			/* bcnt is only valid in the last slot */
			tlen = sc->tn_rb[b].bcnt;

			/* step ... loop ends on error or at end of packet */
			b = (b + 1) & (TN_NRRING - 1);
		} while (!(f & (TNRB_ENP | TNRB_ERR)));

error_handler:
		/* Handle any errors reported in the mailboxes */
		if ((f & (TNRB_STP|TNRB_ENP)) != (TNRB_STP|TNRB_ENP))
			ok = 0;

		if (f & TNRB_ERR) {
			/*
			 * Amd's documentation is not completely clear about
			 * how errors are reported.  This code may be wrong!
			 */

			if ((f & (TNRB_FRAM | TNRB_ENP | TNRB_OFLO)) ==
			    (TNRB_FRAM | TNRB_ENP)) {
				printf("tn%d: framing error\n",
				       sc->tn_dev.dv_unit);
				sc->tn_if.if_ierrors++;
				/* data may be ok anyway */
			}
			if ((f & (TNRB_OFLO | TNRB_ENP)) == TNRB_OFLO) {
				iflog(sc, (LOG_WARNING,
					   "tn%d: overflow error\n",
					   sc->tn_dev.dv_unit));
				sc->tn_if.if_ierrors++;
				/* data not usable */
				ok = 0;
			}
			if ((f & (TNRB_CRC | TNRB_ENP | TNRB_OFLO)) ==
			    (TNRB_FRAM | TNRB_ENP)) {
				printf("tn%d: CRC error\n",
				       sc->tn_dev.dv_unit);
				sc->tn_if.if_ierrors++;
				/*
				 * data not reliable, but let the curious peek
				 */
				if (!(sc->tn_if.if_flags & IFF_PROMISC)) {
					ok = 0;
				}
			}
			if (f & TNRB_BUFF) {
				printf("tn%d: input buffer error\n",
				       sc->tn_dev.dv_unit);
				sc->tn_if.if_ierrors++;
				/* data not usable */
				ok = 0;
			}
		}

		/*
		 * Quick review --- at this point, b0 is the base slot
		 * of the candidate packet, b is one past the last slot
		 * of the candidate packet.  "ok" is a predicate that
		 * indicates whether or not we want to turn the packet
		 * into an mbuf chain and return it.  In any case, we've
		 * handled the error reporting requirements at this time.
		 *
		 * If the packet is OK we want to break out of the giant
		 * loop and enter the one-time code that turns it into
		 * an mbuf chain.  Otherwise we reset the used mailboxes
		 * and loop back to the top, hoping to find another
		 * candidate in the ring.
		 */

		if (ok)
			break;

		/* otherwise reset the mailboxes */
		while (b0 != b) {
			if (sc->tn_rmcl[b0]) {
				/* if it's not null, it's in low mem */
				pa = pmap_extract(pmap_kernel(),
						(vm_offset_t) sc->tn_rmcl[b0]);
				sc->tn_rb[b0].addr = pa;
				sc->tn_rb[b0].flags = TNRB_OWN;
				sc->tn_rb[b0].nsize = -TN_RBUFSIZ;
				sc->tn_rb[b0].bcnt = 0;
			} else
				sc->tn_rb[b0].flags = 0;
			b0 = (b0 + 1) & (TN_NRRING - 1);
		}

		/* update tn_rhead so the next invokation starts right */
		sc->tn_rhead = b0;
	}

	/*
	 * The following code just KNOWS that there are never more
	 * than two clusters per packet.  needs more work.
	 */

	/* whatever else happens, we will need a PKTHDR mbuf */
	MGETHDR(m0, M_DONTWAIT, MT_DATA);
	if (!m0) {
		/* XXX -- this case needs some more thought */
		return 0;
	}
	m0->m_pkthdr.rcvif = &sc->tn_if;
	m0->m_pkthdr.len = tlen;

	/*
	 * Four cases obtain here, depending on the packet length:
	 * 1) it may fit in the mbuf we just allocated.
	 * 2) it may fit in that one plus a single m_next.
	 * 3) it may fit in in one cluster plus a caboose.
	 * 4) otherwise it will require two clusters.
	 *
	 * In the cases involving clusters we will attempt to
	 * allocate replacements for the clusters which already
	 * hold the data.  Failing that, we will copy the data out.
	 */

	if (tlen <= MHLEN+MLEN) {
		/* m0 gets non-cluster copy */
		m0->m_len = tlen;
		if (tlen > MHLEN)
			m0->m_len = MHLEN;
		tlen -= m0->m_len;
		bcopy(sc->tn_rmcl[b0], m0->m_data, m0->m_len);
		temp_ptr = sc->tn_rmcl[b0] + m0->m_len;
	} else {
		/* at least the first buffer goes in a cluster */
		MCLGET(m0, M_DONTWAIT);
		if (!(m0->m_flags & M_EXT)) {
			m_free(m0);
			return 0;
		}
		m0->m_len = tlen;
		if (tlen > TN_RBUFSIZ)
			m0->m_len = TN_RBUFSIZ;
		tlen -= m0->m_len;

		pa = pmap_extract(pmap_kernel(), (vm_offset_t) m0->m_data);
		if (pa & ~0x00ffffff) {
			/* fresh cluster is in high RAM, must copy */
			bcopy(sc->tn_rmcl[b0], m0->m_data, m0->m_len);
		} else {
			m0->m_ext.ext_buf = sc->tn_rmcl[b0];
			sc->tn_rmcl[b0] = m0->m_data;
			m0->m_data = m0->m_ext.ext_buf;
		}

		if (tlen) {
			/* done with first buffer */
			pa = pmap_extract(pmap_kernel(),
					  (vm_offset_t) sc->tn_rmcl[b0]);
			sc->tn_rb[b0].addr = pa;
			sc->tn_rb[b0].flags = TNRB_OWN;
			sc->tn_rb[b0].nsize = -TN_RBUFSIZ;
			sc->tn_rb[b0].bcnt = 0;
			b0 = (b0 + 1) & (TN_NRRING - 1);
			sc->tn_rhead = b0;
			temp_ptr = sc->tn_rmcl[b0];
		}
	}

	/* if we didn't get it all, build caboose */
	if (tlen) {
		MGET(m, M_DONTWAIT, MT_DATA);
		
		if (!m) {
			m_free(m0);
			return 0;
		}
		m0->m_next = m;
		if (tlen <= MLEN) {
			m->m_len = tlen;
			bcopy(temp_ptr, m->m_data, m->m_len);
		} else {
			/* need another cluster */
			MCLGET(m, M_DONTWAIT);
			if (!(m->m_flags & M_EXT)) {
				m_freem(m0);
				return 0;
			}
			m->m_len = tlen;
			pa = pmap_extract(pmap_kernel(),
					  (vm_offset_t) m->m_data);
			if (pa & ~0x00ffffff) {
				/* fresh cluster is in high RAM, must copy */
				bcopy(sc->tn_rmcl[b0], m->m_data, m->m_len);
			} else {
				m->m_ext.ext_buf = sc->tn_rmcl[b0];
				sc->tn_rmcl[b0] = m->m_data;
				m->m_data = m->m_ext.ext_buf;
			}
		}
	}

	pa = pmap_extract(pmap_kernel(), (vm_offset_t) sc->tn_rmcl[b0]);
	sc->tn_rb[b0].addr = pa;
	sc->tn_rb[b0].flags = TNRB_OWN;
	sc->tn_rb[b0].nsize = -TN_RBUFSIZ;
	sc->tn_rb[b0].bcnt = 0;
	b0 = (b0 + 1) & (TN_NRRING - 1);
	sc->tn_rhead = b0;

	return (m0);
}

/*
 * Tnintr is the interrupt handler, called at splimp() by the system.
 */

tnintr(sc)
	struct tn_softc *sc;
{
	while (tn_work(sc, 1) > 0)
		;
	dprintf(("TN intr done\n"));
	return (1);
}

/*
 * Tn_poll is the poll handler -- similar to tnintr but not identical.
 * In particular, the poll routine does not need to loop.
 */

void 
tn_poll(sc)
	struct tn_softc *sc;
{
	int s = splimp();

	if (tn_work(sc, 0) < 0) {
		/* not IFF_RUNNING -- don't reschedule self */
		sc->tn_lflags &= ~TNLF_POLLING;
	} else {
		timeout(tn_poll, sc, 1);
		sc->tn_lflags |= TNLF_POLLING;
	}
	splx(s);
	return;
}

/*
 * Tn_poll_oactive is just like tn_poll except it stops 
 * rescheduling itself if OACTIVE goes off after we service
 * the card.
 *
 * This runs whenever OACTIVE is on, since with transmit intrs
 * disabled, only a receive interrupt or this poll can scavenge
 * Tx ring entries (and potentially turn OACTIVE back off!)
 *
 * Calling this function is a fine way to start the poll going.
 */

void 
tn_poll_oactive(sc)
	struct tn_softc *sc;
{
	int s = splimp();

	if ((tn_work(sc, 0) < 0) || !(sc->tn_if.if_flags & IFF_OACTIVE)) {
		/* either !IFF_RUNNING, or !IFF_OACTIVE */
		sc->tn_lflags &= ~TNLF_POLLING_OA;
	} else {
		timeout(tn_poll_oactive, sc, 1);
		sc->tn_lflags |= TNLF_POLLING_OA;
	}
	splx(s);
	return;
}

/*
 * do the work for tnintr() or tn_poll().  must be called at splimp().
 */

int
tn_work(sc, wantdebug)
	struct tn_softc *sc;
	int wantdebug;
{
	int worked = 0, base = sc->tn_base;
	u_short csr;

	/*
	 * Copy the csr into memory and ACK the outstanding interrupts.
	 * Note: you can't just whack all the interrupt bits because you
	 * would create a race.  After acking the interrupt, return -1
	 * if the interface is marked down.
	 */

	outw(base+TNIO_RAP, TNRA_CSR);
	csr = inw(base+TNIO_RDP);
	outw(base+TNIO_RDP, csr);
	if (wantdebug)
		dprintf(("tn_work: csr=%x\n", csr));

	if (!(sc->tn_if.if_flags & IFF_RUNNING))
		return (-1);

	/* count any errors */
	tn_erroranalysis(sc, csr);

	/* If a receive interrupt happened, service the Rx ring. */
	if (csr & TNCSR_RINT) {
		tn_rxscavenge(sc);
		worked++;
	}

	/* If the Xmitter needs service ... */
	if ((csr & (TNCSR_TINT | TNCSR_TXON) != TNCSR_TXON) ||
	    sc->tn_tqlen || sc->tn_if.if_snd.ifq_head) {
		/* tn_output knows all about servicing the xmitter */
		tn_output(sc);
		worked++;
	}

	return (worked);
}

/*
 * Process an ioctl request.  (pretty much Berkeley code)
 */

tnioctl(ifp, cmd, data)
	struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct tn_softc *sc = tncd.cd_devs[ifp->if_unit];
#ifdef NS
	struct ns_addr *ina = &(IA_SNS(ifa)->sns_addr);
#endif
	int s, error = 0;

	s = splimp();

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			if (!(ifp->if_flags & IFF_RUNNING))
				tninit(ifp->if_unit);	/* before arpwhohas */
			((struct arpcom *) ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *) ifp,
				  &IA_SIN(ifa)->sin_addr);
			break;
#endif
#ifdef NS
		case AF_NS:
			if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *) (sc->tn_addr);
			else {
				bcopy((caddr_t) ina->x_host.c_host,
				      (caddr_t) sc->tn_addr,
				      sizeof(sc->tn_addr));
			}
			if (ifp->if_flags & IFF_RUNNING)
				tnstop(sc);
			tninit(ifp->if_unit);	/* does ne_setaddr() */
			break;
#endif
		default:
			if (!(ifp->if_flags & IFF_RUNNING))
				tninit(ifp->if_unit);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_DEBUG)
			tn_dump(sc);
#ifdef TNDEBUG
		tndebug = ifp->if_flags & IFF_DEBUG;
#endif
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) == IFF_RUNNING)
			tnstop(sc);
		else if ((ifp->if_flags & IFF_UP) &&
			 (!(ifp->if_flags & IFF_RUNNING) ||
			  (ifp->if_flags & (IFF_LINK0 | IFF_PROMISC)) !=
			  (sc->tn_oiflags & (IFF_LINK0 | IFF_PROMISC))
			  )) {
			if (ifp->if_flags & IFF_RUNNING)
				tnstop(sc);
			tninit(ifp->if_unit);
		}
		sc->tn_oiflags = ifp->if_flags;
		break;

#ifdef MULTICAST
	/*
	 * Update our multicast list.
	 */
	case SIOCADDMULTI:
		error = ether_addmulti((struct ifreq *)data, &sc->tn_ac);
		goto reset;

	case SIOCDELMULTI:
		error = ether_delmulti((struct ifreq *)data, &sc->tn_ac);
	reset:
		if (error == ENETRESET) {
			tninit(ifp->if_unit);
			error = 0;
		}
		break;
#endif

	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}

/*
 * tn_dump -- diagnostic routine
 */

tn_dump(sc)
	struct tn_softc *sc;
{
	int base = sc->tn_base;
	int i;

	printf("TN dump called\n");

	printf("rhead = %d\n", sc->tn_rhead);
	for (i = 0; i < TN_NRRING; i++) {
		printf("Rx ring%d: %02x %s\n", i, sc->tn_rb[i].flags,
		       sc->tn_rmcl[i] ? "has cluster" : "no cluster");
	}

	printf("ttail = %d, tqlen = %d\n", sc->tn_ttail, sc->tn_tqlen);
	for (i = 0; i < TN_NTRING; i++) {
		printf("Tx ring%d: %02x %s\n", i, sc->tn_tb[i].flags,
		       sc->tn_tmbuf[i] ? "has mbuf" : "no mbuf");
	}

	printf("%s waiting\n", sc->tn_txwaiting ? "one" : "none");
	outw(base+TNIO_RAP, TNRA_CSR);
	printf("CSR %04x\n", inw(base+TNIO_RDP));
	printf("txhiwat = %d (of %d), rxhiwat = %d (of %d)\n",
	       tn_txhiwat, TN_NTRING, tn_rxhiwat, TN_NRRING);
	if (inw(base+TNIO_RDP) & TNCSR_STOP) {
		outw(base+TNIO_RAP, 74);
		printf("TxCTR %d\n", inw(base+TNIO_RDP));
		outw(base+TNIO_RAP, 78);
		printf("TxLEN %d\n", inw(base+TNIO_RDP));
		outw(base+TNIO_RAP, 72);
		printf("RxCTR %d\n", inw(base+TNIO_RDP));
		outw(base+TNIO_RAP, 76);
		printf("RxLEN %d\n", inw(base+TNIO_RDP));
	}
	outw(base+TNIO_RAP, TNRA_CSR);
}
