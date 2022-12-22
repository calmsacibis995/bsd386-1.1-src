/*-
 * Copyright (c) 1991, 1992, 1993 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: aha.c,v 1.14 1993/12/17 16:14:31 karels Exp $
 */

/*
 * Adaptec AHA-1540[BC]/1542[BC] SCSI host adapter driver
 *
 * TODO:
 *	Compute scatter/gather maps when buffers are queued, not when used
 *	Compute CCB's when buffers are queued
 *	Add support for multiple outstanding commands on the same target
 *	Add hacks for AHA-1540A cards
 */

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

#include "isa.h"
#include "isavar.h"
#include "icu.h"

#include "machine/cpu.h"

#include "ahareg.h"

#define	NO_DRQ	0	/* probably should be -1, and should be elsewhere */

extern int maxmem;

/*
 * Having more than one or two mboxes is useful mainly for debugging.
 * These sizes are calculated to fit inside 2 KB.
 */
#define	NMBOX	8
#define	NSCCB	12

/*
 * This is actually a derived class whose base class is a generic host adapter.
 * The generic host adapter in turn has a generic device as its base class.
 * Thus code can use generic device and generic host adapter pointers
 * to refer to objects of this type.
 */
struct aha_softc {
	struct	hba_softc sc_hba;
	struct	isadev sc_isa;
	struct	intrhand sc_ih;

	int	sc_port;		/* base port for device */

	u_char	sc_id;			/* adapter's SCSI ID */

	/* interrupt state from the current transfer */
	u_char	sc_istat;		/* last interrupt status */
	u_char	sc_stat;		/* last SCSI status */
	u_char	sc_targ;		/* current target */
	u_char	sc_lun;			/* current LUN */
	u_int	sc_resid;		/* last residual byte count */

	/* in/out mailbox information */
	struct	mbox *sc_mbox;		/* virtual address of mbox area */
	u_long	sc_mbox_phys;		/* physical address (for dma) */
	u_long	sc_mbox_count;		/* number of in or out mailboxes */
	struct	mbox *sc_outbox;	/* next available out mbox */
	struct	mbox *sc_inbox;		/* next expected in mbox */

	/* command control block information */
	struct	soft_ccb *sc_sccb;	/* page of SCCB structures */
	u_long	sc_sccb_phys;		/* physical address (for dma) */
	u_int	sc_sccb_count;		/* number of available SCCBs */
	struct	soft_ccb *sc_nextsccb;	/* find free SCCBs starting here */
	struct	soft_ccb *sc_gosccb;	/* send sccb from ahastart to ahago */

	/* bounce information */
	struct	aha_bounce *sc_bo[8];	/* for faking DMA above 16 MB */
};

#define	AHA_CCB_TO_SCCB(sc, p) \
	((sc)->sc_sccb + \
	    ((struct soft_ccb *)aha_to_host(p) - \
	     (struct soft_ccb *)(sc)->sc_sccb_phys))

void	ahaattach __P((struct device *parent, struct device *dev, void *args));
static	int ahaccb __P((struct aha_softc *sc, struct soft_ccb *sccb,
	    int synch, int mbcmd));
int	ahadump __P((struct hba_softc *hba, int targ, struct scsi_cdb *cdb,
	    caddr_t buf, int len, int isphys));
static	void ahaerror __P((struct aha_softc *sc, char *err));
static	void ahaerror2 __P((struct aha_softc *sc, char *err, u_int x));
static	int ahafastcmd __P((int aha, int cmd, u_char *out, u_char *in));
int	ahago __P((struct device *self, int targ, scintr_fn intr,
	    struct device *dev, struct buf *bp, int pad));
void	ahahbareset __P((struct hba_softc *hba, int resetunits));
int	ahaicmd __P((struct hba_softc *hba, int targ, struct scsi_cdb *cdb,
	    caddr_t buf, int len, int rw));
static	void ahainbox __P((struct aha_softc *sc));
int	ahaintr __P((void *sc0));
int	ahaprobe __P((struct device *parent, struct cfdata *cf, void *aux));
void	ahastart __P((struct device *self, struct sq *sq, struct buf *bp,
	    scdgo_fn dgo, struct device *dev));
void	aharel __P((struct device *self));
static	void ahareset __P((struct aha_softc *sc));
int	ahawatch __P((struct aha_softc *sc));
static	void bounce_alloc __P((struct aha_softc *sc, int targ));
static	void bounce_in __P((struct aha_softc *sc, struct soft_ccb *sccb));
static	vm_offset_t bounce_out __P((struct aha_softc *sc,
    struct soft_ccb *sccb, int indx, vm_offset_t v, vm_size_t len, int rw));
static	struct soft_ccb *sccballoc __P((struct aha_softc *sc,
	    struct scsi_cdb *cdb));
static	void sccbinit __P((struct aha_softc *sc, struct soft_ccb *sccb,
	    int targ, scintr_fn intr, struct device *dev, caddr_t buf,
	    int len, int rw));
static	void set_burst_mode __P((struct aha_softc *sc));
static	void sginit __P((struct aha_softc *sc, struct soft_ccb *sccb,
	    caddr_t buf, int len, int rw));

struct cfdriver ahacd =
    { 0, "aha", ahaprobe, ahaattach, sizeof (struct aha_softc) };
static struct hbadriver ahahbadriver =
    { ahaicmd, ahadump, ahastart, ahago, aharel, ahahbareset };

/*
 * The Adaptec uses 3-byte big-endian physical addresses internally (argh).
 */
#if __GNUC__ && __STDC__
static inline void
host_to_aha(register u_long n, register u_char *cp)
#else
static void
host_to_aha(n, cp)
	register u_long n;
	register u_char *cp;
#endif
{

	cp[2] = n; n >>= 8;
	cp[1] = n; n >>= 8;
	cp[0] = n;
}

