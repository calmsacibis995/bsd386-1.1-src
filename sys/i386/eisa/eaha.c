/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: eaha.c,v 1.9 1993/12/17 03:25:21 karels Exp $
 */

/*
 * Adaptec AHA-1740A/1742A SCSI host adapter driver
 * modelled on the 1540B/1542B driver
 *
 * TODO:
 */

#include "eaha.h"

#if NEAHA > 0

#include "param.h"
#include "kernel.h"
#include "systm.h"
#include "buf.h"
#include "device.h"
#include "malloc.h"

#include "vm/vm.h"

#include "scsi/scsi.h"
#include "scsi/scsivar.h"
#include "scsi/disk.h"
#include "scsi/tape.h"

#include "i386/isa/isa.h"
#include "i386/isa/isavar.h"
#include "eisa.h"
#include "i386/isa/icu.h"

#include "eahareg.h"

#define	NSECB	12

/*
 * This is actually a derived class whose base class is a generic host adapter.
 * The generic host adapter in turn has a generic device as its base class.
 * Thus code can use generic device and generic host adapter pointers
 * to refer to objects of this type.
 */
struct eaha_softc {
	struct	hba_softc sc_hba;
	struct	isadev sc_isa;
	struct	intrhand sc_ih;

	int	sc_port;		/* base port for device */

	u_char	sc_id;			/* adapter's SCSI ID */

	/* interrupt state from the current transfer */
	u_char	sc_stat;		/* last host adapter status */
	u_char	sc_targ;		/* current target */
	u_char	sc_lun;			/* current LUN */
	u_long	sc_resid;		/* last residual byte count */

	/* enhanced command control block information */
	struct	soft_ecb *sc_se;	/* page of SECB structures */
	vm_offset_t sc_se_phys;		/* physical address (for dma) */
	u_int	sc_se_count;		/* number of available SECBs */
	struct	soft_ecb *sc_nextse;	/* find free SECBs starting here */
	struct	soft_ecb *sc_gose;	/* send it from eahastart to eahago */

	struct	scsi_sense *sc_sn;	/* argh, for ECB_SENSE */
};

#define	SE_PHYS(sc, v) \
	(((vm_offset_t)(v) - (vm_offset_t)(sc)->sc_se) + (sc)->sc_se_phys)
#define	SE_VIRT(sc, p) \
	(((vm_offset_t)(p) - (sc)->sc_se_phys) + (vm_offset_t)(sc)->sc_se)

static	void eaha_unit_attach __P((struct eaha_softc *sc));
void	eahaattach __P((struct device *parent, struct device *dev, void *args));
static	int eahaecb __P((struct eaha_softc *sc, struct soft_ecb *se,
	    int synch, int mbcmd));
int	eahadump __P((struct hba_softc *hba, int targ, struct scsi_cdb *cdb,
	    caddr_t buf, int len, int isphys));
static	void eahaerror __P((struct eaha_softc *sc, char *err));
static	void eahaerror2 __P((struct eaha_softc *sc, char *err, u_int x));
int	eahago __P((struct device *self, int targ, scintr_fn intr,
	    struct device *dev, struct buf *bp, int pad));
void	eahahbareset __P((struct hba_softc *hba, int resetunits));
int	eahaicmd __P((struct hba_softc *hba, int targ, struct scsi_cdb *cdb,
	    caddr_t buf, int len, int rw));
static	void eahainbox __P((struct eaha_softc *sc));
int	eahaintr __P((void *sc0));
int	eahaprobe __P((struct device *parent, struct cfdata *cf, void *aux));
void	eahastart __P((struct device *self, struct sq *sq, struct buf *bp,
	    scdgo_fn dgo, struct device *dev));
void	eaharel __P((struct device *self));
static	void eahareset __P((struct eaha_softc *sc));
int	eahawatch __P((struct eaha_softc *sc));
static	void esginit __P((struct eaha_softc *sc, struct soft_ecb *se,
	    caddr_t buf, int len));
static	struct soft_ecb *sealloc __P((struct eaha_softc *sc,
	    struct scsi_cdb *cdb));
static	void seinit __P((struct eaha_softc *sc, struct soft_ecb *se, int targ,
	    scintr_fn intr, struct device *dev, caddr_t buf, int len, int rw));
static	u_long seresid __P((struct eaha_softc *sc, struct soft_ecb *se));
static	void sersinit __P((struct eaha_softc *sc, struct soft_ecb *se, int targ,
	    caddr_t buf, int len));

#if __GNUC__ && __STDC__
static inline void
eaha_mbox_out(register int e, register u_long i)
#else
static void
eaha_mbox_out(e, i)
	register int e;
	register u_long i;
