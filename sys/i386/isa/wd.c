/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: wd.c,v 1.37 1994/01/30 01:56:32 karels Exp $
 */
 
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)wd.c	7.2 (Berkeley) 5/9/91
 */

/*
 * ST506/RLL/IDE/ESDI disk driver for Western Digital 1002 style controllers
 */

#include "param.h"
#include "dkbad.h"
#include "systm.h"
#include "conf.h"
#include "file.h"
#include "stat.h"
#include "ioctl.h"
#include "disklabel.h"
#include "buf.h"
#include "uio.h"
#include "device.h"
#include "disk.h"
#include "syslog.h"
#include "vm/vm.h"

#include "machine/bootblock.h"
#include "machine/bootparam.h"
#include "icu.h"
#include "isavar.h"
#include "wdreg.h"

#define	MAXDRV		2		/* per controller */

#define	RETRIES		5	/* number of retries before giving up */
#define	TIMEOUT		10	/* seconds for timeout (for laptops) */

#define wdunit(dev)	(minor(dev) >> 3)
#define wdpart(dev)	(minor(dev) & 0x07)
#define	wdminor(unit,part)	(((unit) << 3) | (part))
#define RAWPART		2	/* 'c' partition */	/* XXX */

#define b_cylin	b_resid		/* cylinder number for doing IO to */
				/* shares an entry in the buf struct */

#define	WDDEBUG
#ifdef	WDDEBUG
int	wddebug = 0;
#define	dprintf(x)	{ if (wddebug) printf x; }
#else
#define	dprintf(x)
#endif

/* per-controller state */
struct wdc_softc {
	struct	device wdc_dev;  	/* base device */
	struct	isadev wdc_id;   	/* ISA device */
	struct	intrhand wdc_ih;	/* interrupt vectoring */
	int	wdc_iobase;		/* I/O port base */
	int	wdc_flags;		/* see below */
	struct	buf wdc_tab;		/* head of controller queue */
};

/* wdc_flags: */
#define	WDC_SETCTLR	0x1	/* XXX just did wdsetctlr */
#define	WDC_LOCKED	0x2	/* locked for direct controller access */
#define	WDC_WAIT	0x4	/* someone needs direct controller access */
#define	WDC_TIMEDOUT	0x8	/* simulated interrupt after timeout */

/*
 * Drive states.  Used for open and format operations.
 * States < OPEN (> 0) are transient, during an open operation.
 * OPENRAW is used for unlabeled disks to inhibit partition
 * bounds checking.
 * XXX should use DK_* states in disk.h, but those miss RECAL, RDBADTBL
 */
#define RAWDISK		0x08		/* raw disk operation, no label */
#define ISRAWSTATE(s)	(RAWDISK&(s))	/* are we in a raw state? */
#define DISKSTATE(s)	(~(REINIT|RAWDISK)&(s)) /* basic state of state mach. */
#define REINIT		0x10		/* doing reinitialization sequence */

#define	CLOSED		0		/* disk is closed. */
					/* "cooked" disk states */
#define	WANTOPEN	1		/* open requested, not started */
#define	RECAL		2		/* doing restore */
#define	RDLABEL		3		/* reading pack label */
#define	RDBADTBL	4		/* reading bad-sector table */
#define	OPEN		5		/* done with open */

#define	WANTOPENRAW	(WANTOPEN|RAWDISK)	/* raw WANTOPEN */
#define	RECALRAW	(RECAL|RAWDISK)	/* raw open, doing restore */
#define	OPENRAW		(OPEN|RAWDISK)	/* open, but unlabeled disk */

/* The max number of bad blocks -- kludge, yeah? */
#define NDKBAD	(sizeof(((struct dkbad *)0)->bt_bad)/sizeof(struct bt_bad))

/* End of bad block list -- should be greater than any valid block number */
#define EOBADLIST	0x7fffffff

#define	NCYLGRP		64	/* number of cyl groups for badsect lookup */

/*
 * state of a disk drive.
 */
struct wd_softc {
	struct	dkdevice wd_dk;  	/* base device */
	long	wd_bc;		/* byte count left */
	int	wd_nconts;	/* times to repeat sequential i/o */
	caddr_t	wd_addr;	/* user buffer address */
	daddr_t	wd_blknum;	/* number of the block to r/w */
	u_char	wd_unit;	/* physical unit number */
	u_char	wd_statusreg;	/* copy of status reg. (on errors) */
	u_char	wd_errorreg;	/* copy of error reg. (on errors) */
	int	wd_flags;	/* see below */
	int	wd_cylpergrp;	/* number of cylinders in cyl_badindx groups */
	struct	buf wd_tab;		/* head of drive queue */
	long	wd_bad[NDKBAD+1];	/* the list of bad blocks */
	/* using char for dk_badindx assumes a limit of <= 127 bad sectors... */
	char	wd_badindx[NCYLGRP];	/* indices into dk_bad for cyl grps */
#ifdef later
	short	wd_badindx[NCYLGRP];	/* indices into wd_bad for cyl grps */
#endif
};

/* redefinitions from dkdevice, pending implementation of generic driver */
#define	wd_state	wd_dk.dk_state
#define	wd_wlabel	wd_dk.dk_wlabel
#define	wd_labelsector	wd_dk.dk_labelsector
#define	wd_copenmask	wd_dk.dk_copenmask
#define	wd_bopenmask	wd_dk.dk_bopenmask
#define	wd_openmask	wd_dk.dk_openmask
#define	wd_dd		wd_dk.dk_label

/* from wd_softc, produce pointer to parent wdc_softc */
#define	wdcont(du)	((struct wdc_softc *)((du)->wd_dk.dk_dev.dv_parent))

/* wd_flags: */
#define	DK_OPSTARTED	0x01	/* continuing operation already started */
#define	DK_BADSRCH	0x02	/* search bad-sector table before trying I/O */
#define	DK_SEBYSE	0x04	/* doing I/O sector by sector */

struct wd_attach_args {
	int	wa_wdc;			/* controller port address */
	int	wa_unit;		/* unit number */
	int	wa_slave;		/* slave number */
};

int	wd_badsearch = 1;	/* force DK_BADSRCH on all controllers */
int	wd_timeout = TIMEOUT;

/*
 * This label is used as a default when initializing a new or raw disk.
 * This is used while reading the dos partition table and/or the disk label,
 * and thus must be set for maximum values.
 */
#define	DFL_CYL	1023
#define	DFL_TRK	5
#define	DFL_SEC	17

extern	int hz;
extern	struct biosgeom biosgeom[2];
#ifdef BSDGEOM
extern	struct bsdgeom bsdgeom;
#endif

int	wdcprobe __P((struct device *, struct cfdata *, void *));
void	wdcattach __P((struct device *, struct device *, void *));
int	wdprobe __P((struct device *, struct cfdata *, void *));
void	wdattach __P((struct device *, struct device *, void *));
int	wdcontrol __P((struct buf *));
int	wdintr __P((struct wdc_softc *));
static	void wdtimeout __P((void));
void	wdsetctlr __P((struct wd_softc *, int));
int	wdstrategy __P((struct buf *));
void	wdstart __P((struct wdc_softc *));
void	wdustart __P((struct wd_softc *));

struct cfdriver wdccd =
	{ NULL, "wdc", wdcprobe, wdcattach, sizeof(struct wdc_softc) };

struct cfdriver wdcd =
	{ NULL, "wd", wdprobe, wdattach, sizeof(struct wd_softc) };
 