#if __GNUC__ && __STDC__
static inline u_long
aha_to_host(register u_char *cp)
#else
static u_long
aha_to_host(cp)
	register u_char *cp;
#endif
{
	register u_long n;

 	n = cp[0]; n <<= 8;
 	n |= cp[1]; n <<= 8;
 	n |= cp[2];

 	return (n);
}

/*
 * Look for an empty outbox.
 * If we don't find one, return NULL.
 */
#if __GNUC__ && __STDC__
static inline struct mbox *
scanmbo(register struct aha_softc *sc, struct mbox *start)
#else
static struct mbox *
scanmbo(sc, start)
	register struct aha_softc *sc;
	struct mbox *start;
#endif
{
	int i;
	struct mbox *mbo = start;

	for (i = 0; i < sc->sc_mbox_count; ++i) {
		if (mbo->mb_status == MBOX_O_FREE)
			return (mbo);
		if (++mbo >= sc->sc_mbox + sc->sc_mbox_count)
			mbo = sc->sc_mbox;
	}

	return (NULL);
}

/*
 * Look for a filled inbox.
 * If we don't find one, return start.
 */
#if __GNUC__ && __STDC__
static inline struct mbox *
scanmbi(register struct aha_softc *sc, struct mbox *start)
#else
static struct mbox *
scanmbi(sc, start)
	register struct aha_softc *sc;
	struct mbox *start;
#endif
{
	int i;
	struct mbox *mbi = start;

	for (i = 0; i < sc->sc_mbox_count; ++i) {
		if (mbi->mb_status != MBOX_I_FREE)
			return (mbi);
		if (++mbi >= sc->sc_mbox + 2 * sc->sc_mbox_count)
			mbi = sc->sc_mbox + sc->sc_mbox_count;
	}

	return (start);
}

static const char cmd_out_size[] =
    { 0, 4, 0, 0, 0, 1, 4, 1, 1, 1, 0, 0, 2, 1, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 0, 1,
      0, 2, 35,3, 1, 0, 0, 0, 0, 2, 0 };

/*
 * Send a command to the host adapter and recover the output immediately.
 */
static int
ahafastcmd(aha, cmd, out, in)
	int aha, cmd;
	unsigned char *out, *in;
{
	int cnt;
	int s = splbio();

	while ((inb(AHA_STAT(aha)) & AHA_S_IDLE) == 0)
		;

	if (inb(AHA_INTR(aha)) & AHA_I_ANY)
		outb(AHA_STAT(aha), AHA_C_IRST);

	/* send command and data bytes */
	for (cnt = cmd_out_size[cmd]; cnt >= 0; --cnt) {
		outb(AHA_DATA(aha), cmd >= 0 ? cmd : *out++);
		cmd = -1;
		while (inb(AHA_STAT(aha)) & AHA_S_CDF)
			;
		if (inb(AHA_INTR(aha)) & AHA_I_HACC)
			break;
	}

	/* receive bytes */
	while ((inb(AHA_INTR(aha)) & AHA_I_HACC) == 0) {
		if (inb(AHA_STAT(aha)) & AHA_S_DF)
			*in++ = inb(AHA_DATA(aha));
	}

	/* look for an error */
	if (inb(AHA_STAT(aha)) & AHA_S_INVDCMD) {
		outb(AHA_STAT(aha), AHA_C_IRST);
		splx(s);
		return (EIO);
	}

	outb(AHA_STAT(aha), AHA_C_IRST);

	splx(s);
	return (0);
}

int
ahaprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	u_char in[20];
	struct isa_attach_args *args = aux;
	int aha = args->ia_iobase;
	int i;

	/* check for diagnostic failure and reserved bits */
	if (inb(AHA_STAT(aha)) & (AHA_S_DIAGF|AHA_S_RSV02))
		return (0);
	if (inb(AHA_INTR(aha)) & (AHA_I_RSV40|AHA_I_RSV20|AHA_I_RSV10))
		return (0);

	/* hard reset, then wait 5 seconds for self-test to complete */
	outb(AHA_STAT(aha), AHA_C_HRST);
	for (i = 0;
	     (inb(AHA_STAT(aha)) & (AHA_S_STST|AHA_S_DIAGF)) == AHA_S_STST;
	     ++i) {
	     	if (i >= 50)
	     		return (0);
		DELAY(100000);
	}
	if ((inb(AHA_STAT(aha)) &
	     (AHA_S_STST|AHA_S_DIAGF|AHA_S_INIT|AHA_S_IDLE)) !=
	    (AHA_S_INIT|AHA_S_IDLE))
		/* print something? */
		return (0);

	if (ahafastcmd(aha, AHA_CONFIG_DATA, (u_char *)0, in))
		return (0);
	switch (in[0]) {
	case 0x80:	args->ia_drq = 7; break;
	case 0x40:	args->ia_drq = 6; break;
	case 0x20:	args->ia_drq = 5; break;
	case 0x01:	args->ia_drq = 0; break;
	case 0x00:	args->ia_drq = NO_DRQ; break; /* No DMA.  Local Bus? */
	default:	return (0);
	}
	switch (in[1]) {
	case 0x40:	args->ia_irq = IRQ15; break;
	case 0x20:	args->ia_irq = IRQ14; break;
	case 0x08:	args->ia_irq = IRQ12; break;
	case 0x04:	args->ia_irq = IRQ11; break;
	case 0x02:	args->ia_irq = IRQ10; break;
	case 0x01:	args->ia_irq = IRQ9; break;
	default:	return (0);
	}
	args->ia_maddr = 0;
	args->ia_msize = 0;
	args->ia_iosize = AHA_NPORT;

	return (1);
}

/*
 * Make sure that we don't get stuck.
 * We check for stale mailboxes.
 */
u_long aha_missed_mbif;
u_long aha_aborted_command;
#define	AHA_WATCH_INTERVAL	45