#endif
{
	outb(EAHA_MBOUT(e), i);
	outb(EAHA_MBOUT(e) + 1, i >>= 8);
	outb(EAHA_MBOUT(e) + 2, i >>= 8);
	outb(EAHA_MBOUT(e) + 3, i >>= 8);
}

#if __GNUC__ && __STDC__
static inline u_long
eaha_mbox_in(register int e)
#else
static u_long
eaha_mbox_in(e)
	register int e;
#endif
{
	register u_long i;

	i = (((u_char)inb(EAHA_MBIN(e) + 3) << 8 |
	      (u_char)inb(EAHA_MBIN(e) + 2)) << 8 |
	      (u_char)inb(EAHA_MBIN(e) + 1)) << 8 |
	      (u_char)inb(EAHA_MBIN(e));
	return (i);
}

static char *eaha_ids[] = {
	"ADP0000",		/* AHA-1740 */
	"ADP0001",		/* AHA-1740A */
	"ADP0002",		/* AHA-1742A */
	"ADP0400",		/* AHA-1744 */
	0
};

struct cfdriver eahacd =
    { 0, "eaha", eahaprobe, eahaattach, sizeof (struct eaha_softc), eaha_ids };
static struct hbadriver eahahbadriver =
    { eahaicmd, eahadump, eahastart, eahago, eaharel, eahahbareset };

int
eahaprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int irq, slot;

	if (isa_bustype != BUS_EISA)
		return (0);

	if ((slot = eisa_match(cf, ia)) == 0)
		return (0);
	ia->ia_iobase = slot << 12;
	ia->ia_iosize = EISA_NPORT;
	eisa_slotalloc(slot);

	/* if we aren't in enhanced mode, reject */
	if ((inb(EAHA_PORTADDR(EAHA_BASE(slot))) & EP_ENHANCED) == 0)
		/* XXX block out these ports?  print something? */
		return (0);

	/* sanity check for config file value */
	if (ia->ia_irq != IRQUNK && (ia->ia_irq & EAHA_IRQMASK) == 0) {
		printf("eaha: bad IRQ configuration\n");
		ia->ia_irq = IRQUNK;
	}
	if (ia->ia_irq == IRQUNK) {
		if ((irq = isa_irqalloc(EAHA_IRQMASK)) == 0)
			return (0);
		ia->ia_irq = irq;
	}

	/* EISA bus masters don't use host DMA channels */
	ia->ia_drq = 0;		/* XXX should be DRQUNK or DRQBUSMASTER? */

#if 0
	/* decode the BIOS address */
	ia->ia_maddr = ((inb(EAHA_BIOSADDR(e)) & EB_BIOSSEL) << 14) + 0xc0000;
	ia->ia_msize = 0x4000;
#else
	ia->ia_maddr = 0;
	ia->ia_msize = 0;
#endif
	return (1);
}

/*
 * Stolen from Chris's scsi_str() function.
 * XXX export the scsi_str() function instead
 */
static void
eahastr(src, dst, len)
	char *src, *dst;
	int len;
{

	while (src[len - 1] == ' ') {
		if (--len == 0) {
			*dst = 0;
			return;
		}
	}
	bcopy(src, dst, len);
	dst[len] = 0;
}

/*
 * Unlike the 1542B, the 1742A has no fancy built-in command
 * to probe for targets and units on the SCSI bus.
 * This routine could probably go into the generic SCSI driver...
 */
static void
eaha_unit_attach(sc)
	struct eaha_softc *sc;
{
	int targ, lun, status;
	static struct scsi_sense sn;

	for (targ = 0; targ < 8; ++targ) {
		if (targ == sc->sc_id)
			continue;
		for (lun = 0; lun < 8; ++lun) {
			status = scsi_test_unit_ready(&sc->sc_hba, targ, lun);
			if (status == -1)
				continue;
			if ((status & STS_MASK) != STS_GOOD) {
				status = scsi_request_sense(&sc->sc_hba,
				    targ, lun, (caddr_t)&sn, sizeof sn);
				if (status != STS_GOOD)
					continue;
				if (SENSE_ISXSENSE(&sn) && XSENSE_ISSTD(&sn)) {
					if (XSENSE_KEY(&sn) == SKEY_MEDIUM_ERR
					 || XSENSE_KEY(&sn) == SKEY_HARD_ERR
					 || XSENSE_KEY(&sn) == SKEY_ILLEGAL)
					    	continue;
				}
			}
			SCSI_FOUNDTARGET(&sc->sc_hba, targ);
			break;
		}
	}
}