/* ARGSUSED */
static int
wdprint(aux, name)
	void *aux;
	char *name;
{
	register struct wd_attach_args *wa = (struct wd_attach_args *) aux;

	if (name)
		printf(" at %s", name);
	printf(" slave %d", wa->wa_slave);
	return (UNCONF);
}

/*
 * Controller probe routine
 */
wdcprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int wdc = ia->ia_iobase;
	void wdforceintr();

	/* XXX sorry, needs to be better */
	outb(wdc+wd_cyl_lo, 0xa5);	/* check cyllo is writable */
	if (inb(wdc+wd_cyl_lo) != 0xa5)
		return (0);
	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(wdforceintr, (void *) &wdc);
		if (ffs(ia->ia_irq) - 1 == 0)
			return (0);
	}
	ia->ia_iosize = WD_NPORT;
	return (1);
}

/*
 * Tickle the card to make it tell us where it is for autoconfig.
 * This is done by reading in the label sector of drive 0.
 */
void
wdforceintr(pwdc)
	int *pwdc;
{
	int wdc = *pwdc;
	int cyloffset = 0;
#define	UNIT	0

	outb(wdc+wd_cyl_hi, DFL_CYL >> 8);
	outb(wdc+wd_cyl_lo, DFL_CYL);
	outb(wdc+wd_sdh, WDSD_IBM | (UNIT << 4) + DFL_TRK - 1);
	outb(wdc+wd_seccnt, DFL_SEC);
	outb(wdc+wd_command, WDCC_SETGEOM);

	outb(wdc+wd_precomp, 0xff);	/* sometimes this is head bit 3 */
	outb(wdc+wd_seccnt, 1);
	outb(wdc+wd_sector, LABELSECTOR + 1);
	outb(wdc+wd_cyl_lo, (cyloffset & 0xff));
	outb(wdc+wd_cyl_hi, (cyloffset >> 8));
	outb(wdc+wd_sdh, WDSD_IBM | (UNIT << 4));
	outb(wdc+wd_command, WDCC_READ);
#undef UNIT
}

/*
 * Attach controller.
 */
void
wdcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;	
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	register struct wdc_softc *sc = (struct wdc_softc *) self;
	int wdc = ia->ia_iobase;
	int i;

	sc->wdc_iobase = wdc;
	wdcreset(wdc);
	printf("\n");

	/*
	 * Search for slaves on controller. 
	 */
	config_search(wdprobe, self, aux);

	isa_establish(&sc->wdc_id, &sc->wdc_dev);
	sc->wdc_ih.ih_fun = wdintr;
	sc->wdc_ih.ih_arg = (void *) sc;
	intr_establish(ia->ia_irq, &sc->wdc_ih, DV_DISK);
}

wdcreset(wdc)
	int wdc;
{
	int i;

	outb(wdc+wd_ctlr, WDCTL_RESET | WDCTL_HEAD3ENB);
	DELAY(10000);
	outb(wdc+wd_ctlr, WDCTL_HEAD3ENB);	/* clear reset */
	i = 500;
	do {
		DELAY(10000);
	} while (inb(wdc+wd_status) & WDCS_BUSY && --i >= 0);
	if (i < 0)
		printf("wd controller hung???\n");
}

/* ARGSUSED */
wdprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	struct wd_attach_args wa;
	int slave = cf->cf_unit % MAXDRV;	/* XXX */
	int wdc = ia->ia_iobase;

	if (slave >= MAXDRV) {
		printf("wd%d: illegal slave number %d\n", cf->cf_unit, slave);
		return (0);
	}
	/*
	 * Select drive and test for READY to see whether drive exists.
	 * Nonexistent IDE drives return trash, so check that BUSY is off.
	 */
	outb(wdc+wd_sdh, WDSD_IBM | (slave << 4));
	DELAY(10000);
	if ((inb(wdc+wd_status) & (WDCS_BUSY|WDCS_READY)) == WDCS_READY) {
		wa.wa_unit = cf->cf_unit;
		wa.wa_slave = slave;
		config_attach(parent, cf, &wa, wdprint);
		return (1);
	}
	return (0);		/* didn't find ready drive */
}

/* ARGSUSED */
void
wdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;	
{	
	register struct wd_softc *sc = (struct wd_softc *) self;
	register struct wd_attach_args *wa = (struct wd_attach_args *) aux;

	printf("\n");
	sc->wd_unit = wa->wa_slave;
	disk_attach(&sc->wd_dk);
	if (wd_badsearch)
		sc->wd_flags |= DK_BADSRCH;
}

/*
 * Initialize a drive.
 */