int
ahawatch(sc)
	struct aha_softc *sc;
{
	struct mbox *mb = sc->sc_inbox;
	struct soft_ccb *sccb = sc->sc_nextsccb;
	time_t now = time.tv_sec;
	int i, op;
	int s;

	/* did we miss an MBIF interrupt? */
	while ((mb = scanmbi(sc, mb))->mb_status != MBOX_I_FREE) {
		s = splbio();	/* avoid races */
		if (mb->mb_status != MBOX_I_FREE) {
			++aha_missed_mbif;
			sc->sc_inbox = mb;
			ahainbox(sc);
		}
		splx(s);
	}

	/*
	 * Scan sccb list, look for stale entries.
	 * If a read or write entry hasn't been serviced in a reasonable
	 * amount of time, abort the transaction.
	 */
	for (i = 0; i < sc->sc_sccb_count; ++i) {
		op = sccb->sccb_ccb.ccb_cdb6.cdb_cmd;    
		if (sccb->sccb_ccb.ccb_opcode != CCB_FREE &&
		    now - sccb->sccb_stamp > 2 * AHA_WATCH_INTERVAL &&
		    (op == CMD_READ10 || op == CMD_WRITE10 ||
		     op == CMD_READ || op == CMD_WRITE)) {
		    	s = splbio();
		    	if (sccb->sccb_ccb.ccb_opcode != CCB_FREE &&
			    now - sccb->sccb_stamp > 2 * AHA_WATCH_INTERVAL) {
				u_long ccb_phys = sc->sc_sccb_phys +
				    ((u_long)sccb - (u_long)sc->sc_sccb);

#ifdef DIAGNOSTIC
				printf("%s: command timed out\n",
				    sc->sc_hba.hba_dev.dv_xname);
#endif
				++aha_aborted_command;
				ahaccb(sc, sccb, 0, MBOX_O_ABORT);
			}
			splx(s);
		}
		if (++sccb >= sc->sc_sccb + sc->sc_sccb_count)
			sccb = sc->sc_sccb;
	}

	timeout(ahawatch, sc, AHA_WATCH_INTERVAL * hz);
	return (0);
}

/*
 * Floppy DMA buffers are so small that 11 microseconds of delay
 * (the default) can cause a data late.
 */
u_char aha_bus_on_time = 8;
u_char aha_bus_off_time = 4;

static void
set_burst_mode(sc)
	struct aha_softc *sc;
{
	int aha = sc->sc_port;

	ahafastcmd(aha, AHA_BUS_ON_TIME, &aha_bus_on_time, (u_char *)0);
	ahafastcmd(aha, AHA_BUS_OFF_TIME, &aha_bus_off_time, (u_char *)0);
}

/*
 * Allocate the bounce map.
 */
static void
bounce_alloc(sc, targ)
	struct aha_softc *sc;
	int targ;
{
	struct aha_bounce *bo, *bo_end;
	vm_offset_t buf;

	MALLOC(bo, struct aha_bounce *, MAXSG * sizeof *bo, M_DEVBUF, M_WAITOK);
	MALLOC(buf, vm_offset_t, MAXSG * NBPG, M_DEVBUF, M_WAITOK);
	sc->sc_bo[targ] = bo;
	for (bo_end = bo + MAXSG; bo < bo_end; ++bo, buf += NBPG) {
		bo->bo_v = buf;
		bo->bo_p = pmap_extract(pmap_kernel(), bo->bo_v);
		if (bo->bo_p >= ISA_MAXADDR) {
			printf("%s: tg%d unusable, can't create bounce map\n",
			    sc->sc_hba.hba_dev.dv_xname, targ);
			/* panic("bounce_alloc"); */
			break;
		}
		bo->bo_dst = 0;
		bo->bo_len = 0;
	}
}

void
ahaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct aha_softc *sc = (struct aha_softc *)self;
	struct isa_attach_args *args = aux;
	struct soft_ccb *sccb;
	int aha = args->ia_iobase;
	caddr_t va;
	u_long pa;
	size_t bytes;
	int irq, drq;
	int targ;
	u_char in[20];
	u_char out[20];
	u_char tgmap[8];
	int smallfloppyfifo = 1;
	int sync_base = 200;
#ifdef DIAGNOSTIC
	int first = 1;
#endif

	sc->sc_port = aha;

	ahafastcmd(aha, AHA_INQUIRY, (u_char *)0, in);
	switch (in[0]) {
	case 0:
		printf(": Adaptec AHA-1540/16 (unsupported)");
		break;
	case '0':
		printf(": Adaptec AHA-1540 (unsupported)");
		break;
	case 'A':
		if (in[1] == 'A') {
			if (args->ia_drq != NO_DRQ)
				printf(": Bustek BT-542B");
			else
				printf(": Bustek BT-445S");
		} else
			printf(": Adaptec AHA-1540B/1542B");
		break;
	case 'B':
		printf(": Adaptec AHA-1640 (unsupported)");
		break;
	case 'C':
		printf(": Adaptec AHA-1740A/1742A");
		smallfloppyfifo = 0;
		break;
	case 'D':
		printf(": Adaptec AHA-1540C/1542C");
		smallfloppyfifo = 0;
		sync_base = 100;
		break;
	case 'E':
		printf(": Adaptec AHA-1540CF/1542CF");
		smallfloppyfifo = 0;
		sync_base = 100;
		break;
	default:
		printf(": unknown type %c%c Adaptec card", in[0], in[1]);
		break;
	}
	printf(" rev %c%c", in[2], in[3]);

	/* XXX we really only need the SCSI id, but as long as we're here */
	ahafastcmd(aha, AHA_CONFIG_DATA, (u_char *)0, in);
	switch (in[1]) {
	case 0x40:	irq = IRQ15; break;
	case 0x20:	irq = IRQ14; break;
	case 0x08:	irq = IRQ12; break;
	case 0x04:	irq = IRQ11; break;
	case 0x02:	irq = IRQ10; break;
	case 0x01:	irq = IRQ9; break;
	default:	irq = -1; break;
	}
	if (irq < 0)
		printf(" irq code 0x%x", in[1]);