void
eahaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	struct eaha_softc *sc = (struct eaha_softc *) self;
	struct soft_ecb *se;
	struct ehinq *ehinq;
	struct escd *escd;
	vm_offset_t pehinq, pescd;
	char vendor[sizeof ehinq->eh_vendor + 1];
	char product[sizeof ehinq->eh_product + 1];
	char firmware[sizeof ehinq->eh_firmware + 1];
	char rev[sizeof ehinq->eh_rev + 1];
	size_t bytes;
	int intdef;
	int targ, e;
	int need_init = 0;
	u_char ebctrl;

	printf(": ");

	e = sc->sc_port = EAHA_BASE(ia->ia_iobase >> 12);

	/*
	 * The manual says that you should always enable the BIOS
	 * on every adapter in the system, so we should
	 * never need to do initialization ourselves;
	 * this is paranoia.  Probably doesn't work, either.
	 */
	ebctrl = inb(EAHA_EBCTRL(e));
	if (ebctrl & EE_HAERR) {
		/* card went insane, do our best */
		printf("(serious host adapter error) ");
		outb(EAHA_EBCTRL(e), EE_ERRST);
		DELAY(1000000);
		outb(EAHA_EBCTRL(e), 0);
		need_init = 1;
	} else if ((ebctrl & EE_CDEN) == 0)
		/* the BIOS was probably disabled */
		need_init = 1;

	/* disable the BIOS, we can't use it any more */
	outb(EAHA_BIOSADDR(e), 0);

	/* program the IRQ */
	switch (ia->ia_irq) {
	case IRQ9:	intdef = EID_INT9; break;
	case IRQ10:	intdef = EID_INT10; break;
	case IRQ11:	intdef = EID_INT11; break;
	case IRQ12:	intdef = EID_INT12; break;
	case IRQ14:	intdef = EID_INT14; break;
	case IRQ15:	intdef = EID_INT15; break;
	}
	/* XXX EID_INTHIGH is for ISA-compatible interrupts */
	outb(EAHA_INTDEF(e), EID_INTEN | EID_INTHIGH | intdef);

	/* select enhanced mode operation */
	/* EP_ENHANCED has no effect here, it's just for comfort */
	outb(EAHA_PORTADDR(e), EP_ENHANCED | EP_ISADISABLE);

	if (need_init) {
		/* see p 4-27 of the manual for details */
		outb(EAHA_BUSDEF(e), EBD_BUS8);
		outb(EAHA_SCSIDEF(e), ESD_RSTPWR | 7);
		outb(EAHA_EBCTRL(e), EE_CDEN);
		DELAY(100000);	/* paranoia */
	}

	sc->sc_id = inb(EAHA_SCSIDEF(e)) & ESD_HSCSIID;

	/* put the card in a known state */
	outb(EAHA_CTRL(e), EC_HRDY | EC_CLRINT);

	/*
	 * Allocate space for SECBs.
	 * These need to be contiguous because they are accessed
	 * with physical addresses from the card's bus master DMA.
	 * We also need a contiguous sense buffer for ECB_SENSE (sigh).
	 */
	bytes = NSECB * sizeof (struct soft_ecb) + sizeof (struct scsi_sense);
	if (bytes > NBPG)
		panic("eahaattach sc_se");
	MALLOC(sc->sc_se, struct soft_ecb *, bytes, M_DEVBUF, M_WAITOK);
	bzero(sc->sc_se, bytes);
	sc->sc_se_phys = pmap_extract(pmap_kernel(), (vm_offset_t)sc->sc_se);
	sc->sc_se_count = NSECB;
	sc->sc_nextse = sc->sc_se;
	sc->sc_gose = 0;    
	for (se = sc->sc_se; se < &sc->sc_se[sc->sc_se_count]; ++se)
		se->se_ecb.e_cmd = ECB_FREE;
	sc->sc_sn = (struct scsi_sense *)((caddr_t)sc->sc_se + bytes) - 1;

	/*
	 * Execute an ECB to obtain host adapter inquiry data.
	 * This is done solely to print cute info during autoconfiguration.
	 */
	MALLOC(ehinq, struct ehinq *, sizeof *ehinq, M_DEVBUF, M_WAITOK);
	pehinq = pmap_extract(pmap_kernel(), (vm_offset_t)ehinq);
	se = sc->sc_nextse;
	se->se_ecb.e_cmd = ECB_INQUIRY;
	se->se_ecb.e_flag1 = F1_SES;
	se->se_ecb.e_data = pehinq;
	se->se_ecb.e_datalen = sizeof *ehinq;
	se->se_ecb.e_status = SE_PHYS(sc, &se->se_est);
	se->se_targ = sc->sc_id;
	if ((eahaecb(sc, se, 1, EA_START) & STS_MASK) != STS_GOOD)
		printf("(host adapter inquiry failed, hastat = 0x%x)",
		    se->se_est.es_hastat);
	else {
		eahastr(ehinq->eh_vendor, vendor, sizeof ehinq->eh_vendor);
		eahastr(ehinq->eh_product, product, sizeof ehinq->eh_product);
		eahastr(ehinq->eh_firmware, firmware,
		    sizeof ehinq->eh_firmware);
		eahastr(ehinq->eh_rev, rev, sizeof ehinq->eh_rev);
		printf("%s %s (%s) rev %s", vendor, product, firmware, rev);
		if (ehinq->eh_flags & EHF_WID)
			printf(", 16-bit data bus");
		if (ehinq->eh_ecbs < sc->sc_se_count)
			/* should never happen */
			sc->sc_se_count = ehinq->eh_ecbs;
	}
	FREE(ehinq, M_DEVBUF);

	printf("\n");

	if (need_init) {
		/*
		 * Program the SCSI configuration data.
		 * XXX 10 MB/s synch transfers hang on my test machine!
		 */
		MALLOC(escd, struct escd *, sizeof *escd, M_DEVBUF, M_WAITOK);
		pescd = pmap_extract(pmap_kernel(), (vm_offset_t)escd);
		for (targ = 0;
		     targ < sizeof escd->ec_target / sizeof escd->ec_target[0];
		     ++targ)
			escd->ec_target[targ] = ESC_DISCONN | ESC_SYNCH |
			    ESC_PARITY | ESC_XFER_6_7;
		se = sc->sc_nextse;
		se->se_ecb.e_cmd = ECB_INIT;
		se->se_ecb.e_flag1 = F1_SES;
		se->se_ecb.e_data = pescd;
		se->se_ecb.e_datalen = sizeof *escd;
		se->se_ecb.e_status = SE_PHYS(sc, &se->se_est);
		se->se_targ = sc->sc_id;
		if ((eahaecb(sc, se, 1, EA_START) & STS_MASK) != STS_GOOD)
			printf("%s: failed to set SCSI configuration data\n",
			    sc->sc_hba.hba_dev.dv_xname);
		FREE(escd, M_DEVBUF);
	}

	/*
	 * Link into ISA/EISA and set interrupt handler.
	 */
	isa_establish(&sc->sc_isa, &sc->sc_hba.hba_dev);
	sc->sc_ih.ih_fun = eahaintr;
	sc->sc_ih.ih_arg = sc;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_DISK);

	sc->sc_hba.hba_driver = &eahahbadriver;

	eahareset(sc);

	printf("%s: delaying to permit certain devices to finish self-test",
	    sc->sc_hba.hba_dev.dv_xname);
	DELAY(5000000);
	printf("\n");

	eaha_unit_attach(sc);
}