wdopen(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	register unsigned int unit;
	register struct buf *bp;
	register struct wd_softc *du;
	dev_t rawdev;
	struct dkbad *db;
	int i, s, error = 0, use_fdisk;
	struct mbpart dospart;
	daddr_t labelsect;
	char *msg;
	static int timer_started = 0;
	extern int cold;
	struct biosgeom *bgp;

	unit = wdunit(dev);
	if (unit >= wdcd.cd_ndevs || (du = wdcd.cd_devs[unit]) == NULL)
		return (ENXIO);
	s = splbio();
	while (du->wd_state != OPEN && du->wd_state != OPENRAW &&
	    du->wd_state != CLOSED)
		if (error = tsleep((caddr_t)du, PRIBIO | PCATCH, devopn, 0))
			break;
	splx(s);
	if (error)
		return (error);
	if (du->wd_openmask)
		goto checkpart;	/* already is open, don't mess with it */
	if (flags & O_NONBLOCK) {
		du->wd_state = WANTOPENRAW;
		use_fdisk = 0;
	} else {
		du->wd_state = WANTOPEN;
		use_fdisk = 1;
	}

	/* begin d_init1 */
	if (!timer_started) {
		timer_started++;
		timeout(wdtimeout, (caddr_t) 0, hz);
	}
	/*
	 * If we don't yet have a disklabel, use the geometry passed from
	 * the bootstrap for the boot disk if we have it (ifdef BSDGEOM),
	 * otherwise use the geometry from the BIOS/CMOS if available,
	 * otherwise use the default sizes until we've read the label,
	 * or longer if there isn't one there.
	 */
	if (du->wd_dd.d_ncylinders == 0) {
		labelsect = LABELSECTOR;
		du->wd_dd.d_flags = D_SOFT;
		du->wd_dd.d_type = DTYPE_ST506;
		du->wd_dd.d_secsize = 512;
#ifdef BSDGEOM
		if (wdcont(du)->wdc_dev.dv_unit == 0 &&
		    bsdgeom.unit == du->wd_unit && bsdgeom.ncylinders) {
			du->wd_dd.d_ncylinders = bsdgeom.ncylinders;
			du->wd_dd.d_ntracks = bsdgeom.ntracks;
			du->wd_dd.d_nsectors = bsdgeom.nsectors;
			labelsect += bsdgeom.bsd_startsec;
			use_fdisk = 0;
		} else
#endif
		if (wdcont(du)->wdc_dev.dv_unit == 0 &&
		    (bgp = &biosgeom[du->wd_unit])->ncylinders) {
			du->wd_dd.d_nsectors = bgp->nsectors;
			du->wd_dd.d_ntracks = bgp->ntracks;
			du->wd_dd.d_ncylinders = bgp->ncylinders;
		} else {
			du->wd_dd.d_flags |= D_DEFAULT;
			du->wd_dd.d_nsectors = DFL_SEC;
			du->wd_dd.d_ntracks = DFL_TRK;
			du->wd_dd.d_ncylinders = DFL_CYL;
		}
		du->wd_dd.d_npartitions = 8;
		du->wd_dd.d_secpercyl =
		    du->wd_dd.d_ntracks * du->wd_dd.d_nsectors;
		du->wd_dd.d_partitions[RAWPART].p_offset = 0;
		du->wd_dd.d_partitions[RAWPART].p_size =
		    du->wd_dd.d_ncylinders * du->wd_dd.d_secpercyl;
		du->wd_dd.d_partitions[RAWPART].p_fstype = FS_UNUSED;
		du->wd_dd.d_partitions[0] = du->wd_dd.d_partitions[RAWPART];
		du->wd_labelsector = labelsect;
	} else {
		labelsect = du->wd_labelsector;
		use_fdisk = 0;
	}

	/* no bad block forwarding unless/until we read bad-sector table */
	du->wd_bad[0] = EOBADLIST;
	du->wd_cylpergrp = INT_MAX;
	du->wd_badindx[0] = -1;
	du->wd_dk.dk_stats.dk_bpms = 32 * (60 * DEV_BSIZE / 2);	/* XXX */
	du->wd_dk.dk_stats.dk_secsize = DEV_BSIZE;
	/* end d_init1 */

	/*
	 * Check for a DOS partition table.  If we have one,
	 * use its idea of the BSD partition for the label location.
	 * Note that we are running with the default geometry,
	 * and thus the labelsector value we compute is likely to be wrong,
	 * but the read will hopefully work using the same (wrong) geometry.
	 * We recompute the label location once we have the real geometry.
	 */
	rawdev = makedev(major(dev), wdminor(unit, RAWPART));
	if (use_fdisk && (error = getbsdpartition(rawdev,
	    wdstrategy, &du->wd_dd, &dospart)) == 0) {
		labelsect += mbpssec(&dospart) +
		    mbpstrk(&dospart) * du->wd_dd.d_nsectors +
		    mbpscyl(&dospart) * du->wd_dd.d_secpercyl;
		dprintf(("label at %d/%d/%d => %d\n", mbpscyl(&dospart),
		    mbpstrk(&dospart), mbpssec(&dospart), labelsect));
	} else
		use_fdisk = 0;

	/*
	 * Recal will be done in wdcontrol during first read operation,
	 * either from getbsdpartition call above or readdisklabel here.
	 * If the state is WANTOPENRAW, the read operation will "fail"
	 * after doing a recal, and we don't actually read a label.
	 */
	if (msg = readdisklabel(rawdev, wdstrategy, &du->wd_dd, labelsect)) {
		if (du->wd_state != OPENRAW) {
			log(LOG_ERR, "wd%d: %s\n", unit, msg);
			if (cold) {
				du->wd_state = CLOSED;
				return (ENXIO);
			} else
				du->wd_state = OPENRAW;
		}
		goto checkpart;
	}
	/*
	 * turn label sect/trk/cyl into block number
	 */
	if (use_fdisk) {
		du->wd_labelsector = LABELSECTOR + mbpssec(&dospart) +
		    mbpstrk(&dospart) * du->wd_dd.d_nsectors +
		    mbpscyl(&dospart) * du->wd_dd.d_secpercyl;
		dprintf(("label really %d\n", du->wd_labelsector));
	} else
		du->wd_labelsector = labelsect;
	du->wd_dk.dk_label.d_bsd_startsec = labelsect - LABELSECTOR;
	du->wd_dk.dk_stats.dk_bpms = (du->wd_dk.dk_label.d_rpm / 60) *
	    du->wd_dk.dk_label.d_nsectors * du->wd_dk.dk_label.d_secsize;

	/* begin d_init2 */
	wdsetctlr(du, 1);

	/*
	 * Read bad sector table into memory.
	 */
	du->wd_state = RDBADTBL;
	i = 0;
	bp = geteblk(du->wd_dd.d_secsize);
	do {
		bp->b_flags = B_BUSY | B_READ;
		bp->b_dev = rawdev;
		bp->b_blkno = du->wd_dd.d_secperunit - du->wd_dd.d_nsectors + i;
		if (du->wd_dd.d_secsize > DEV_BSIZE)
			bp->b_blkno *= du->wd_dd.d_secsize / DEV_BSIZE;
		else
			bp->b_blkno /= DEV_BSIZE / du->wd_dd.d_secsize;
		bp->b_bcount = du->wd_dd.d_secsize;
		bp->b_cylin = du->wd_dd.d_ncylinders - 1;
		s = splbio();
		wdstrategy(bp);
		biowait(bp);
		splx(s);
	} while ((bp->b_flags & B_ERROR) && (i += 2) < 10 &&
		i < du->wd_dd.d_nsectors);

	db = (struct dkbad *)(bp->b_un.b_addr);
#define DKBAD_MAGIC 0x4321
	if ((bp->b_flags & B_ERROR) == 0 && db->bt_mbz == 0 &&
	    db->bt_flag == DKBAD_MAGIC)
		wd_setbad(du, db);
	else
		printf("wd%d: %s bad-sector file\n", unit,
		    (bp->b_flags & B_ERROR) ? "can't read" : "format error in");
	bp->b_flags = B_INVAL | B_AGE;
	brelse(bp);
	/* end d_init2 */

	du->wd_state = OPEN;
	wakeup((caddr_t) du);

checkpart:
	return (dkopenpart(&du->wd_dk, dev, fmt));
}

/* ARGSUSED */
wdclose(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	struct wd_softc *du = wdcd.cd_devs[wdunit(dev)];

	return (dkclose(&du->wd_dk, dev, flags, fmt, p));
}

/*
 * Set up bad sector info for disk from bad-sector table.
 * We initialize an index table to find the starting position
 * in the bad sector list for each group of cylinders.
 * The lookup uses pre-increment, so we decrement each index.
 */
wd_setbad(du, db)
	struct wd_softc *du;
	struct dkbad *db;
{
	daddr_t *xp = du->wd_bad;
	struct bt_bad *bb;
	char *indxp = du->wd_badindx;
	int cyl, i;

	du->wd_cylpergrp = (du->wd_dd.d_ncylinders + NCYLGRP - 1) / NCYLGRP;

	/* first group starts at 0 */
	*indxp++ = (0 - 1);
	cyl = du->wd_cylpergrp;

	for (i = 0, bb = db->bt_bad; i < NDKBAD; i++, bb++) {
		if (bb->bt_cyl == 0xffff)
			break;
		*xp++ = bb->bt_cyl * (long)(du->wd_dd.d_secpercyl) +
			(bb->bt_trksec >> 8) * du->wd_dd.d_nsectors +
			(bb->bt_trksec & 0xff);
		dprintf(("BAD: %d; ", xp[-1]));
		while (bb->bt_cyl >= cyl) {
			*indxp++ = i - 1;
			cyl += du->wd_cylpergrp;
			dprintf(("INDX: %d\n", i - 1));
		}
	}
	*xp = EOBADLIST;	/* End of a list */
	i--;
	while (indxp < &du->wd_badindx[NCYLGRP]) {
		*indxp++ = i;
		dprintf(("INDX: %d; ", i - 1));
	}
}

/*
 * Read/write routine for a buffer.  Finds the proper unit, range checks
 * arguments, and schedules the transfer.  Does not wait for the transfer
 * to complete.  Multi-page transfers are supported.  All I/O requests must
 * be a multiple of a sector in length.
 */