#ifdef DEBUG
	else {
		if (irq != args->ia_irq)
			panic("ahaattach irq");
	}
#endif
	switch (in[0]) {
	case 0x80:	drq = 7; break;
	case 0x40:	drq = 6; break;
	case 0x20:	drq = 5; break;
	case 0x01:	drq = 0; break;
	case 0x00:	drq = NO_DRQ; break;	/* No DMA.  Local Bus? */
	default:	drq = -1; break;
	}
	if (drq < 0)
		printf(" drq code 0x%x", in[0]);
#ifdef DEBUG
	else {
		if (drq != args->ia_drq && drq != NO_DRQ)
			panic("ahaattach drq");
	}
#endif
	sc->sc_id = in[2];
	printf("\n");

	/*
	 * Program the mailbox lock on the 1542C and late-model 1542Bs.
	 */
	if (ahafastcmd(aha, AHA_GET_LOCK, (u_char *)0, in) == 0) {
#ifdef DIAGNOSTIC
		if (in[0] & 0x8)
			printf("%s: extended BIOS translation enabled\n",
			    sc->sc_hba.hba_dev.dv_xname);
#endif
		if (in[1]) {
			out[0] = 0;
			out[1] = in[1];
			if (ahafastcmd(aha, AHA_SET_LOCK, out, (u_char *)0))
				panic("ahaattach clear mbox lock");
#ifdef DIAGNOSTIC
			printf("%s: unlocked\n", sc->sc_hba.hba_dev.dv_xname);
#endif
		}
	}

	/*
	 * Link into isa and set interrupt handler.
	 */
	isa_establish(&sc->sc_isa, &sc->sc_hba.hba_dev);
	sc->sc_ih.ih_fun = ahaintr;
	sc->sc_ih.ih_arg = sc;
	intr_establish(irq, &sc->sc_ih, DV_DISK);

	/*
	 * Allocate space for mailboxes and SCCBs.
	 * These need to be contiguous because they are accessed
	 * with physical addresses from the card's bus master DMA.
	 * XXX dynamically configure sc_mbox_count and sc_sccb_count
	 */
	bytes = NMBOX * 2 * sizeof (struct mbox) +
	    NSCCB * sizeof (struct soft_ccb);
	if (bytes > NBPG)
		panic("ahaattach sizes");
	MALLOC(va, caddr_t, bytes, M_DEVBUF, M_WAITOK);
	bzero(va, bytes);
	pa = pmap_extract(pmap_kernel(), (vm_offset_t)va);

	sc->sc_mbox = (struct mbox *)va;
	sc->sc_mbox_phys = pa;
	sc->sc_mbox_count = NMBOX;
	sc->sc_outbox = sc->sc_mbox;
	sc->sc_inbox = sc->sc_mbox + sc->sc_mbox_count;
	va += sc->sc_mbox_count * 2 * sizeof (struct mbox);
	pa += sc->sc_mbox_count * 2 * sizeof (struct mbox);

	sc->sc_sccb = (struct soft_ccb *)va;
	sc->sc_sccb_phys = pa;
	sc->sc_sccb_count = NSCCB;
	sc->sc_nextsccb = sc->sc_sccb;
	sc->sc_gosccb = 0;
	for (sccb = sc->sc_sccb;
	     sccb < sc->sc_sccb + sc->sc_sccb_count;
	     ++sccb)
		sccb->sccb_ccb.ccb_opcode = CCB_FREE;

	/* negotiate bus master DMA for our DMA channel */
	/* XXX this is only needed if BIOS is disabled */
	if (drq != NO_DRQ)
		at_dma_cascade(drq);

	if (smallfloppyfifo)
		set_burst_mode(sc);

	sc->sc_hba.hba_driver = &ahahbadriver;

	ahareset(sc);

	printf("%s: delaying to permit certain devices to finish self-test",
	    sc->sc_hba.hba_dev.dv_xname);
	DELAY(5000000);
	printf("\n");

#ifdef DIAGNOSTIC
	/*
	 * Print out several host adapter parameters
	 * that are primarily of interest in debugging.
	 */
	out[0] = 17;
	ahafastcmd(aha, AHA_SETUP_DATA, out, in);
	printf("%s:", sc->sc_hba.hba_dev.dv_xname);
	if (in[0] & 1) {
		printf(" synch negotiation");
		first = 0;
	}
	if (in[0] & 2) {
		printf("%s parity", first ? "" : ",");
		first = 0;
	}
	printf("%s bus transfer ", first ? "" : ",");
	/* XXX custom values for standard numbers are guesses */
	/* XXX values of 99 and ff from observation on 1542CF */
  	switch (in[1]) {
	case 0xff:			printf("3.3 Mb/s"); break;
	case 0: case 0xbb:		printf("5 Mb/s");   break;
	case 1:	case 0xa2: case 0x99:	printf("6.7 Mb/s"); break;
	case 2:	case 0x91:		printf("8 Mb/s");   break;
	case 3:	case 0x80:		printf("10 Mb/s");  break;
	case 4: case 0xaa:		printf("5.7 Mb/s"); break;
	default:
		printf("0x%x custom", in[1]); break;
	}
	printf(", bus on/off %d/%d us\n", in[2], in[3]);