static void
eahareset(sc)
	struct eaha_softc *sc;
{
	int e = sc->sc_port;
	u_char intr, result;

	while ((inb(EAHA_STAT(e)) & (ES_EMPTY|ES_BUSY)) != ES_EMPTY)
		;
	eaha_mbox_out(e, IM_CMD_RESET);
	outb(EAHA_CTRL(e), EC_HRDY);
	outb(EAHA_ATTN(e), EA_IMMED | sc->sc_id);

	while ((inb(EAHA_STAT(e)) & ES_INTR) == 0)
		;
	intr = inb(EAHA_INTR(e));	
	result = inb(EAHA_MBIN(e));
	outb(EAHA_CTRL(e), EC_HRDY | EC_CLRINT);

	if (intr != (EI_IMMSUCC | sc->sc_id))
		printf("%s: reset failed, intr = 0x%x, result = 0x%x\n",
		    sc->sc_hba.hba_dev.dv_xname, intr, result);
}

void
eahahbareset(hba, resetunits)
	struct hba_softc *hba;
	int resetunits;
{
	struct eaha_softc *sc = (struct eaha_softc *)hba;

	eahareset(sc);

	if (resetunits)
		/* XXX do a device reset on each unit? */
		scsi_reset_units(hba);
}

/*
 * Given a buffer and a length, set up a scatter/gather map
 * in an ecb to map the buffer for a SCSI transfer.
 * Initialize the ECB flag for the correct transfer type.
 */
static void
esginit(sc, se, buf, len)
	struct eaha_softc *sc;
	struct soft_ecb *se;
	caddr_t buf;
	int len;
{
	struct esg *esg = &se->se_sg[0];
	vm_size_t pages;
	vm_offset_t start = i386_trunc_page(buf);
	vm_offset_t end = i386_round_page(buf + len);
	vm_offset_t v = (vm_offset_t)buf;
	struct ecb *ecb = &se->se_ecb;

	if (len <= 0) {
		ecb->e_data = 0;
		ecb->e_datalen = 0;
		return;
	}

	if (start + NBPG == end) {
		/* transfer lies entirely within a page */
		ecb->e_data = pmap_extract(pmap_kernel(), v);
		ecb->e_datalen = (vm_size_t)len;
		return;
	}

	/* transfer too big -- we'll need a scatter/gather map */
	if ((pages = i386_btop(end - start)) > 17)
		panic("esginit pages");

	ecb->e_flag1 |= F1_SG;
	ecb->e_data = SE_PHYS(sc, esg);
	ecb->e_datalen = pages * sizeof *esg;

	end = v + len;
	do {
		start = v;
		v = MIN(i386_trunc_page(start + NBPG), end);
		esg->esg_data = pmap_extract(pmap_kernel(), start);
		esg->esg_datalen = v - start;
		++esg;
	} while (v < end);
}