wdstrategy(bp)
	register struct buf *bp;
{
	register struct buf *dp;
	register struct wd_softc *du;
	register struct partition *p;
	long maxsz, sz;
	int unit = wdunit(bp->b_dev);
	int s;

#ifdef DIAGNOSTIC
	if (unit >= wdcd.cd_ndevs || wdcd.cd_devs[unit] == NULL ||
	    bp->b_blkno < 0) {
		printf("wd err: unit = %d, blkno = %d, bcount = %d\n",
			unit, bp->b_blkno, bp->b_bcount);
		bp->b_error = EINVAL;
		goto bad;
	}
#endif

	du = wdcd.cd_devs[unit];
	if (DISKSTATE(du->wd_state) < OPEN)
		goto raw;

	/*
	 * Determine the size of the transfer, and make sure it is
	 * within the boundaries of the partition.
	 */
	p = &du->wd_dd.d_partitions[wdpart(bp->b_dev)];
	maxsz = p->p_size;
	sz = (bp->b_bcount + DEV_BSIZE - 1) >> DEV_BSHIFT;
	if (bp->b_blkno + p->p_offset <= du->wd_labelsector &&
	    bp->b_blkno + p->p_offset + sz > du->wd_labelsector &&
	    (bp->b_flags & B_READ) == 0 && du->wd_wlabel == 0) {
		bp->b_error = EROFS;
		goto bad;
	}
	if (bp->b_blkno < 0 || bp->b_blkno + sz > maxsz) {
		/* if exactly at end of disk, return an EOF on read */
		if (bp->b_blkno == maxsz && bp->b_flags & B_READ) {
			bp->b_resid = bp->b_bcount;
			biodone(bp);
			return (0);
		}
		/* or truncate if part of it fits */
		sz = maxsz - bp->b_blkno;
		if (sz <= 0) {
			bp->b_error = EINVAL;
			goto bad;
		}
		bp->b_bcount = sz << DEV_BSHIFT;
	}
	bp->b_cylin = (bp->b_blkno + p->p_offset) / du->wd_dd.d_secpercyl;

raw:
	dp = &du->wd_tab;
	s = splhigh();
	disksort(dp, bp);
	if (dp->b_active == 0)
		wdustart(du);		/* start drive if idle */
	if (wdcont(du)->wdc_tab.b_active == 0)
		wdstart(wdcont(du));	/* start IO if controller idle */
	splx(s);
	return (0);

bad:
	bp->b_flags |= B_ERROR;
	biodone(bp);
	return (EINVAL);
}

/* 
 * Routine to queue a read or write command to the controller.  The request is
 * linked into the active list for the controller.  If the controller is idle,
 * the transfer is started.
 */
void
wdustart(du)
	register struct wd_softc *du;
{
	int unit = du->wd_dk.dk_dev.dv_unit;
	register struct buf *cdp = &wdcont(du)->wdc_tab;
	register struct buf *bp, *dp;

	dp = &du->wd_tab;
	if (dp->b_active)
		return;
	bp = dp->b_actf;
	if (bp == NULL)
		return;	
	dp->b_forw = NULL;
	if (cdp->b_actf == NULL)
		cdp->b_actf = dp;
	else
		cdp->b_actl->b_forw = dp;
	cdp->b_actl = dp;
	dp->b_active = 1;		/* mark the drive as busy */
}

/*
 * Controller startup routine.  This does the calculation, and starts
 * a single-sector read or write operation.  Called to start a transfer,
 * or from the interrupt routine to continue a multi-sector transfer.
 */
void
wdstart(wdcp)
	register struct wdc_softc *wdcp;
{
	register struct wd_softc *du;	/* disk unit for IO */
	register struct buf *bp;
	struct buf *dp, *cdp;
	daddr_t blknum;
	int i, unit, wdc;

	cdp = &wdcp->wdc_tab;
loop:
	dp = cdp->b_actf;
	if (dp == NULL)
		return;
	bp = dp->b_actf;
	if (bp == NULL) {
		cdp->b_actf = dp->b_forw;
		goto loop;
	}
	unit = wdunit(bp->b_dev);
	du = wdcd.cd_devs[unit];
	wdc = wdcp->wdc_iobase;
	if (DISKSTATE(du->wd_state) <= RECAL) {
		if (wdcontrol(bp)) {
			dp->b_actf = bp->av_forw;
			goto loop;	/* done */
		}
		return;
	}
	if (bp->b_bcount == 0) {
		biodone(bp);
		return;
	}
	if ((du->wd_flags & DK_OPSTARTED) == 0) {
		du->wd_flags |= DK_OPSTARTED;
		du->wd_bc = bp->b_bcount;
		du->wd_addr = bp->b_un.b_addr;

		/*
		 * Convert DEV_BSIZE "blocks" to sectors, and
		 * calculate the physical block number and
		 * number of blocks to r/w in one operation.
		 */
		du->wd_blknum = bp->b_blkno * DEV_BSIZE / du->wd_dd.d_secsize;
		if (DISKSTATE(du->wd_state) == OPEN)
			du->wd_blknum +=
			    du->wd_dd.d_partitions[wdpart(bp->b_dev)].p_offset;
		du->wd_dk.dk_stats.dk_xfers++;
		du->wd_dk.dk_stats.dk_sectors +=
		    bp->b_bcount / du->wd_dd.d_secsize;
	}
	du->wd_dk.dk_stats.dk_busy = 1;

	/*
	 * Calculate number of sectors to read/write
	 */
	blknum = du->wd_blknum;
	if (du->wd_flags & DK_SEBYSE)
		du->wd_nconts = 0;
	else
		du->wd_nconts = ((du->wd_bc + du->wd_dd.d_secsize - 1) /
		    du->wd_dd.d_secsize) - 1;
	dprintf(("\nwdstart %d: %s %d bytes blk %d; nconts %d\n", unit,
		(bp->b_flags & B_READ) ? "read" : "write",
		du->wd_bc, blknum, du->wd_nconts));

	/* 
	 * After an error, or always if forced, see if any part
	 * of the current transfer is in the bad block list.
	 */
	if (du->wd_flags & (DK_SEBYSE|DK_BADSRCH)) {
	    	register daddr_t *xp;
	    	daddr_t eblk = blknum + du->wd_nconts;

	    	xp = &du->wd_bad[du->wd_badindx[bp->b_cylin / du->wd_cylpergrp]];
		dprintf(("bad lookup %d-%d\n", blknum, eblk));
		while (*++xp < blknum && *++xp < blknum &&
		       *++xp < blknum && *++xp < blknum)
			 ;
		if (*xp <= eblk) {	
			/*
			 * If we find one of the blocks, see whether it's
			 * the first block of the transfer (revector now),
			 * or a subsequent block.  In the latter case,
			 * shorten the transfer to end just before the
			 * revectored sector.
			 */
			if (*xp == blknum) {
				dprintf(("wd%d: blk %d replaced with ",
				    unit, blknum));
				blknum = du->wd_dd.d_secperunit -
					 du->wd_dd.d_nsectors - 1 -
					 (xp - du->wd_bad);
				du->wd_nconts = 0;
				dprintf(("%d\n", blknum));
			} else
				du->wd_nconts = *xp - blknum - 1;
		}
	}

	/*
	 * Start the IO.
	 * The wait for BUSY to clear should not be necessary
	 * (and it hangs on non-existent IDE drives).
	 */
	for (i = 1000; (inb(wdc+wd_status) & WDCS_BUSY) && i-- >= 0; )
		;
	/* mark controller active, sec. to timeout */
	cdp->b_active = wd_timeout;

	/*
	 * Pin 2 on the daisy chain cable has a different meaning depending
	 * on the type of drive attached, and here we have to tell the
	 * disk controller which signal to send.  If the drive has more
	 * than 8 heads, then the signal is the most significant bit
	 * of the head number.  But, if the drive has fewer than 8 heads,
	 * it may (or may not) use this wire to signal "Reduced Write 
	 * Current".  This signal is asserted by the disk controller
	 * chip whenever the cylinder number of the request is greater
	 * than or equal to the "precomp cylinder".  All drives with
	 * >= 8 heads, and some other drives keep up with the cylinder
	 * number internally, so they don't need this signal from the
	 * controller.
	 *
	 * Virtually all of the pre-production testing of BSDI was done 
	 * with this signal permanantly set to Head Select, so any drives 
	 * that actually interpret this signal for Write Current control 
	 * were effectivly told never to use Reduced Write Current.  Also, 
	 * in all existing labels, d_precompcyl has the value 0 (it is 
	 * referred to in disktab(5) as "d0").
	 *
	 * So, in the interest of not rocking the boat, we only enable
	 * the Reduced Write Current function if someone has taken the
	 * extra step of setting a reasonable precomp cylinder in the 
	 * disk label.  Otherwise, we keep the old behavior.
	 */
	if (du->wd_dd.d_ntracks < 8 && du->wd_dd.d_precompcyl > 0 &&
	    du->wd_dd.d_precompcyl < 1024) {
		outb(wdc+wd_precomp, du->wd_dd.d_precompcyl / 4);
		outb(wdc+wd_ctlr, 0);	/* enable Reduced Write Current */
	} else {
		outb(wdc+wd_precomp, 0xff);
		outb(wdc+wd_ctlr, WDCTL_HEAD3ENB); /* enable head bit 3 */
	}

#ifdef notdef
	if (bp->b_flags & B_FORMAT) {
		wr(wdc+wd_sector, du->wd_dd.d_gap3);
		wr(wdc+wd_seccnt, du->wd_dd.d_nsectors);
	} else {
#endif
		outb(wdc+wd_seccnt, du->wd_nconts + 1);
		outb(wdc+wd_sector, 1 + (blknum % du->wd_dd.d_nsectors));
#ifdef notdef
	}
#endif
	i = blknum / du->wd_dd.d_secpercyl;
	outb(wdc+wd_cyl_lo, i);
	outb(wdc+wd_cyl_hi, i >> 8);

	/* Set up the SDH register (select drive). */
	i = (blknum % du->wd_dd.d_secpercyl) / du->wd_dd.d_nsectors;
	outb(wdc+wd_sdh, WDSD_IBM | (du->wd_unit << 4) | i);
	while ((inb(wdc+wd_status) & WDCS_READY) == 0)
		;
#ifdef notdef
	if (bp->b_flags & B_FORMAT) {
		outb(wdc+wd_command, WDCC_FORMAT);
		return;
	}
#endif
	/* If this is a read operation, just go away until it's done. */
	if (bp->b_flags & B_READ) {
		outb(wdc+wd_command, WDCC_READ);
		return;
	}

	/*
	 * Write command
	 */
	outb(wdc+wd_command, WDCC_WRITE);

	/* Ready to send data?	*/
	while ((inb(wdc+wd_status) & WDCS_DRQ) == 0)
		;

	outsw(wdc+wd_data, du->wd_addr, du->wd_dd.d_secsize / 2);
}