#endif

	ahafastcmd(aha, AHA_INSTALLED_DEVS, (u_char *)0, tgmap);
	for (targ = 0; targ < 8; ++targ)
		if (tgmap[targ]) {
			SCSI_FOUNDTARGET(&sc->sc_hba, targ);
			if (maxmem * NBPG >= ISA_MAXADDR)
				bounce_alloc(sc, targ);
#ifdef DIAGNOSTIC
			/*
			 * Ideally this would appear earlier,
			 * but synch negotiation won't occur without
			 * data transfer and it's simple to let
			 * target attach routines do this for us.
			 */
			out[0] = 17;
			ahafastcmd(aha, AHA_SETUP_DATA, out, in);
			if ((in[targ + 8] & 0xf) == 0 &&
			    (in[16] & 1 << targ) == 0)
				continue;
			printf("%s:",
			    sc->sc_hba.hba_targets[targ]->t_dev.dv_xname);
			first = 1;
			if (in[targ + 8] & 0xf) {
			    printf(" synch transfer period %d ns, offset %d",
				(in[targ + 8] >> 4 & 7) * 50 + sync_base,
				(in[targ + 8] & 0xf));
			    switch ((in[targ + 8] >> 4 & 7) + sync_base / 50) {
				case 2:  printf(" (10 Mb/s)");   break;
				case 3:  printf(" (6.67 Mb/s)"); break;
				case 4:  printf(" (5 Mb/s)");    break;
				case 5:  printf(" (4 Mb/s)");    break;
				case 6:  printf(" (3.33 Mb/s)"); break;
				case 7:  printf(" (2.86 Mb/s)"); break;
				case 8:  printf(" (2.50 Mb/s)"); break;
				case 9:  printf(" (2.22 Mb/s)"); break;
				case 10: printf(" (2 Mb/s)");    break;
				case 11: printf(" (1.81 Mb/s)"); break;
				default: printf(" (?? Mb/s)");
			    }
			}
			if (in[16] & 1 << targ) {
				if (in[targ + 8] & 0xf)
					printf(",");
				printf(" disconnect off");
			}
			printf("\n");
#endif
		}

	/* XXX if there's only one target, disable disconnection option? */
	/* XXX how about dynamically attached units? */

	timeout(ahawatch, sc, AHA_WATCH_INTERVAL * hz);
}

static void
ahareset(sc)
	struct aha_softc *sc;
{
	int aha = sc->sc_port;
	u_char out[4], *op = out;

	outb(AHA_STAT(aha), AHA_C_SRST);
	DELAY(100000);	/* for the 1542C */
	while ((inb(AHA_STAT(aha)) & (AHA_S_INIT|AHA_S_IDLE)) !=
	    (AHA_S_INIT|AHA_S_IDLE))
		;

	*op++ = sc->sc_mbox_count;
	host_to_aha(sc->sc_mbox_phys, op);
	if (ahafastcmd(aha, AHA_MBOX_INIT, out, (u_char *)0))
		panic("anareset mbox init failed");

	/* XXX do we need to explicitly disable MBOA interrupts? */
}

void
ahahbareset(hba, resetunits)
	struct hba_softc *hba;
	int resetunits;
{
	struct aha_softc *sc = (struct aha_softc *)hba;

	ahareset(sc);

	if (resetunits)
		scsi_reset_units(hba);
}

/*
 * Allocate an outbound bounce page, copying data if necessary.
 */
static vm_offset_t
bounce_out(sc, sccb, indx, v, len, rw)
	struct aha_softc *sc;
	struct soft_ccb *sccb;
	vm_offset_t v;
	int indx, rw;
	vm_size_t len;
{
	struct aha_bounce *bo =
	    &sc->sc_bo[CCB_C_TARGET(sccb->sccb_ccb.ccb_control)][indx];
	vm_size_t o = v & PGOFSET;

	if (rw == B_READ) {
		bo->bo_dst = v;
		bo->bo_len = len;
		++sccb->sccb_bounces;
	} else
		bcopy((void *)v, (void *)(bo->bo_v + o), len);
	return (bo->bo_p + o);
}

/*
 * Finish up bouncing in on a read.
 */
static void
bounce_in(sc, sccb)
	struct aha_softc *sc;
	struct soft_ccb *sccb;
{
	vm_offset_t dst;
	struct aha_bounce *bo;

	for (bo = &sc->sc_bo[CCB_C_TARGET(sccb->sccb_ccb.ccb_control)][0];
	    sccb->sccb_bounces > 0; ++bo)
		if (dst = bo->bo_dst) {
			bcopy((void *)(bo->bo_v + (dst & PGOFSET)),
			    (void *)dst, bo->bo_len);
			bo->bo_dst = 0;
			--sccb->sccb_bounces;
		}
}

/*
 * Given a buffer and a length, set up a scatter/gather map
 * in a ccb to map the buffer for a SCSI transfer.
 * Initialize the CCB opcode for the correct transfer type.
 */
static void
sginit(sc, sccb, buf, len, rw)
	struct aha_softc *sc;
	struct soft_ccb *sccb;
	caddr_t buf;
	int len, rw;
{
	struct sg *sg = &sccb->sccb_sg[0];
	vm_size_t pages;
	vm_offset_t start = i386_trunc_page(buf);
	vm_offset_t end = i386_round_page(buf + len);
	vm_offset_t v = (vm_offset_t)buf;
	vm_offset_t p;
	struct ccb *ccb = &sccb->sccb_ccb;

	if (len <= 0) {
		ccb->ccb_opcode = CCB_CMD_RDL;
		ccb->ccb_data[0] = 0;
		ccb->ccb_data[1] = 0;
		ccb->ccb_data[2] = 0;
		ccb->ccb_datalen[0] = 0;
		ccb->ccb_datalen[1] = 0;
		ccb->ccb_datalen[2] = 0;
		return;
	}

	if (start + NBPG == end) {
		/* transfer lies entirely within a page */
		if ((p = pmap_extract(pmap_kernel(), v)) >= ISA_MAXADDR)
			p = bounce_out(sc, sccb, 0, v, (vm_size_t)len, rw);
		ccb->ccb_opcode = CCB_CMD_RDL;
		host_to_aha(p, ccb->ccb_data);
		host_to_aha((u_long)len, ccb->ccb_datalen);
		return;
	}

	/* transfer too big -- we'll need a scatter/gather map */
	if ((pages = i386_btop(end - start)) > 17)
		panic("sginit pages");

	ccb->ccb_opcode = CCB_CMD_SG_RDL;
	host_to_aha(((vm_offset_t)sg - (vm_offset_t)sc->sc_sccb) +
	    sc->sc_sccb_phys, ccb->ccb_data);
	host_to_aha(pages * sizeof *sg, ccb->ccb_datalen);

	end = v + len;
	do {
		start = v;
		v = MIN(i386_trunc_page(start + NBPG), end);
		if ((p = pmap_extract(pmap_kernel(), start)) >= ISA_MAXADDR)
			p = bounce_out(sc, sccb, sg - &sccb->sccb_sg[0],
			    start, v - start, rw);
		host_to_aha(v - start, sg->sg_len);
		host_to_aha(p, sg->sg_addr);
		++sg;
	} while (v < end);
}