/*
 * Allocate and initialize a software ECB.
 * ECBs currently all live in a single page,
 * to make it simple to find their physical addresses.
 * Must be called at splbio().
 * XXX we wouldn't need splbio() if we didn't call this from eahaintr()!
 */
static struct soft_ecb *
sealloc(sc, cdb)
	struct eaha_softc *sc;
	struct scsi_cdb *cdb;
{
	struct soft_ecb *se = sc->sc_nextse;
	struct ecb *ecb;
	int i;

	for (i = 0; i < sc->sc_se_count; ++i) {
		if (se->se_ecb.e_cmd == ECB_FREE)
			break;
		if (++se >= sc->sc_se + sc->sc_se_count)
			se = sc->sc_se;
	}
	ecb = &se->se_ecb;
	if (ecb->e_cmd != ECB_FREE)
		panic("sealloc");
	ecb->e_cmd = ECB_CMD;
	ecb->e_flag1 = F1_DSB;
	ecb->e_flag2 = 0;
	ecb->e_status = SE_PHYS(sc, &se->se_est);
	ecb->e_sense = 0;
	ecb->e_senselen = 0;    
	sc->sc_nextse = se + 1;
	if (sc->sc_nextse >= sc->sc_se + sc->sc_se_count)
		sc->sc_nextse = sc->sc_se;

	if (cdb) {
		if (SCSICMDLEN(cdb->cdb_bytes[0]) == 6)
			ecb->e_cdb6 = *CDB6(cdb);
		else if (SCSICMDLEN(cdb->cdb_bytes[0]) == 10)
			ecb->e_cdb10 = *CDB10(cdb);
		else
			panic("sealloc cdb size");
	}

	return (se);
}

/*
 * Initialize (most of) an SECB.
 * We assume that the CDB has already been set up.
 */
static void
seinit(sc, se, targ, intr, dev, buf, len, rw)
	struct eaha_softc *sc;
	struct soft_ecb *se;
	int targ;
	scintr_fn intr;
	struct device *dev;
	caddr_t buf;
	int len, rw;
{
	struct ecb *ecb = &se->se_ecb;

	/*
	 * Store target and LUN, and set CDB length.
	 */
	se->se_targ = targ;
	ecb->e_flag2 |= ecb->e_cdbbytes[1] >> 5;
	if (len) {
		ecb->e_flag2 |= F2_DAT;
		if (rw == B_READ)
			ecb->e_flag2 |= F2_DIR;
	}
	se->se_len = len;
	ecb->e_cdblen = SCSICMDLEN(ecb->e_cdbbytes[0]);

	esginit(sc, se, buf, len);

	se->se_intr = intr;
	se->se_intrdev = dev;
}

/*
 * This routine is required because of the way
 * the 174x treats contingent allegiance conditions.
 * Only the special ECB_SENSE command can be used
 * to request sense after a check condition status is reported.
 */
static void
sersinit(sc, se, targ, buf, len)
	struct eaha_softc *sc;
	struct soft_ecb *se;
	int targ;
	caddr_t buf;
	int len;
{
	struct ecb *ecb = &se->se_ecb;
	vm_offset_t start = i386_trunc_page(buf);
	vm_offset_t end = i386_round_page(buf + len);

	se->se_targ = targ;
	se->se_len = len;
	ecb->e_cmd = ECB_SENSE;
	ecb->e_flag1 |= F1_SES;
	ecb->e_flag2 |= ecb->e_cdbbytes[1] >> 5;
	ecb->e_data = 0;
	ecb->e_datalen = 0;
	ecb->e_cdblen = 0;
	bzero(ecb->e_cdbbytes, sizeof ecb->e_cdbbytes);

	/*
	 * If the buffer doesn't cross a page boundary, use it directly.
	 * Otherwise we must copy...
	 */
	if (start + NBPG == end) {
		ecb->e_sense = pmap_extract(pmap_kernel(), (vm_offset_t)buf);
		ecb->e_senselen = len;
	} else {
		ecb->e_sense = SE_PHYS(sc, sc->sc_sn);
		ecb->e_senselen = sizeof *sc->sc_sn;
	}
}

/*
 * The adapter returns the number of bytes that
 * it failed to transfer from the current scatter/gather buffer.
 * This routine converts this value into a net residual byte count.
 */
static u_long
seresid(sc, se)
	struct eaha_softc *sc;
	struct soft_ecb *se;
{
	struct estatus *es = &se->se_est;
	struct esg *esg;
	u_long len, resid;
	int found;