/*
 * Perform a hard reset during error recovery.
 * Sometimes a controller/drive combination wedges so hard
 * that nothing else will unwedge it (and maybe not even this).
 * All drives on this controller will then need to be reinitialized,
 * doing a recal and setting geometry.
 */
static void
wdc_reinit(wdcp, bp)
	struct wdc_softc *wdcp;
	struct buf *bp;
{
	int wdc = wdcp->wdc_iobase;
	struct wd_softc *du;
	void **dvp;

	wdcreset(wdc);

	for (dvp = wdcd.cd_devs; dvp < &wdcd.cd_devs[wdcd.cd_ndevs]; ++dvp)
	    if ((du = (struct wd_softc *)*dvp) && wdcont(du) == wdcp) {
		/*
		 * Change disk state to cause recal depending on previous
		 * state.  If already re-initializing (!) or closed,
		 * nothing to do.  If open, REINIT | WANTOPEN | (wasraw)
		 * will do recal then go to previous open state.
		 * Other starts are partly initialized; start them over.
		 */ 
		if (du->wd_state & REINIT)
			continue;
		switch (DISKSTATE(du->wd_state)) {
		case CLOSED:
			break;
		case OPEN:
			du->wd_state = REINIT|WANTOPEN|(du->wd_state&RAWDISK);
			break;
		default:
			du->wd_state = WANTOPEN|(du->wd_state&RAWDISK);
			break;

		}
	    }

	(void) wdcontrol(bp);		/* start recal on current drive */
}

/*
 * Interrupt routine for the controller.  Acknowledge the interrupt, check for
 * errors on the current operation, mark it done if necessary, and start
 * the next request.  Also check for a partially done transfer, and
 * continue with the next chunk if so.
 */
wdintr(wdcp)
	struct wdc_softc *wdcp;
{
	int ctlr = wdcp->wdc_dev.dv_unit;
	struct buf *cdp = &wdcp->wdc_tab;
	register struct buf *dp = cdp->b_actf;
	register struct buf *bp;
	int unit;
	register struct	wd_softc *du;
	int wdc = wdcp->wdc_iobase;
	int status, done, i;

	/* Shouldn't need this, but it may be a slow controller. */
	i = 100;
	while ((status = inb(wdc+wd_status)) & WDCS_BUSY) {
		if (i-- <= 0) {
			printf("wdc%d: controller wedged\n", ctlr);
			wdcp->wdc_flags |= WDC_TIMEDOUT;
			break;
		}
		DELAY(1000);
	}
	if (!cdp->b_active) {
		if (wdcp->wdc_flags & WDC_SETCTLR)
			wdcp->wdc_flags &= ~WDC_SETCTLR;
		else if (status == (WDCS_READY|WDCS_SEEKCMPLT))
			;	/* happens on notebooks powering down disk */
		else
			printf("wdc%d: unexpected interrupt (status %b)\n",
			    ctlr, status, WDCS_BITS);
		return (-1);
	}
	bp = dp->b_actf;
	unit = wdunit(bp->b_dev);
	du = wdcd.cd_devs[unit];
	du->wd_dk.dk_stats.dk_busy = 0;

	dprintf(("I "));
	if (DISKSTATE(du->wd_state) <= RECAL) {
		if (wdcontrol(bp))
			goto done;
		return (1);
	}
	/*
	 * Check for errors or timeouts.
	 */
	if (wdcp->wdc_flags & WDC_TIMEDOUT &&
	    (status & (WDCS_ERR | WDCS_ECCCOR | WDCS_DRQ)) == WDCS_DRQ) {
		log(LOG_ERR, "wd%d: timeout ignored, processing interrupt\n",
		    unit);
		wdcp->wdc_flags &= ~WDC_TIMEDOUT;
	}