/*
 * Allocate and initialize a software CCB.
 * CCBs currently all live in a single page,
 * to make it simple to find their physical addresses.
 * Must be called at splbio().
 * XXX we wouldn't need splbio() if we didn't call this from ahaintr()!
 */
static struct soft_ccb *
sccballoc(sc, cdb)
	struct aha_softc *sc;
	struct scsi_cdb *cdb;
{
	struct soft_ccb *sccb = sc->sc_nextsccb;
	int i;

	for (i = 0; i < sc->sc_sccb_count; ++i) {
		if (sccb->sccb_ccb.ccb_opcode == CCB_FREE)
			break;
		if (++sccb >= sc->sc_sccb + sc->sc_sccb_count)
			sccb = sc->sc_sccb;
	}
	if (sccb->sccb_ccb.ccb_opcode != CCB_FREE)
		panic("sccballoc");
	sccb->sccb_stamp = time.tv_sec;
	sccb->sccb_ccb.ccb_opcode = CCB_CMD_SG_RDL;
	sc->sc_nextsccb = sccb + 1;
	if (sc->sc_nextsccb >= sc->sc_sccb + sc->sc_sccb_count)
		sc->sc_nextsccb = sc->sc_sccb;

	if (cdb) {
		if (SCSICMDLEN(cdb->cdb_bytes[0]) == 6)
			sccb->sccb_ccb.ccb_cdb6 = *CDB6(cdb);
		else if (SCSICMDLEN(cdb->cdb_bytes[0]) == 10)
			sccb->sccb_ccb.ccb_cdb10 = *CDB10(cdb);
		else
			panic("sccballoc cdb size");
	}

	return (sccb);
}

/*
 * Initialize (most of) an SCCB.
 * We assume that the CDB has already been set up.
 */
static void
sccbinit(sc, sccb, targ, intr, dev, buf, len, rw)
	struct aha_softc *sc;
	struct soft_ccb *sccb;
	int targ;
	scintr_fn intr;
	struct device *dev;
	caddr_t buf;
	int len, rw;
{
	struct ccb *ccb = &sccb->sccb_ccb;

	/* XXX do we need direction bits if we always check residual count? */
	ccb->ccb_control = CCB_CONTROL(targ, 0, 0, ccb->ccb_cdbbytes[1] >> 5);

	ccb->ccb_cdblen = SCSICMDLEN(ccb->ccb_cdbbytes[0]);

	sginit(sc, sccb, buf, len, rw);

	/* XXX when we permit multiple commands per target, must change this */
	ccb->ccb_rqslen = 1;	/* disable automatic request sense */
	ccb->ccb_rsv1 = 0;	/* XXX necessary? */
	ccb->ccb_rsv2 = 0;	/* XXX necessary? */

	sccb->sccb_intr = intr;
	sccb->sccb_intrdev = dev;
}

/*
 * Fire off a CCB.
 * If synch is set, poll for completion.
 * Must be called at splbio().
 */
static int
ahaccb(sc, sccb, synch, mbcmd)
	struct aha_softc *sc;
	struct soft_ccb *sccb;
	int synch;
	int mbcmd;
{
	struct ccb *ccb = &sccb->sccb_ccb;
	u_long ccb_phys = sc->sc_sccb_phys +
	    ((u_long)ccb - (u_long)sc->sc_sccb);
	struct mbox *mb;
	int aha = sc->sc_port;
	int status;

	if ((mb = scanmbo(sc, sc->sc_outbox)) == NULL)
		/* if we run out of mailboxes here, it's big trouble */
		panic("ahaccb scanmbo");
	sc->sc_outbox = mb + 1;
	if (sc->sc_outbox >= sc->sc_mbox + sc->sc_mbox_count)
		sc->sc_outbox = sc->sc_mbox;
	host_to_aha(ccb_phys, mb->mb_ccb);
	mb->mb_cmd = mbcmd;

	/* start the adapter's outbox scan */
	sc->sc_istat = inb(AHA_INTR(aha));	/* XXX should do more */
	outb(AHA_STAT(aha), AHA_C_IRST);
	while (inb(AHA_STAT(aha)) & AHA_S_CDF)
		;
	outb(AHA_DATA(aha), AHA_START_SCSI_CMD);

	if (!synch)
		return (0);

	/* wait for notification */
	while ((inb(AHA_INTR(aha)) & AHA_I_ANY) == 0)
		;
	sc->sc_istat = inb(AHA_INTR(aha));
	outb(AHA_STAT(aha), AHA_C_IRST);
	if ((sc->sc_istat & AHA_I_MBIF) == 0)
		panic("ahaccb missing mbif");

	/* scan for the inbox */
	/* XXX what do we do if the command hangs? */
	mb = sc->sc_inbox;
	for(;;) {
		mb = scanmbi(sc, mb);
		if (mb->mb_status == MBOX_I_FREE)
			continue;
		if (aha_to_host(mb->mb_ccb) == ccb_phys)
			break;
		if (++mb >= sc->sc_mbox + 2 * sc->sc_mbox_count)
			mb = sc->sc_mbox + sc->sc_mbox_count;
	}
	sc->sc_resid = aha_to_host(ccb->ccb_datalen);
	if ((sc->sc_stat = ccb->ccb_hastat) == CCB_H_NORMAL)
		status = ccb->ccb_tarstat;
	else if (ccb->ccb_cdbbytes[0] == CMD_TEST_UNIT_READY)
		/* probing, don't make noise */
		status = -1;
	else {
		printf("%s: host adapter error 0x%x on immediate command\n",
		    sc->sc_hba.hba_dev.dv_xname, ccb->ccb_hastat);
		/* XXX panic? */
		status = -1;
	}
	mb->mb_status = MBOX_I_FREE;
	if (sc->sc_gosccb == sccb)
		sc->sc_gosccb = 0;
	if (sccb->sccb_bounces > 0)
		bounce_in(sc, sccb);
	ccb->ccb_opcode = CCB_FREE;

	return (status);
}