	if ((es->es_flags & ESF_DU) == 0)
		return (0);
	if ((se->se_ecb.e_flag1 & F1_SG) == 0)
		return (es->es_resid);
	found = 0;
	for (esg = se->se_sg, len = se->se_len;
	     len;
	     len -= esg->esg_datalen, ++esg)
		if (found)
			resid += esg->esg_datalen;
		else if (es->es_residbuf == SE_PHYS(sc, esg)) {
			resid = esg->esg_datalen - es->es_resid;
			found = 1;
		}
	return (resid);
}

/*
 * Fire off an ECB.
 * If synch is set, poll for completion.
 * Must be called at splbio().
 */
static int
eahaecb(sc, se, synch, mbcmd)
	struct eaha_softc *sc;
	struct soft_ecb *se;
	int synch;
	int mbcmd;
{
	struct ecb *ecb = &se->se_ecb;
	u_long ecb_phys = SE_PHYS(sc, ecb);
	int e = sc->sc_port;
	int status;
	u_char intr;

	while ((inb(EAHA_STAT(e)) & (ES_EMPTY|ES_BUSY)) != ES_EMPTY)
		;
	eaha_mbox_out(e, ecb_phys);
	outb(EAHA_ATTN(e), mbcmd | se->se_targ);

	if (!synch)
		return (0);

	/*
	 * Wait for a synchronous command to complete.
	 * If we picked up an asynchronous command,
	 * process it and try again.
	 */
	for (;;) {
		while ((inb(EAHA_STAT(e)) & ES_INTR) == 0)
			;
		intr = inb(EAHA_INTR(e));
		if ((intr & EI_TARGMASK) == se->se_targ &&
		    eaha_mbox_in(e) == ecb_phys)
			break;
		eahainbox(sc);
	}
	outb(EAHA_CTRL(e), EC_HRDY | EC_CLRINT);

	/*
	 * We only retrieve the status block if there has been an error.
	 * Underruns aren't (necessarily) errors, so we handle them specially.
	 */
	if ((intr & EI_STATMASK) == EI_SUCCESS ||
	    (intr & EI_STATMASK) == EI_RETRY) {
		sc->sc_resid = 0;
		status = STS_GOOD;
	} else {
		sc->sc_resid = seresid(sc, se);
		if (ESF_BAD(se->se_est.es_flags)) {
			if (se->se_est.es_hastat == ESH_LENGTH &&
			    (sc->sc_resid != se->se_len ||
			     ecb->e_cdbbytes[0] == CMD_MODE_SENSE ||
			     ecb->e_cdbbytes[0] == CMD_READ6 ||
			     ecb->e_cdbbytes[0] == CMD_READ10 ||
			     ecb->e_cdbbytes[0] == CMD_WRITE6 ||
			     ecb->e_cdbbytes[0] == CMD_WRITE10))
				/*
				 * Ignore overrun and underrun in two cases:
				 * we actually did move some data, or
				 * the command was a read or write.
				 */
				status = STS_GOOD;
			else {
				status = -1;
				if (se->se_est.es_hastat != ESH_TIMEOUT &&
				    ecb->e_cdbbytes[0] != CMD_TEST_UNIT_READY &&
				    ecb->e_cdbbytes[0] != CMD_READ_CAPACITY)
				/* typical config error -- keep quiet */
printf("%s: host adapter error on immediate command, flags=%b, status=%x\n",
					    sc->sc_hba.hba_dev.dv_xname,
					    se->se_est.es_flags, ESF_BITS,
					    se->se_est.es_hastat);
			}
		} else
			status = se->se_est.es_tarstat;
	}

	if (sc->sc_gose == se)
		sc->sc_gose = 0;
	ecb->e_cmd = ECB_FREE;

	return (status);
}

int
eahaicmd(hba, targ, cdb, buf, len, rw)
	struct hba_softc *hba;
	int targ;
	struct scsi_cdb *cdb;
	caddr_t buf;
	int len, rw;
{
	struct eaha_softc *sc = (struct eaha_softc *)hba;
	struct soft_ecb *se;
	int s, error;

	s = splbio();    
	se = sealloc(sc, cdb);
	if (CDB6(cdb)->cdb_cmd == CMD_REQUEST_SENSE)
		sersinit(sc, se, targ, buf, len);
	else
		seinit(sc, se, targ, (scintr_fn)NULL, (struct device *)NULL,
		    buf, len, rw);
	error = eahaecb(sc, se, 1, EA_START);
	/* kluge city... */
	if (se->se_ecb.e_cmd == ECB_SENSE &&
	    se->se_ecb.e_sense == SE_PHYS(sc, sc->sc_sn))
		bcopy((caddr_t)sc->sc_sn, buf, len);
	splx(s);
	return (error);
}