	if (status & (WDCS_ERR | WDCS_ECCCOR) ||
	    wdcp->wdc_flags & WDC_TIMEDOUT) {
		du->wd_statusreg = status;
		du->wd_errorreg = inb(wdc+wd_error);	/* save error status */
		dprintf(("status %x error %x\n", status, du->wd_errorreg));
#ifdef notdef
		if (bp->b_flags & B_FORMAT) {
			bp->b_flags |= B_ERROR;
			goto done;
		}
#endif

		if (status & WDCS_ERR || wdcp->wdc_flags & WDC_TIMEDOUT) {
			wdcp->wdc_flags &= ~WDC_TIMEDOUT;
			if (++cdp->b_errcnt <= RETRIES) {
				/*
				 * Retry i/o sector by sector
				 */
				cdp->b_active = 0;
				du->wd_flags |= DK_SEBYSE;
				if (du->wd_errorreg & WDERR_ABORT &&
				    cdp->b_errcnt == 2) {
					/*
					 * Some controller/disk combinations
					 * have a pathological state
					 * characterized by an ABORT error;
					 * only a hard reset seems to make
					 * things better.
					 */
					printf("wd%d: multiple abort errors%s\n",
					    unit, ", resetting controller...");
					wdc_reinit(wdcp, bp);
					return (1);
				}
				wdstart(wdcp);
				return (1);
			}
			diskerr(bp, "wd", "hard error", LOG_PRINTF,
			    (bp->b_bcount - du->wd_bc) / du->wd_dd.d_secsize,
			    &du->wd_dd);
			printf(" status %b error %b\n",
			    du->wd_statusreg, WDCS_BITS,
			    du->wd_errorreg, WDERR_BITS);
			bp->b_flags |= B_ERROR;	/* flag the error */
			goto done;
		} else {
			log(LOG_WARNING, "wd%d%c: soft ecc bn %d\n",
			    unit, wdpart(bp->b_dev) + 'a', du->wd_blknum);
			du->wd_nconts = 0;	/* start new transfer */
		}
	}

	/*
	 * If this was a successful read operation, fetch the data.
	 */
	done = min(du->wd_dd.d_secsize, du->wd_bc);
	if (bp->b_flags & B_READ) {
		int dummy;

		/* Ready to receive data? */
		while ((inb(wdc+wd_status) & WDCS_DRQ) == 0)
			;

		/*dprintf(DDSK, "addr %x\n", du->wd_addr);*/
		i = done / 2;
		insw(wdc+wd_data, (int) du->wd_addr, i);
		while (i++ < du->wd_dd.d_secsize / 2)
			insw(wdc+wd_data, &dummy, 1);
	}

	/* If we got an error earlier, report it */
	if (cdp->b_errcnt) {
		diskerr(bp, "wd", "soft error", LOG_WARNING,
		    (bp->b_bcount - du->wd_bc) / du->wd_dd.d_secsize,
		    &du->wd_dd);
		addlog(" status %b error %b retries %d\n",
			du->wd_statusreg, WDCS_BITS,
			du->wd_errorreg, WDERR_BITS, cdp->b_errcnt);
		cdp->b_errcnt = 0;
	}

	du->wd_bc -= done;
	du->wd_addr += done;
	du->wd_blknum++;

	/*
	 * If this transfer isn't finished,
	 * proceed with the next contiguous i/o sector.
	 */
	if (du->wd_nconts > 0) {
		du->wd_nconts--;

		/* Write the next piece of data on writing */
		if (!(bp->b_flags & B_READ)) {

			/* Ready to accept data? */
			while ((inb(wdc+wd_status) & WDCS_DRQ) == 0)
				;
			outsw(wdc+wd_data, du->wd_addr,
			    du->wd_dd.d_secsize / 2);
		}
		return (1);
	}

	/*
	 * Do we need more i/o to finish this request?
	 */
	if (du->wd_bc > 0) {
		wdstart(wdcp);
		return (1);
	}

done:
	/*
	 * Done with this transfer, with or without error
	 */
	cdp->b_actf = dp->b_forw;
	cdp->b_errcnt = 0;
	du->wd_flags &= ~(DK_OPSTARTED|DK_SEBYSE);
	dp->b_active = 0;
	dp->b_actf = bp->av_forw;
	dp->b_errcnt = 0;
	bp->b_resid = du->wd_bc;	/* 0 except on error... */
	biodone(bp);

	/*
	 * Start the next request.
	 */
	cdp->b_active = 0;
	if (dp->b_actf)
		wdustart(du);		/* requeue disk if more io to do */
	
	if (wdcp->wdc_flags & WDC_WAIT) {
		wdcp->wdc_flags &= ~WDC_WAIT;
		wakeup((caddr_t) wdcp);
	} else if (cdp->b_actf)
		wdstart(wdcp);		/* start IO on next drive */
	return (1);
}

/*
 * Driver timeout
 */
static void
wdtimeout()
{
	register i, s;
	register struct wdc_softc *wdcp;
	register struct buf *cdp;

	s = splbio();
	for (i = 0; i < wdccd.cd_ndevs; cdp++, i++) {
		if ((wdcp = wdccd.cd_devs[i]) == NULL)
			continue;
		cdp = &wdcp->wdc_tab;
		if (cdp->b_active && --cdp->b_active == 0) {
			printf("wdc%d: lost interrupt\n", i);
			cdp->b_active++;
			wdcp->wdc_flags |= WDC_TIMEDOUT;
			wdintr(wdcp);
		}
	}
	splx(s);
	timeout(wdtimeout, (caddr_t) NULL, hz);
}

/*
 * Implement initialization operations.
 * Called from wdstart or wdintr during opens.
 * Uses finite-state-machine to track progress of operation in progress.
 * Returns 0 if operation still in progress, 1 if completed.
 */
wdcontrol(bp)
	register struct buf *bp;
{
	register struct wd_softc *du;
	register struct wdc_softc *wdcp;
	struct buf *cdp;
	int unit, punit;
	unsigned char stat;
	int s, wdc;

	unit = wdunit(bp->b_dev);
	du = (struct wd_softc *) wdcd.cd_devs[unit];
	wdcp = wdcont(du);
	cdp = &wdcp->wdc_tab;
	wdc = wdcp->wdc_iobase;
	punit = du->wd_unit;
	switch (DISKSTATE(du->wd_state)) {

	tryagainrecal:
	case WANTOPEN:			/* set SDH, step rate, do restore */
		dprintf(("wd%d: recal ", unit));
		s = splbio();		/* not called from intr level ... */
		outb(wdc+wd_sdh, WDSD_IBM | (punit << 4));
		cdp->b_active = wd_timeout;
		outb(wdc+wd_command, WDCC_RESTORE | WD_STEP);
		du->wd_state++;
		splx(s);
		return (0);

	case RECAL:
		if (((stat = inb(wdc+wd_status)) & WDCS_ERR) ||
		    wdcp->wdc_flags & WDC_TIMEDOUT) {
			printf("wd%d: recal %s", unit,
			    wdcp->wdc_flags & WDC_TIMEDOUT ?
			    "timed out" : "failed");
			if (du->wd_state & REINIT)
				printf(" after controller reset");
			printf(": status %b error %b\n",
			    stat, WDCS_BITS, inb(wdc+wd_error), WDERR_BITS);
			if (++cdp->b_errcnt < RETRIES)
				goto tryagainrecal;

			bp->b_flags |= B_ERROR;		/* didn't read label */
			du->wd_state = OPENRAW;
			return (1);
		}

		wdsetctlr(du, 0);

		/*
		 * if reinitializing after a controller reset,
		 * we are now ready to continue the current operation.
		 */
		if (du->wd_state & REINIT) {
			du->wd_state = (du->wd_state&RAWDISK) ? OPENRAW : OPEN;
			wdstart(wdcp);
			return (0);
		}

		if (ISRAWSTATE(du->wd_state)) {
			bp->b_flags |= B_ERROR;		/* didn't read label */
			du->wd_state = OPENRAW;
			return (1);
		}
		dprintf(("rdlabel "));
		du->wd_state = RDLABEL;
		wdstart(wdcp);
		return (0);

	default:
		panic("wdcontrol");
	}
	/* NOTREACHED */
}