int
ahaicmd(hba, targ, cdb, buf, len, rw)
	struct hba_softc *hba;
	int targ;
	struct scsi_cdb *cdb;
	caddr_t buf;
	int len, rw;
{
	struct aha_softc *sc = (struct aha_softc *)hba;
	struct soft_ccb *sccb;
	int s, error;

	s = splbio();    
	sccb = sccballoc(sc, cdb);
	splx(s);
	sccbinit(sc, sccb, targ, (scintr_fn)NULL, (struct device *)NULL,
	    buf, len, rw);
	s = splbio();    
	error = ahaccb(sc, sccb, 1, MBOX_O_START);
	splx(s);
	return (error);
}

int
ahadump(hba, targ, cdb, buf, len, isphys)
	struct hba_softc *hba;
	int targ;
	struct scsi_cdb *cdb;
	caddr_t buf;
	int len, isphys;
{
	struct aha_softc *sc = (struct aha_softc *)hba;
	struct soft_ccb *sccb;
	struct ccb *ccb;
	u_long from, to, lastfrom;

	if (!isphys)
		panic("ahadump virtual!?\n");

	/*
	 * Several assumptions:
	 * +	The upper-level dump code always calls us on aligned
	 *	2^n chunks which never cross 64 KB physical memory boundaries
	 * +	We're running in virtual mode with interrupts blocked
	 */

	sccb = sccballoc(sc, cdb);
	ccb = &sccb->sccb_ccb;

	ccb->ccb_opcode = CCB_CMD_RDL;
	ccb->ccb_control = CCB_CONTROL(targ, 0, 0, ccb->ccb_cdbbytes[1] >> 5);
	ccb->ccb_cdblen = SCSICMDLEN(ccb->ccb_cdbbytes[0]);
	ccb->ccb_rqslen = 1;	/* disable automatic request sense */
	if ((vm_offset_t)buf >= ISA_MAXADDR) {
		/* assumes len <= MAXSG * NBPG and contiguous bounce pages */
		from = btoc((u_long)buf);
		to = btoc(sc->sc_bo[targ][0].bo_p);
		lastfrom = btoc((vm_offset_t)&buf[len]);
		while (from < lastfrom)
			physcopyseg(from++, to++);
		buf = (caddr_t)sc->sc_bo[targ][0].bo_p;
	}
	host_to_aha((u_long)buf, ccb->ccb_data);
	host_to_aha((u_long)len, ccb->ccb_datalen);

	sccb->sccb_intr = NULL;
	sccb->sccb_intrdev = 0;

	return (ahaccb(sc, sccb, 1, MBOX_O_START));
}

/*
 * Start a transfer.
 *
 * Since the AHA-154x handles transactions in parallel (with disconnect /
 * reconnect), we don't have to queue requests for the host adapter.
 *
 * This code is NOT re-entrant: (*dgo)() must call ahago() without calling
 * ahastart() first (because of the sc_gosccb kluge).
 */
void
ahastart(self, sq, bp, dgo, dev)
	struct device *self;
	struct sq *sq;
	struct buf *bp;
	scdgo_fn dgo;
	struct device *dev;
{
	struct aha_softc *sc = (struct aha_softc *)self;

	if (bp) {
		/* asynch transaction */
		sc->sc_gosccb = sccballoc(sc, (struct scsi_cdb *)NULL);
		(*dgo)(dev,
		    (struct scsi_cdb *)&sc->sc_gosccb->sccb_ccb.ccb_cdb);
		return;
	}
	/* let ahaicmd() allocate its own sccb */
	(*dgo)(dev, (struct scsi_cdb *)NULL);
}

/*
 * Get the host adapter going on a command.
 *
 * XXX should we allocate the DMA channel here, rather than dedicate one?
 * XXX must be called at splbio() since interrupts can call it
 * XXX but it'd be much better for the work to get done at low ipl!
 */
int
ahago(self, targ, intr, dev, bp, pad)
	struct device *self;
	int targ;
	scintr_fn intr;
	struct device *dev;
	struct buf *bp;
	int pad;
{
	struct aha_softc *sc = (struct aha_softc *)self;

	sccbinit(sc, sc->sc_gosccb, targ, intr, dev,
	    bp->b_un.b_addr, bp->b_bcount, bp->b_flags & B_READ);
	return (ahaccb(sc, sc->sc_gosccb, 0, MBOX_O_START));
}

static void
ahaerror(sc, err)
	struct aha_softc *sc;
	char *err;
{

	printf("%s: %s (target %d, lun %d)\n",
	    sc->sc_hba.hba_dev.dv_xname, err, sc->sc_targ, sc->sc_lun);
}

static void
ahaerror2(sc, err, x)
	struct aha_softc *sc;
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
ahainbox(sc)
	struct aha_softc *sc;
{
	struct mbox *mbi = sc->sc_inbox;
	struct soft_ccb *sccb = AHA_CCB_TO_SCCB(sc, mbi->mb_ccb);
	struct ccb *ccb = &sccb->sccb_ccb;
	struct sq *sq;
	int status = -1;