int
eahadump(hba, targ, cdb, buf, len, isphys)
	struct hba_softc *hba;
	int targ;
	struct scsi_cdb *cdb;
	caddr_t buf;
	int len, isphys;
{
	struct eaha_softc *sc = (struct eaha_softc *)hba;
	struct soft_ecb *se;
	struct ecb *ecb;

	if (!isphys)
		panic("eahadump virtual!?\n");

	/*
	 * Several assumptions:
	 * +	The upper-level dump code always calls us on aligned
	 *	2^n chunks which never cross 64 KB physical memory boundaries
	 * +	We're running in virtual mode with interrupts blocked
	 */

	se = sealloc(sc, cdb);
	ecb = &se->se_ecb;

	ecb->e_cmd = ECB_CMD;
	se->se_targ = targ;
	ecb->e_flag2 |= ecb->e_cdbbytes[1] >> 5;
	ecb->e_cdblen = SCSICMDLEN(ecb->e_cdbbytes[0]);
	ecb->e_data = (vm_offset_t)buf;
	ecb->e_datalen = (vm_size_t)len;

	se->se_intr = NULL;
	se->se_intrdev = 0;

	return (eahaecb(sc, se, 1, EA_START));
}

/*
 * Start a transfer.
 *
 * Since the AHA-1542B handles transactions in parallel (with disconnect /
 * reconnect), we don't have to queue requests for the host adapter.
 *
 * This code is NOT re-entrant: (*dgo)() must call ahago() without calling
 * eahastart() first (because of the sc_gose kluge).
 */
void
eahastart(self, sq, bp, dgo, dev)
	struct device *self;
	struct sq *sq;
	struct buf *bp;
	scdgo_fn dgo;
	struct device *dev;
{
	struct eaha_softc *sc = (struct eaha_softc *)self;

	if (bp) {
		/* asynch transaction */
		sc->sc_gose = sealloc(sc, (struct scsi_cdb *)NULL);
		(*dgo)(dev,
		    (struct scsi_cdb *)&sc->sc_gose->se_ecb.e_cdb);
		return;
	}
	/* let ahaicmd() allocate its own secb */
	(*dgo)(dev, (struct scsi_cdb *)NULL);
}

/*
 * Get the host adapter going on a command.
 *
 * XXX must be called at splbio() since interrupts can call it
 * XXX but it'd be much better for the work to get done at low ipl!
 */
int
eahago(self, targ, intr, dev, bp, pad)
	struct device *self;
	int targ;
	scintr_fn intr;
	struct device *dev;
	struct buf *bp;
	int pad;
{
	struct eaha_softc *sc = (struct eaha_softc *)self;

	seinit(sc, sc->sc_gose, targ, intr, dev,
	    bp->b_un.b_addr, bp->b_bcount, bp->b_flags & B_READ);
	return (eahaecb(sc, sc->sc_gose, 0, EA_START));
}

static void
eahaerror(sc, err)
	struct eaha_softc *sc;
	char *err;
{

	printf("%s: %s (target %d, lun %d)\n",
	    sc->sc_hba.hba_dev.dv_xname, err, sc->sc_targ, sc->sc_lun);
}

static void
eahaerror2(sc, err, x)
	struct eaha_softc *sc;
	char *err;
	u_int x;
{

	printf("%s: %s 0x%x (target %d, lun %d)\n",
	    sc->sc_hba.hba_dev.dv_xname, err, x, sc->sc_targ, sc->sc_lun);
}

/*
 * Handle a filled in-mailbox.
 */
static void
eahainbox(sc)
	struct eaha_softc *sc;
{
	int e = sc->sc_port;
	struct soft_ecb *se = (struct soft_ecb *)SE_VIRT(sc, eaha_mbox_in(e));
	struct ecb *ecb = &se->se_ecb;
	struct sq *sq;
	int status;
	int istat = inb(EAHA_INTR(e)) & EI_STATMASK;
	int esf;

	/* cue the next completed command */
	outb(EAHA_CTRL(e), EC_HRDY | EC_CLRINT);

	if (se < sc->sc_se || se >= &sc->sc_se[sc->sc_se_count]) {
		printf("%s: inbox ecb phys %x: ", sc->sc_hba.hba_dev.dv_xname,
		    SE_PHYS(sc, se));
		panic("bogus ecb");
	}

	sc->sc_hba.hba_intr = se->se_intr;
	sc->sc_hba.hba_intrdev = se->se_intrdev;
	sc->sc_targ = se->se_targ;
	sc->sc_lun = ecb->e_flag2 & F2_LUNMASK;