/*
 * Set controller parameters from disk label.
 * If needlock is 1, wait for controller to be idle.
 */
void
wdsetctlr(du, needlock)
	struct wd_softc *du;
{
	struct wdc_softc *wdcp = wdcont(du);
	int wdc = wdcp->wdc_iobase;

	if (needlock)
		wdlock(wdcp);
	wdcp->wdc_flags |= WDC_SETCTLR;	/* XXX */
	outb(wdc+wd_cyl_lo, du->wd_dd.d_ncylinders);
	outb(wdc+wd_cyl_hi, du->wd_dd.d_ncylinders >> 8);
	outb(wdc+wd_sdh,
	    WDSD_IBM | (du->wd_unit << 4) | du->wd_dd.d_ntracks - 1);
	outb(wdc+wd_seccnt, du->wd_dd.d_nsectors);
	outb(wdc+wd_command, WDCC_SETGEOM);

	while (inb(wdc+wd_status) & WDCS_BUSY)
		;
	if (needlock)
		wdunlock(wdcp);
}

/*
 * Wait for controller to finish current operation
 * so that direct controller accesses can be done.
 */
wdlock(wdc)
	struct wdc_softc *wdc;
{
	int s;

	s = splbio();
	while (wdc->wdc_tab.b_active || wdc->wdc_flags & WDC_LOCKED) {
		wdc->wdc_flags |= WDC_WAIT;
		sleep((caddr_t) wdc, PRIBIO);
	}
	wdc->wdc_flags |= WDC_LOCKED;
	splx(s);
}

/*
 * Continue normal operations after pausing for 
 * munging the controller directly.
 */
wdunlock(wdc)
	struct wdc_softc *wdc;
{

	wdc->wdc_flags &= ~WDC_LOCKED;
	if (wdc->wdc_flags & WDC_WAIT) {
		wdc->wdc_flags &= ~WDC_WAIT;
		wakeup((caddr_t) wdc);
	} else if (wdc->wdc_tab.b_actf)
		wdstart(wdc);
}

wdioctl(dev, cmd, addr, flag)
	dev_t dev;
	caddr_t addr;
	int cmd, flag;
{
	int unit = wdunit(dev);
	register struct wd_softc *du;
	int error = 0;
	/*int wdformat();*/

	du = wdcd.cd_devs[unit];

	switch (cmd) {

	case DIOCGDINFO:
		*(struct disklabel *)addr = du->wd_dd;
		break;

	case DIOCGHWINFO:
		error = wdgetgeom(du, (struct disklabel *)addr);
		break;

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = &du->wd_dd;
		((struct partinfo *)addr)->part =
		    &du->wd_dd.d_partitions[wdpart(dev)];
		break;

	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else
			error = setdisklabel(&du->wd_dd,
			    (struct disklabel *) addr,
			    (du->wd_state == OPENRAW) ?  0 : du->wd_openmask);
		if (error == 0) {
			if (du->wd_state == OPENRAW)
				du->wd_state = OPEN;
			du->wd_labelsector = du->wd_dd.d_bsd_startsec +
			    LABELSECTOR;
		}
		wdsetctlr(du, 1);
		break;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else
			du->wd_wlabel = *(int *)addr;
		break;

	case DIOCWDINFO:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else if ((error = setdisklabel(&du->wd_dd,
		    (struct disklabel *)addr,
		    (du->wd_state == OPENRAW) ? 0 : (du->wd_bopenmask |
		    (du->wd_copenmask &~ (1 << RAWPART))))) == 0) {
			int wlab;

			if (error == 0) {
				if (du->wd_state == OPENRAW)
					du->wd_state = OPEN;
				du->wd_labelsector = du->wd_dd.d_bsd_startsec +
				    LABELSECTOR;
			}
			wdsetctlr(du, 1);

			/* simulate opening partition 0 so write succeeds */
			du->wd_openmask |= (1 << 0);	    /* XXX */
			wlab = du->wd_wlabel;
			du->wd_wlabel = 1;
			error = writedisklabel(dev, wdstrategy, &du->wd_dd,
			    du->wd_labelsector);
			du->wd_openmask = du->wd_copenmask | du->wd_bopenmask;
			du->wd_wlabel = wlab;
		}
		break;

	case DIOCSBAD:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else
			wd_setbad(du, (struct dkbad *) addr);
		break;

#ifdef notyet
	case DIOCGDINFOP:
		*(struct disklabel **)addr = &(du->wd_dd);
		break;

	case DIOCWFORMAT:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else {
			register struct format_op *fop;
			struct uio auio;
			struct iovec aiov;

			fop = (struct format_op *)addr;
			aiov.iov_base = fop->df_buf;
			aiov.iov_len = fop->df_count;
			auio.uio_iov = &aiov;
			auio.uio_iovcnt = 1;
			auio.uio_resid = fop->df_count;
			auio.uio_segflg = 0;
			auio.uio_offset =
				fop->df_startblk * du->wd_dd.d_secsize;
			error = physio(wdformat, &rwdbuf[unit], dev, B_WRITE,
				minphys, &auio);
			fop->df_count -= auio.uio_resid;
			fop->df_reg[0] = du->wd_statusreg;
			fop->df_reg[1] = du->wd_errorreg;
		}
		break;
#endif

	default:
		error = ENOTTY;
		break;
	}
	return (error);
}

#ifdef notdef
wdformat(bp)
	struct buf *bp;
{

	bp->b_flags |= B_FORMAT;
	return (wdstrategy(bp));
}
#endif

/*
 * The following section is derived from the 386BSD wd driver.
 */

/*
 * issue READP to drive to ask it what it is.
 * As this command may not be implemented by all controllers,
 * we use it only on demand in setting up a disk.
 */
int
wdgetgeom(du, lp)
	struct wd_softc *du;
	struct disklabel *lp;
{
	struct wdc_softc *wdcp = wdcont(du);
	int stat, x, i;
	int error = 0, wdc = wdcp->wdc_iobase;
	char tb[DEV_BSIZE];
	struct wdparams *wp = (struct wdparams *) tb;
	int timeout = 1000000;

	wdlock(wdcp);
	x = splbio();		/* not called from intr level ... */
	outb(wdc+wd_sdh, WDSD_IBM | (du->wd_unit << 4));

	/* controller ready for command? */
	while (((stat = inb(wdc + wd_status)) & WDCS_BUSY) && timeout > 0)
		timeout--;
	if (timeout <= 0) {
		error = ETIMEDOUT;
		goto out;
	}

	/* send command, await results */
	outb(wdc+wd_command, WDCC_READP);
	while (((stat = inb(wdc+wd_status)) & WDCS_BUSY) && timeout > 0)
		timeout--;
	if (timeout <= 0) {
		error = ETIMEDOUT;
		goto out;
	}

	/* is controller ready to return data? */
	while (((stat = inb(wdc+wd_status)) & (WDCS_ERR|WDCS_DRQ)) == 0 &&
	    timeout > 0)
		timeout--;
	if (timeout <= 0) {
		error = ETIMEDOUT;
		goto out;
	}

	if (stat & WDCS_ERR) {
		error = EIO;
		goto out;
	}