	sc->sc_hba.hba_intr = sccb->sccb_intr;
	sc->sc_hba.hba_intrdev = sccb->sccb_intrdev;
	sc->sc_resid = aha_to_host(ccb->ccb_datalen);
	sc->sc_targ = CCB_C_TARGET(ccb->ccb_control);
	sc->sc_lun = CCB_C_LUN(ccb->ccb_control);
	sc->sc_stat = ccb->ccb_hastat;

	switch (mbi->mb_status) {

	case MBOX_I_ERROR:

		switch (ccb->ccb_hastat) {
		case CCB_H_NORMAL:
			status = ccb->ccb_tarstat;
			if ((status & STS_MASK) == STS_GOOD)
				ahaerror(sc,
				    "no host adapter detected error???");
			break;
		case CCB_H_TIMEOUT:
			ahaerror(sc, "target selection timeout");
			break;
		case CCB_H_OVERRUN:
			ahaerror(sc, "data overrun or underrun");
			/* XXX should we actually report this as an error? */
			break;
		case CCB_H_BUSFREE:
			ahaerror(sc, "target unexpectedly freed the SCSI bus");
			break;
		case CCB_H_PHASE:
			ahaerror(sc, "target bus phase sequence error");
			/* we should see a SCSI bus reset after this one */
			break;
		case CCB_H_INVCCB:
			ahaerror2(sc, "invalid ccb opcode", ccb->ccb_opcode);
			break;
		case CCB_H_LINKLUN:
			ahaerror(sc, "linked ccb doesn't have same LUN");
			break;
		case CCB_H_INVTDIR:
			ahaerror(sc, "invalid direction in target mode");
			break;
		case CCB_H_DUPCCB:
			ahaerror(sc, "duplicate ccb received in target mode");
			break;
		case CCB_H_INVPARM:
			ahaerror(sc, "invalid ccb or segment list parameter");
			/* XXX dump the ccb? */
			break;
		default:
			ahaerror2(sc, "unknown host adapter error",
			    ccb->ccb_hastat);
			break;
		}
		break;

	case MBOX_I_COMPLETED:
		status = ccb->ccb_tarstat;
		break;

	case MBOX_I_ABORTED:
		break;

	case MBOX_I_ABORT_FAILED:
		if (ccb->ccb_opcode == CCB_FREE) {
			sc->sc_inbox->mb_status = MBOX_I_FREE;
			if (++sc->sc_inbox >=
			    sc->sc_mbox + 2 * sc->sc_mbox_count)
				sc->sc_inbox = sc->sc_mbox + sc->sc_mbox_count;
			return;
		}
		ahaerror(sc, "host adapter command abort failed");
		break;

	default:
		printf("ahainbox: bad mbox status %x\n", mbi->mb_status);
		panic("ahainbox");
		break;
	}

	/* free up resources */
	sc->sc_inbox->mb_status = MBOX_I_FREE;
	if (sc->sc_gosccb == sccb)
		sc->sc_gosccb = 0;
	if (sccb->sccb_bounces > 0)
		bounce_in(sc, sccb);
	ccb->ccb_opcode = CCB_FREE;

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
			sc->sc_gosccb = sccballoc(sc, (struct scsi_cdb *)NULL);
			(*sq->sq_dgo)(sq->sq_dev, (struct scsi_cdb *)
			    &sc->sc_gosccb->sccb_ccb.ccb_cdb);
		}
	}

	/* increment to the next mailbox */
	if (++sc->sc_inbox >= sc->sc_mbox + 2 * sc->sc_mbox_count)
		sc->sc_inbox = sc->sc_mbox + sc->sc_mbox_count;
}

int
ahaintr(sc0)
	void *sc0;
{
	struct aha_softc *sc = sc0;

	sc->sc_istat = inb(AHA_INTR(sc->sc_port));
	outb(AHA_STAT(sc->sc_port), AHA_C_IRST);

	if ((sc->sc_istat & AHA_I_ANY) == 0) {
		printf("%s: stray interrupt\n", sc->sc_hba.hba_dev.dv_xname);
		return (0);
	}

	if (sc->sc_istat & AHA_I_HACC)
		if (inb(AHA_STAT(sc->sc_port)) & AHA_S_INVDCMD) {
			printf("%s: invalid host adapter command\n",
			    sc->sc_hba.hba_dev.dv_xname);
			panic("ahaintr HACC");
		} else
			printf("%s: interrupt for immediate command\n",
			    sc->sc_hba.hba_dev.dv_xname);

	if (sc->sc_istat & AHA_I_SCRD)
		printf("%s: SCSI bus reset\n", sc->sc_hba.hba_dev.dv_xname);

	if (sc->sc_istat & AHA_I_MBOA)
		printf("%s: mailbox out available\n",
		    sc->sc_hba.hba_dev.dv_xname);

	if (sc->sc_istat & AHA_I_MBIF)
		sc->sc_inbox = scanmbi(sc, sc->sc_inbox);

	while (sc->sc_inbox->mb_status != MBOX_I_FREE)
		ahainbox(sc);

	return (1);
}

void
aharel(self)
	struct device *self;
{
	struct aha_softc *sc = (struct aha_softc *)self;
	struct soft_ccb *sccb;
	struct sq *sq;

	if (sccb = sc->sc_gosccb) {
		if (sc->sc_gosccb == sccb)
			sc->sc_gosccb = 0;
		sccb->sccb_ccb.ccb_opcode = CCB_FREE;
	}
	if ((sq = sc->sc_hba.hba_head) != NULL) {
		sc->sc_gosccb = sccballoc(sc, (struct scsi_cdb *)NULL);
		(*sq->sq_dgo)(sq->sq_dev,
		    (struct scsi_cdb *)&sc->sc_gosccb->sccb_ccb.ccb_cdb);
	}
}