	/*
	 * We don't get a status block unless
	 * there's been an error, so we have to
	 * be careful when extracting status.
	 */
	if (istat == EI_SUCCESS || istat == EI_RETRY) {
		sc->sc_resid = 0;
		sc->sc_stat = ESH_GOOD;
		status = STS_GOOD;
	} else {
		sc->sc_resid = seresid(sc, se);
		if (ESF_HAERROR(se->se_est.es_flags))
			sc->sc_stat = se->se_est.es_hastat;
		else
			sc->sc_stat = ESH_GOOD;
		status = -1;

		switch (sc->sc_stat) {
		case ESH_HOST:
			/* command aborted by host -- presumably on purpose */
			break;
		case ESH_ADAPTER:
			eahaerror(sc, "command aborted by adapter");
			break;
		case ESH_DOWNLOAD:
			eahaerror(sc, "firmware not downloaded");
			break;
		case ESH_TARGET:
			eahaerror(sc, "invalid target");
			break;
		case ESH_TIMEOUT:
			eahaerror(sc, "selection timeout");
			break;
		case ESH_BUSFREE:
			eahaerror(sc, "unexpected bus free occurred");
			break;
		case ESH_BUSPHASE:
			eahaerror(sc, "invalid bus phase detected");
			break;
		case ESH_OPCODE:
			eahaerror2(sc, "invalid operation code", ecb->e_cmd);
			break;
		case ESH_LINK:
			eahaerror(sc, "invalid SCSI linking operation");
			break;
		case ESH_PARAM:
			eahaerror(sc, "invalid control block parameter");
			break;
		case ESH_DUPTARG:
			eahaerror(sc, "duplicate target CDB received");
			break;
		case ESH_SCATTER:
			eahaerror(sc, "invalid scatter/gather list");
			break;
		case ESH_SENSE:
			eahaerror(sc, "request sense command failed");
			break;
		case ESH_TAG:
			eahaerror(sc, "tagged queuing message rejected");
			break;
		case ESH_HARDWARE:
			eahaerror(sc, "host adapter hardware error");
			break;
		case ESH_ATTN:
			eahaerror(sc, "target didn't respond to attn");
			break;
		case ESH_HARESET:
			eahaerror(sc, "SCSI bus reset by host adapter");
			break;
		case ESH_RESET:
			eahaerror(sc, "SCSI bus reset by other device");
			break;
		case ESH_CKSUM:
			eahaerror(sc, "program checksum failure");
			break;
		case ESH_LENGTH:	/* ignore data overrun / underrun */
		case ESH_GOOD:
			esf = se->se_est.es_flags;
			if (esf & ESF_QF)
				/* should retry this */
				eahaerror(sc, "host adapter queue full");
			else if (esf & ESF_CH)
				/* should never happen */
				eahaerror(sc, "chaining halted with error");
			else if (esf & ESF_ECA)
				/* XXX should execute a RESUME command here */
				eahaerror(sc,
				    "extended contingent allegiance");
			else
				status = se->se_est.es_tarstat;
			break;
		default:
			eahaerror2(sc, "unknown host adapter error",
			    sc->sc_stat);
			break;
		}
	}

	/* free up resources */
	if (sc->sc_gose == se)
		sc->sc_gose = 0;
	ecb->e_cmd = ECB_FREE;

	if (sc->sc_hba.hba_intr && sc->sc_hba.hba_intrdev) {
		/*
		 * For non-immediate commands,
		 * pass status back to higher levels and
		 * start the next transfer.
		 */
		(*sc->sc_hba.hba_intr)(sc->sc_hba.hba_intrdev, status,
		    sc->sc_resid);
		if ((sq = sc->sc_hba.hba_head) != NULL) {
			sc->sc_hba.hba_head = sq->sq_forw;
			sc->sc_gose = sealloc(sc, (struct scsi_cdb *)NULL);
			(*sq->sq_dgo)(sq->sq_dev, (struct scsi_cdb *)
			    &sc->sc_gose->se_ecb.e_cdb);
		}
	}
}

int
eahaintr(sc0)
	void *sc0;
{
	struct eaha_softc *sc = sc0;

	while (inb(EAHA_STAT(sc->sc_port)) & ES_INTR)
		eahainbox(sc);

	return (1);
}

void
eaharel(self)
	struct device *self;
{
	struct eaha_softc *sc = (struct eaha_softc *)self;
	struct soft_ecb *se;
	struct sq *sq;

	if (se = sc->sc_gose) {
		if (sc->sc_gose == se)
			sc->sc_gose = 0;
		se->se_ecb.e_cmd = ECB_FREE;
	}
	if ((sq = sc->sc_hba.hba_head) != NULL) {
		sc->sc_gose = sealloc(sc, (struct scsi_cdb *)NULL);
		(*sq->sq_dgo)(sq->sq_dev,
		    (struct scsi_cdb *)&sc->sc_gose->se_ecb.e_cdb);
	}
}

#endif /* EAHA */