	/* obtain parameters */
	insw(wdc+wd_data, tb, sizeof(tb)/sizeof(short));

	lp->d_ncylinders = wp->wdp_fixedcyl + wp->wdp_removcyl /*+- 1*/;
	lp->d_ntracks = wp->wdp_heads;
	lp->d_nsectors = wp->wdp_sectors;

	/* XXX sometimes possibly needed */
	(void) inb(wdc+wd_status);
out:
	wdunlock(wdcp);
	splx(x);
	return (error);
}

wdsize(dev)
	dev_t dev;
{
	register int unit = wdunit(dev);
	register int part = wdpart(dev);
	register struct wd_softc *du;
	register int val;

	if (unit >= wdcd.cd_ndevs || (du = wdcd.cd_devs[unit]) == NULL)
		return (-1);
	if (du->wd_state == 0) {
		val = wdopen(dev, 0, S_IFBLK, (struct proc *) NULL);
		if (val < 0)
			return (-1);
	}
	if (part >= du->wd_dd.d_npartitions)
		return (-1);
	return ((int)((u_long)du->wd_dd.d_partitions[part].p_size *
		du->wd_dd.d_secsize / DEV_BSIZE));
}

extern	char *vmmap;	    /* poor name! */

wddump(dev)			/* dump core after a system crash */
	dev_t dev;
{
	register struct wd_softc *du;	/* disk unit to do the IO */
	long num, onum;			/* number of sectors to write */
	int wdc;
	int unit, part;
	daddr_t blknum, sblk, eblk, blkcnt;
	daddr_t *xp;
	long cylin, head, sector;
	long secpertrk, secpercyl, nblocks, i;
	char *addr;
	extern int dumpsize;
	static wddoingadump = 0;
	
	addr = (char *) 0;		/* starting address */
	/* size of memory to dump */
	num = dumpsize;
	unit = wdunit(dev);
	part = wdpart(dev);		/* file system */
	/* check for acceptable drive number */
	if (unit >= wdcd.cd_ndevs || (du = wdcd.cd_devs[unit]) == NULL)
		return (ENXIO);

	wdc = wdcont(du)->wdc_iobase;
	/* was it ever initialized ? */
	if (du->wd_state < OPEN)
		return (ENXIO);

	/* Convert to disk sectors */
	num = (u_long) num * NBPG / du->wd_dd.d_secsize;

	if (wddoingadump)
		return (EFAULT);

	secpertrk = du->wd_dd.d_nsectors;
	secpercyl = du->wd_dd.d_secpercyl;
	nblocks = du->wd_dd.d_partitions[part].p_size;

/*pg("part %x, nblocks %d, dumplo %d num %d\n", part, nblocks, dumplo, num); */
	/* check transfer bounds against partition size */
	if (dumplo < 0 || dumplo >= nblocks)
		return (EINVAL);
	if (dumplo + num > nblocks)
		num = nblocks - dumplo;
	blknum = dumplo + du->wd_dd.d_partitions[part].p_offset;

	wddoingadump = 1;
	i = 100000;
	while (inb(wdc+wd_status) & WDCS_BUSY && i-- > 0)
		;
	outb(wdc+wd_sdh, WDSD_IBM | (du->wd_unit << 4));
	outb(wdc+wd_command, WDCC_RESTORE | WD_STEP);
	while (inb(wdc+wd_status) & WDCS_BUSY)
		;

	wdsetctlr(du, 0);
	if (du->wd_dd.d_ntracks < 8 && du->wd_dd.d_precompcyl > 0 &&
	    du->wd_dd.d_precompcyl < 1024) {
		outb(wdc+wd_precomp, du->wd_dd.d_precompcyl / 4);
		outb(wdc+wd_ctlr, 0);	/* enable Reduced Write Current */
	} else {
		outb(wdc+wd_precomp, 0xff);
		outb(wdc+wd_ctlr, WDCTL_HEAD3ENB); /* enable head bit 3 */
	}
	
	onum = num;
	while (num > 0) {
		blkcnt = min(num, CLBYTES / 512);
		pmap_enter(pmap_kernel(), vmmap, addr, VM_PROT_READ, TRUE);

	again:
		sblk = blknum;
	    	eblk = blknum + blkcnt - 1;

	    	xp = &du->wd_bad[du->wd_badindx[(blknum / secpercyl) /
		    du->wd_cylpergrp]];
		dprintf(("bad lookup %d-%d\n", blknum, eblk));
		while (*++xp < blknum)
			 ;
		if (*xp <= eblk) {	
			/*
			 * If we find one of the blocks, see whether it's
			 * the first block of the transfer (revector now),
			 * or a subsequent block.  In the latter case,
			 * shorten the transfer to end just before the
			 * revectored sector.
			 */
			if (*xp == blknum) {
				dprintf(("blk %d replaced with ", blknum));
				sblk = du->wd_dd.d_secperunit -
					 du->wd_dd.d_nsectors - 1 -
					 (xp - du->wd_bad);
				blkcnt = 1;
				dprintf(("%d\n", sblk));
			} else
				blkcnt = *xp - blknum;
		}

		/* compute disk address */
		cylin = sblk / secpercyl;
		head = (sblk % secpercyl) / secpertrk;
		sector = (sblk % secpertrk) + 1;	/* origin 1 */

		/* select drive and head. */
		outb(wdc+wd_sdh, WDSD_IBM | (du->wd_unit<<4) | (head & 0xf));
		while ((inb(wdc+wd_status) & WDCS_READY) == 0)
			;

		/* transfer some blocks */
		outb(wdc+wd_sector, sector);
		outb(wdc+wd_seccnt, blkcnt);
		outb(wdc+wd_cyl_lo, cylin);
		outb(wdc+wd_cyl_hi, cylin >> 8);
#ifdef notdef
		/* lets just talk about this first...*/
		pg("sdh 0%o sector %d cyl %d addr 0x%x",
			inb(wdc+wd_sdh), inb(wdc+wd_sector),
			inb(wdc+wd_cyl_hi)*256 + inb(wdc+wd_cyl_lo), addr);
#endif
		outb(wdc+wd_command, WDCC_WRITE);
		
		while (blkcnt-- > 0) {
			/* Ready to send data?	*/
			while ((inb(wdc+wd_status) & WDCS_DRQ) == 0)
				;
			if (inb(wdc+wd_status) & WDCS_ERR)
				return (EIO);

			outsw(wdc+wd_data, vmmap + ((int)addr & PGOFSET), 256);
			addr += 512;
			num--;
			blknum++;

			if (inb(wdc+wd_status) & WDCS_ERR)
				return (EIO);
		}
		/* Check data request (should be done).	 */
		if (inb(wdc+wd_status) & WDCS_DRQ)
			return (EIO);

		/* wait for completion */
		for (i = 1000000; inb(wdc+wd_status) & WDCS_BUSY; i--)
			if (i < 0)
				return (EIO);
		/* error check the xfer */
		if (inb(wdc+wd_status) & WDCS_ERR)
			return (EIO);
#if 0
		/* skip I/O memory */
		if (addr == (char *)0xa0000) {
			long d = (0x100000 - 0xa0000) / 512;
			addr = (char *)0x100000;
			num -= d;  blknum += d;
		}
#endif

		if (num > 0 && (int)addr & PGOFSET) {
			blkcnt = min(num,
			    (CLBYTES - ((int)addr & PGOFSET)) / 512);
			goto again;
		}
		if (onum - num >= 100) {
			printf(".");
			onum = num;
		}
	}
	return (0);
}
