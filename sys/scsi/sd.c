/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: sd.c,v 1.19 1994/01/12 03:00:53 karels Exp $
 */

/*-
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Lawrence Berkeley Laboratory and its contributors.
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
 * from sd.c,v 1.11 91/12/19 01:10:18 torek Exp  
 */

/*
 * SCSI CCS (Command Command Set) disk driver.
 *
 * MACHINE INDEPENDENT (do not put machine dependent goo in here!)
 *
 * (from sd.c,v 1.7 90/12/15 14:11:26 van Exp)
 */

#ifndef SCSI
you must define SCSI to use this driver
#endif

#include "sys/param.h"
#include "sys/systm.h"
#include "sys/proc.h"
#include "sys/buf.h"
#include "sys/conf.h"
#include "sys/errno.h"
#include "sys/device.h"
#include "sys/disklabel.h"
#include "sys/disk.h"
#include "sys/fcntl.h"
#include "sys/ioctl.h"
#include "sys/malloc.h"

#include "ufs/fs.h"		/* XXX for SBSIZE and BBSIZE */

#include "scsi/scsi.h"
#include "scsi/disk.h"
#include "scsi/disktape.h"
#include "scsi/scsivar.h"
#include "scsi/scsi_ioctl.h"

#include "machine/bootblock.h"
#include "machine/cpu.h"

#ifdef i386
#undef d_type	/* XXX !!! */
#endif

/*
 * Per-disk variables.
 *
 * sc_dk contains all the `disk' specific stuff (label/partitions,
 * transfer rate, etc).  We need only things that are special to
 * scsi disks.  Note that our blocks are in terms of DEV_BSIZE blocks.
 */
struct sd_softc {
	struct	dkdevice sc_dk;	/* base disk device, must be first */
	struct	unit sc_unit;	/* scsi unit */
	u_char	sc_type;	/* drive type */
	u_char	sc_bshift;	/* convert device blocks to DEV_BSIZE blks */
	short	sc_flags;	/* see below */
	u_int	sc_blks;	/* number of blocks on device */
	int	sc_blksize;	/* device block size in bytes */

	/* should be in dkdevice?? */
	struct	buf sc_tab;	/* transfer queue */

	/* statistics */
	long	sc_resets;	/* number of times reset */
	long	sc_transfers;	/* count of total transfers */
	long	sc_partials;	/* count of `partial' transfers */

	/* for user formatting */
	struct	scsi_fmt sc_fmt;
};

#define	SDF_ALIVE	1	/* drive OK for regular kernel use */

void sdattach __P((struct device *parent, struct device *self, void *aux));
static void sdcapacity __P((struct sd_softc *sc, struct scsi_attach_args *sa));
int sdclose __P((dev_t dev, int flags, int ifmt, struct proc *p));
int sddump __P((dev_t dev));
int sddump2 __P((dev_t dev, daddr_t blkoff, caddr_t addr, int len, int isphys));
static int sderror __P((struct sd_softc *sc, int stat));
void sdgo __P((struct device *sc0, struct scsi_cdb *cdb));
void sdigo __P((struct device *sc0, struct scsi_cdb *cdb));
void sdintr __P((struct device *sc0, int stat, int resid));
int sdioctl __P((dev_t dev, int cmd, caddr_t data, int flag, struct proc *p));
static void sdlblkstrat __P((struct buf *bp, int bsize));
int sdmatch __P((struct device *parent, struct cfdata *cf, void *aux));
int sdopen __P((dev_t dev, int flags, int ifmt, struct proc *p));
void sdreset __P((struct unit *u));
int sdsize __P((dev_t dev));
int sdstrategy __P((struct buf *bp));

struct cfdriver sdcd =
    { NULL, "sd", sdmatch, sdattach, sizeof(struct sd_softc) };

static struct unitdriver sdunitdriver = { /*sdgo, sdintr*/ sdreset };

#ifdef notyet
static struct sddkdriver = { sdstrategy };
#endif

#ifdef DEBUG
int sddebug = 1;
#define SDB_ERROR	0x01
#define SDB_PARTIAL	0x02
#endif

#define	sdunit(x)	(minor(x) >> 3)
#define sdpart(x)	(minor(x) & 0x7)
#define	RAWDEV(x)	(((x) & ~0x7) | 2)	/* 'c' partition */

#define	b_cylin		b_resid

#define	SDRETRY		2

int
sdmatch(parent, cf, aux)
	struct device *parent;
	register struct cfdata *cf;
	void *aux;
{
	register struct scsi_attach_args *sa = aux;
#ifdef DEBUG
	char *whynot;
#endif

#ifdef notyet
	/*
	 * unit number must match, or be given as `any'
	 */
	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != sa->sa_unit)
		return (0);
#endif
	/*
	 * drive must be a disk, and of a kind we recognize
	 */
	if ((sa->sa_inq_status & STS_MASK) != STS_GOOD) {
#ifdef DEBUG
		whynot = "INQUIRY failed";
#endif
		goto notdisk;
	}

	switch (sa->sa_si.si_type & TYPE_TYPE_MASK) {

	case TYPE_DAD:		/* disk */
	case TYPE_WORM:		/* WORM */
	case TYPE_ROM:		/* CD-ROM */
	case TYPE_MO:		/* Magneto-optical */
	case TYPE_JUKEBOX:	/* medium changer */
		break;

	default:
	notdisk:
		return (0);
	}

	/*
	 * It is a disk of some kind; take it.  We will figure out
	 * the rest in the attach routine.
	 */
	return (1);
}

static void
sdcapacity(sc, sa)
	struct sd_softc *sc;
	struct scsi_attach_args *sa;
{
	struct scsi_cdb cap = { CMD_READ_CAPACITY };
	u_char capbuf[8];
	int i;

	CDB10(&cap)->cdb_lun_rel = sc->sc_unit.u_unit << 5;
	i = (*sc->sc_unit.u_hbd->hd_icmd)(sc->sc_unit.u_hba,
	    sc->sc_unit.u_targ, &cap, (char *)capbuf, sizeof capbuf, B_READ);
	i &= STS_MASK;
	if (i == STS_GOOD) {
#define	NUMBER(p) (((p)[0] << 24) | ((p)[1] << 16) | ((p)[2] << 8) | (p)[3])
		/* return value is address of last valid block */
		sc->sc_blks = NUMBER(&capbuf[0]) + 1;
		sc->sc_blksize = NUMBER(&capbuf[4]);
		sc->sc_dk.dk_label.d_flags &= ~D_DEFAULT;
		if (sa && sa->sa_si.si_qual & QUAL_RMB) {
			sc->sc_dk.dk_label.d_flags |= D_REMOVABLE;
			printf(" removable");
		}
		if (sa)
			printf(" %u*%d\n", sc->sc_blks, sc->sc_blksize);
#if 0
	} else if (i == STS_CHECKCOND &&
	    (strcmp(vendor, "HP") == 0 && strcmp(drive, "S6300.650A") == 0)) {
		/* XXX unformatted or nonexistent MO medium: fake it */
		sc->sc_blks = 318664;
		sc->sc_blksize = 1024;
#endif
	} else {
		/* take care of any contingent allegiance */
		(void)scsi_request_sense(sc->sc_unit.u_hba,
		    sc->sc_unit.u_targ, sc->sc_unit.u_unit,
		    (caddr_t)&sc->sc_sense.sense, sizeof sc->sc_sense.sense);
		sc->sc_blks = 10000000;	/* a random large number */
		sc->sc_blksize = 2048;	/* XXX for CD-ROM */
		sc->sc_dk.dk_label.d_flags |= D_DEFAULT;
		if (sa)
			if (sa->sa_si.si_qual & QUAL_RMB) {
				sc->sc_dk.dk_label.d_flags |= D_REMOVABLE;
				printf(" removable\n");
			} else
				printf(" unknown size\n");
	}

	if (sc->sc_blksize != DEV_BSIZE) {
		sc->sc_bshift = 0;
		for (i = sc->sc_blksize; i > DEV_BSIZE; i >>= 1)
			++sc->sc_bshift;
		if (i != DEV_BSIZE) {
			printf("%s: blksize not multiple of %d: cannot use\n",
			    sc->sc_dk.dk_dev.dv_xname, DEV_BSIZE);
			return;
		}
		sc->sc_blks <<= sc->sc_bshift;
	}
}

/*
 * Attach a disk (called after sdmatch returns true).
 * Note that this routine is never reentered (so we can use statics).
 */
void
sdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct sd_softc *sc = (struct sd_softc *)self;
	register struct scsi_attach_args *sa = aux;
	struct disklabel *lp = &sc->sc_dk.dk_label;
	int spc;
	register int i;
	char vendor[10], drive[17], rev[5];

	/*
	 * Declare our existence.
	 */
	sc->sc_unit.u_driver = &sdunitdriver;
	scsi_establish(&sc->sc_unit, &sc->sc_dk.dk_dev, sa->sa_unit);

	/*
	 * Figure out what kind of disk this is.
	 * We only accepted it if the inquiry succeeded, so
	 * we can inspect those fields.
	 */
	i = (sa->sa_si.si_version >> VER_ANSI_SHIFT) & VER_ANSI_MASK;
	if (i == 1 || i == 2) {
		scsi_inq_ansi((struct scsi_inq_ansi *)&sa->sa_si,
		    vendor, drive, rev);
		printf(": %s %s", vendor, drive);
		if (rev[0])
			printf(" rev %s", rev);
		if (i == 2)
			printf(" (SCSI-2)");
		else if ((sa->sa_si.si_v2info & V2INFO_RDF_MASK) ==
		    V2INFO_RDF_CCS)
			printf(" (CCS)");
		else
			printf(" (SCSI-1)");
	} else {
		/* bleah */
		bcopy("<unknown>", vendor, 10);
		bcopy("<unknown>", drive, 10);
		printf(": type 0x%x, qual 0x%x, ver %d",
		    sa->sa_si.si_type, sa->sa_si.si_qual,
		    sa->sa_si.si_version);
	}

#ifdef notyet
	sc->sc_dk.dk_driver = &sddkdriver;
	/* READ DISK LABEL HERE, UNLESS REMOVABLE MEDIUM... NEEDS THOUGHT */
#endif
	disk_attach(&sc->sc_dk);
	sc->sc_type = sa->sa_si.si_type;	/* sufficient? */
	sc->sc_dk.dk_stats.dk_bpms = 32 * (60 * DEV_BSIZE / 2);	/* XXX */
	sc->sc_dk.dk_stats.dk_secsize = DEV_BSIZE;
	lp->d_flags = D_SOFT;

	/*
	 * Print some comforting message about disk geometry and/or size.
	 */
	sdcapacity(sc, sa);

	/*
	 * No disk labels yet, build fake one until we call readdisklabel().
	 */
	lp->d_type = DTYPE_SCSI;
	strncpy(lp->d_typename, vendor, sizeof lp->d_typename);
	if ((i = strlen(vendor)) < sizeof lp->d_typename - 2) {
		lp->d_typename[i] = ' ';
		strncpy(&lp->d_typename[i+1], drive,
		    sizeof lp->d_typename - (i + 1));
	}
	lp->d_secsize = sc->sc_blksize;
	lp->d_npartitions = 8;

	/* create a temporary disklabel */
	lp->d_nsectors = 64;
	lp->d_ntracks = 8;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
	lp->d_ncylinders = howmany(sc->sc_blks, lp->d_secpercyl);
	lp->d_partitions[2].p_offset = 0;
	lp->d_partitions[2].p_size = sc->sc_blks;
	lp->d_partitions[2].p_fstype = FS_UNUSED;
	lp->d_partitions[0] = lp->d_partitions[2];

	sc->sc_flags |= SDF_ALIVE;
	/* all done */
}

/*
 * Reset a disk, after a SCSI bus reset.
 *
 * XXX untested and probably incomplete/incorrect
 */
void
sdreset(u)
	register struct unit *u;
{
	register struct sd_softc *sc = (struct sd_softc *)u->u_dev;

	printf(" %s", sc->sc_dk.dk_dev.dv_xname);
	sc->sc_resets++;
}

/* dev_t is short, must use prototype syntax */
int
sdopen(dev_t dev, int flags, int ifmt, struct proc *p)
{
	register int unit = sdunit(dev);
	register struct sd_softc *sc;
	daddr_t labelsect = LABELSECTOR;
	dev_t rawdev = RAWDEV(dev);
	char *msg;
	int stat, error = 0, s;
	extern int cold;

	if (unit >= sdcd.cd_ndevs || (sc = sdcd.cd_devs[unit]) == NULL)
		return (ENXIO);
	if ((sc->sc_flags & SDF_ALIVE) == 0 &&
	    !suser(p->p_ucred, &p->p_acflag))
		return (ENXIO);

	s = splbio();
	while (sc->sc_dk.dk_state != DK_OPEN &&
	    sc->sc_dk.dk_state != DK_OPENRAW &&
	    sc->sc_dk.dk_state != DK_CLOSED)
		if (error = tsleep((caddr_t)&sc->sc_dk, PRIBIO | PCATCH,
		    devopn, 0))
			break;
	splx(s);
	if (error)
		return (error);
	if (sc->sc_dk.dk_openmask)
		goto checkpart;	/* already is open, don't mess with it */

	/* beware of removable media */
	if ((flags & O_NONBLOCK) == 0) {
	again:
		stat = scsi_test_unit_ready(sc->sc_unit.u_hba,
		    sc->sc_unit.u_targ, sc->sc_unit.u_unit);
		if ((stat & STS_MASK) != STS_GOOD &&
		    (error = sderror(sc, stat))) {
			if (error == EAGAIN)
				goto again;
			if (error == ERESTART)
				error = EIO;
			return (error);
		}
		if (sc->sc_dk.dk_label.d_flags & D_REMOVABLE)
			sdcapacity(sc, 0);
	} else if (sc->sc_dk.dk_label.d_flags & D_REMOVABLE)
		sc->sc_dk.dk_label.d_flags |= D_DEFAULT;	/* unknown */

	sc->sc_dk.dk_state = DK_RDLABEL;
#ifdef i386
	{
	struct mbpart dospart;
	
	/*
	 * On (PC-style) 386, check for a DOS partition table,
	 * and if present, locate the label in the primary BSD partition.
	 * (Unfortunate that this has to be in a machine-independent
	 * source file, but that's what happens in machine-independent
	 * device drivers!)
	 */
	if ((flags & O_NONBLOCK) == 0 && getbsdpartition(rawdev, sdstrategy,
	    &sc->sc_dk.dk_label, &dospart) == 0)
	    	labelsect += dospart.start;
	}
#endif
	sc->sc_dk.dk_labelsector = labelsect;
	if ((flags & O_NONBLOCK) == 0) {
		if (msg = readdisklabel(rawdev, sdstrategy, &sc->sc_dk.dk_label,
		    labelsect)) {
			if ((sc->sc_dk.dk_label.d_flags & D_REMOVABLE) == 0)
				printf("%s: %s\n", sc->sc_dk.dk_dev.dv_xname,
				    msg);
			if (cold)
				sc->sc_dk.dk_state = DK_CLOSED;
			else
				sc->sc_dk.dk_state = DK_OPENRAW;
		} else {
			sc->sc_dk.dk_state = DK_OPEN;
			sc->sc_dk.dk_stats.dk_bpms =
			    (sc->sc_dk.dk_label.d_rpm / 60) *
			    sc->sc_dk.dk_label.d_nsectors * DEV_BSIZE;
		}
	} else
		sc->sc_dk.dk_state = DK_OPENRAW;
	wakeup((caddr_t)&sc->sc_dk);

checkpart:
	return (dkopenpart(&sc->sc_dk, dev, ifmt));
}

int
sdclose(dev_t dev, int flags, int ifmt, struct proc *p)
{
	register struct sd_softc *sc = sdcd.cd_devs[sdunit(dev)];

	sc->sc_format_pid = 0;
	return (dkclose(&sc->sc_dk, dev, flags, ifmt, p));
}

/*
 * This routine is called for partial block transfers and non-aligned
 * transfers (the latter only being possible on devices with a block size
 * larger than DEV_BSIZE).  The operation is performed in three steps
 * using a locally allocated buffer:
 *	1. transfer any initial partial block
 *	2. transfer full blocks
 *	3. transfer any final partial block
 */
static void
sdlblkstrat(bp, bsize)
	register struct buf *bp;
	register int bsize;
{
	register int bn, resid, boff, count;
	register caddr_t addr, cbuf;
	struct buf *tbp;

	MALLOC(cbuf, caddr_t, bsize, M_DEVBUF, M_WAITOK);
	MALLOC(tbp, struct buf *, sizeof *tbp, M_DEVBUF, M_WAITOK);

	bzero((caddr_t)tbp, sizeof tbp);
	tbp->b_proc = curproc;
	tbp->b_dev = bp->b_dev;
	bn = bp->b_blkno;
	resid = bp->b_bcount;
	addr = bp->b_un.b_addr;
#ifdef DEBUG
	if (sddebug & SDB_PARTIAL)
		printf("sdlblkstrat: bp %x flags %x bn %x resid %x addr %x\n",
		       bp, bp->b_flags, bn, resid, addr);
#endif

	while (resid > 0) {
		boff = dbtob(bn) & (bsize - 1);
		if (boff || resid < bsize) {
			struct sd_softc *sc = sdcd.cd_devs[sdunit(bp->b_dev)];
			sc->sc_partials++;
			count = MIN(resid, bsize - boff);
			tbp->b_flags = B_BUSY | B_READ;
			tbp->b_blkno = bn - btodb(boff);
			tbp->b_un.b_addr = cbuf;
			tbp->b_bcount = bsize;
#ifdef DEBUG
			if (sddebug & SDB_PARTIAL)
				printf(" readahead: bn %x cnt %x off %x addr %x\n",
				       tbp->b_blkno, count, boff, addr);
#endif
			sdstrategy(tbp);
			biowait(tbp);
			if (tbp->b_flags & B_ERROR) {
				bp->b_flags |= B_ERROR;
				bp->b_error = tbp->b_error;
				break;
			}
			if (bp->b_flags & B_READ) {
				bcopy(&cbuf[boff], addr, count);
				goto done;
			}
			bcopy(addr, &cbuf[boff], count);
#ifdef DEBUG
			if (sddebug & SDB_PARTIAL)
				printf(" writeback: bn %x cnt %x off %x addr %x\n",
				       tbp->b_blkno, count, boff, addr);
#endif
		} else {
			count = resid & ~(bsize - 1);
			tbp->b_blkno = bn;
			tbp->b_un.b_addr = addr;
			tbp->b_bcount = count;
#ifdef DEBUG
			if (sddebug & SDB_PARTIAL)
				printf(" fulltrans: bn %x cnt %x addr %x\n",
				       tbp->b_blkno, count, addr);
#endif
		}
		tbp->b_flags = B_BUSY | (bp->b_flags & B_READ);
		sdstrategy(tbp);
		biowait(tbp);
		if (tbp->b_flags & B_ERROR) {
			bp->b_flags |= B_ERROR;
			bp->b_error = tbp->b_error;
			break;
		}
done:
		bn += btodb(count);
		resid -= count;
		addr += count;
#ifdef DEBUG
		if (sddebug & SDB_PARTIAL)
			printf(" done: bn %x resid %x addr %x\n",
			       bn, resid, addr);
#endif
	}
	free(cbuf, M_DEVBUF);
	free(tbp, M_DEVBUF);
	biodone(bp);
}

/*
 * Start a transfer on sc as described by bp
 * (i.e., call hba or target start).
 * If in format mode, we may not need dma.
 */
#define sdstart(sc, bp) \
	((sc)->sc_format_pid && scsi_legal_cmds[(sc)->sc_cmd.cdb_bytes[0]] > 0 ? \
	    (*(sc)->sc_unit.u_start)((sc)->sc_unit.u_updev, \
		    &(sc)->sc_unit.u_forw, (struct buf *)NULL, \
		    sdigo, &(sc)->sc_dk.dk_dev) : \
	    (*(sc)->sc_unit.u_start)((sc)->sc_unit.u_updev, \
		    &(sc)->sc_unit.u_forw, bp, sdgo, &(sc)->sc_dk.dk_dev))

sdstrategy(bp)
	register struct buf *bp;
{
	register struct sd_softc *sc = sdcd.cd_devs[sdunit(bp->b_dev)];
	register int s;

	if (sc->sc_format_pid) {
		/* XXXXXXXXX SHOULD NOT COMPARE curproc IN HERE!?! */
		/*
		 * In format mode, only allow the owner to mess
		 * with the drive.  Skip all the partition checks.
		 */
		if (sc->sc_format_pid != curproc->p_pid) {
			bp->b_error = EPERM;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		bp->b_cylin = 0;
	} else {
		register daddr_t bn = bp->b_blkno;
		register int sz = howmany(bp->b_bcount, DEV_BSIZE);
		register struct partition *p;

		/*
		 * Make sure transfer is within partition.
		 * If it starts at the end, return EOF; if
		 * it extends past the end, truncate it.
		 */
		p = &sc->sc_dk.dk_label.d_partitions[sdpart(bp->b_dev)];
		if ((unsigned)bn >= p->p_size) {
			if ((unsigned)bn > p->p_size) {
				bp->b_error = EINVAL;
				bp->b_flags |= B_ERROR;
			} else
				bp->b_resid = bp->b_bcount;
			biodone(bp);
			return;
		}
		if (bn + sz > p->p_size) {
			sz = p->p_size - bn;
			bp->b_bcount = dbtob(sz);
		}
		/*
		 * If disk label write permission is off and
		 * this transfer would write the label, reject it.
		 */
		if ((bp->b_flags & B_READ) == 0 &&
		    bn + p->p_offset <= sc->sc_dk.dk_labelsector &&
		    bn + sz + p->p_offset > sc->sc_dk.dk_labelsector &&
		    sc->sc_dk.dk_wlabel == 0) {
			bp->b_error = EROFS;
			bp->b_flags |= B_ERROR;
			biodone(bp);
			return;
		}
		/*
		 * Non-aligned or partial-block transfers handled specially.
		 * SHOULD THIS BE AT A HIGHER LEVEL?
		 */
		s = sc->sc_blksize - 1;
		if ((dbtob(bn) & s) || (bp->b_bcount & s)) {
			sdlblkstrat(bp, sc->sc_blksize);
			return;
		}
		bp->b_cylin = (bn + p->p_offset) >> sc->sc_bshift;
	}

	/*
	 * Transfer valid, or format mode.  Queue the request
	 * on the drive, and maybe try to start it.
	 */
	s = splbio();
	disksort(&sc->sc_tab, bp);
	if (sc->sc_tab.b_active == 0) {
		sc->sc_tab.b_active = 1;
		sdstart(sc, bp);
	}
	splx(s);
}

static int
sderror(sc, stat)
	struct sd_softc *sc;
	int stat;
{
	struct scsi_sense *sn;
	char *s = sc->sc_dk.dk_dev.dv_xname;
	char *m;
	int key;

	sc->sc_fmt.sf_sense.status = stat;

	/*
	 * We always ask for sense after a potential error,
	 * to break up contingent allegiance.
	 */
	sn = (struct scsi_sense *)sc->sc_fmt.sf_sense.sense;
	stat = scsi_request_sense(sc->sc_unit.u_hba, sc->sc_unit.u_targ,
	    sc->sc_unit.u_unit, (caddr_t)sn, sizeof *sn);
	if ((stat & STS_MASK) != STS_GOOD) {
		sc->sc_fmt.sf_sense.status = stat;	/* ??? */
		printf("%s: sense failed, status %x\n", s, stat);
		return (EIO);
	}

	if ((sc->sc_fmt.sf_sense.status & STS_MASK) == STS_CHECKCOND) {
		if (!SENSE_ISXSENSE(sn) || !XSENSE_ISSTD(sn)) {
			printf("%s: scsi sense class %d, code %d\n",
			    s, SENSE_ECLASS(sn), SENSE_ECODE(sn));
			return (ERESTART);
		}
		key = XSENSE_KEY(sn);
		if (key == SKEY_ATTENTION)
			/* XXX complain if someone has this device open? */
			return (EAGAIN);
		if (key != SKEY_NONE) {
			/* print sense key specific data? */
			if (XSENSE_HASASCQ(sn))
				m = "%s: scsi sense: %s: asc 0x%x, ascq 0x%x\n";
			else if (XSENSE_HASASC(sn))
				m = "%s: scsi sense: %s: asc 0x%x\n";
			else
				m = "%s: scsi sense: %s\n";
			printf(m, s, SCSISENSEKEYMSG(key),
			    XSENSE_ASC(sn), XSENSE_ASCQ(sn));

			if (key == SKEY_MEDIUM_ERR || key == SKEY_HARD_ERR)
				/* attempt software retries */
				return (ERESTART);
			if (key != SKEY_RECOVERED)
				return (EIO);
		}
		return (0);
	}
#ifdef DEBUG
	stat = sc->sc_fmt.sf_sense.status;
	if (stat != -1 && (stat & STS_MASK) != STS_GOOD)
		printf("%s: scsi status %d\n", s, stat);
#endif
	return ((sc->sc_fmt.sf_sense.status & STS_MASK) != STS_GOOD ?
	    ERESTART : 0);
}

/*
 * sdigo is called from the hba driver when it has got the scsi bus
 * for us, and we were doing a format op that did not need dma.
 */
void
sdigo(sc0, cdb)
	struct device *sc0;
	struct scsi_cdb *cdb;
{
	register struct sd_softc *sc = (struct sd_softc *)sc0;
	register struct buf *bp = sc->sc_tab.b_actf;
	register int stat;

	stat = (*sc->sc_unit.u_hbd->hd_icmd)(sc->sc_unit.u_hba,
	    sc->sc_unit.u_targ, &sc->sc_cmd, bp->b_un.b_addr, bp->b_bcount,
	    bp->b_flags & B_READ);
	sc->sc_sense.status = stat;
	if (stat & 0xfe) {		/* XXX */
		(void) sderror(sc, stat);
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
	}
	/*
	 * Done with SCSI bus, before we `ought' to be.  Release it.
	 */
	(*sc->sc_unit.u_rel)(sc->sc_unit.u_updev);
	bp->b_resid = 0;
	sc->sc_tab.b_errcnt = 0;
	sc->sc_tab.b_actf = bp->b_actf;
	biodone(bp);
	if ((bp = sc->sc_tab.b_actf) == NULL)
		sc->sc_tab.b_active = 0;
	else
		sdstart(sc, bp);
}

/*
 * sdgo is called from the hba driver or target code when it has
 * allocated the scsi bus and DMA resources and target datapath for us.
 */
void
sdgo(sc0, cdb)
	struct device *sc0;
	register struct scsi_cdb *cdb;
{
	register struct sd_softc *sc = (struct sd_softc *)sc0;
	register struct buf *bp = sc->sc_tab.b_actf;
	register int n;
	register unsigned int u;

	if (sc->sc_format_pid) {
		*cdb = sc->sc_cmd;
		n = 0;
	} else {
		CDB10(cdb)->cdb_cmd = bp->b_flags & B_READ ? CMD_READ10 :
		    CMD_WRITE10;
		CDB10(cdb)->cdb_lun_rel = sc->sc_unit.u_unit << 5;
		u = bp->b_cylin;
		CDB10(cdb)->cdb_lbah = u >> 24;
		CDB10(cdb)->cdb_lbahm = u >> 16;
		CDB10(cdb)->cdb_lbalm = u >> 8;
		CDB10(cdb)->cdb_lbal = u;
		CDB10(cdb)->cdb_xxx = 0;
		n = sc->sc_blksize - 1;
		u = (bp->b_bcount + n) >> (DEV_BSHIFT + sc->sc_bshift);
		CDB10(cdb)->cdb_lenh = u >> 8;
		CDB10(cdb)->cdb_lenl = u;
		CDB10(cdb)->cdb_ctrl = 0;
		n = (bp->b_bcount & n) != 0;
#ifdef DEBUG
		if (n)
			printf("%s: partial block xfer -- %x bytes\n",
			    sc->sc_dk.dk_dev.dv_xname, bp->b_bcount);
#endif
		sc->sc_transfers++;
	}
	if ((*sc->sc_unit.u_go)(sc->sc_unit.u_updev, sc->sc_unit.u_targ,
	    sdintr, (void *)sc, bp, n) == 0) {
		sc->sc_dk.dk_stats.dk_busy = 1;
		sc->sc_dk.dk_stats.dk_xfers++;
		sc->sc_dk.dk_stats.dk_sectors += bp->b_bcount / DEV_BSIZE;
		return;
	}
	/*
	 * Some sort of nasty unrecoverable error: clobber the
	 * transfer.  Call the bus release function first, though.
	 */
	(*sc->sc_unit.u_rel)(sc->sc_unit.u_updev);
#ifdef DEBUG
	if (sddebug & SDB_ERROR)
		printf("%s: sdgo: %s adr %d blk %d len %d ecnt %d\n",
		    sc->sc_dk.dk_dev.dv_xname,
		    bp->b_flags & B_READ? "read" : "write",
		    bp->b_un.b_addr, bp->b_cylin, bp->b_bcount,
		    sc->sc_tab.b_errcnt);
#endif
	bp->b_flags |= B_ERROR;
	bp->b_error = EIO;
	bp->b_resid = 0;
	sc->sc_tab.b_errcnt = 0;
	sc->sc_tab.b_actf = bp->b_actf;
	biodone(bp);
	if ((bp = sc->sc_tab.b_actf) == NULL)
		sc->sc_tab.b_active = 0;
	else
		sdstart(sc, bp);
}

/*
 * A transfer finished (or, someday, disconnected).
 * We are already off the target/hba queues.
 * Restart this one for error recovery, or start the next, as appropriate.
 */
void
sdintr(sc0, stat, resid)
	struct device *sc0;
	int stat, resid;
{
	register struct sd_softc *sc = (struct sd_softc *)sc0;
	register struct buf *bp = sc->sc_tab.b_actf;
	int error;

	if (bp == NULL)
		panic("sdintr");
	sc->sc_dk.dk_stats.dk_busy = 0;
	if ((stat & STS_MASK) != STS_GOOD) {
#ifdef DEBUG
		if (sddebug & SDB_ERROR)
			printf("%s: sdintr scsi status 0x%x resid %d\n",
			    sc->sc_dk.dk_dev.dv_xname, stat, resid);
#endif
		if (error = sderror(sc, stat)) {
			if (error == EAGAIN)
				/* harmless, restartable */
				goto restart;
			if (error == ERESTART) {
				/* medium or hardware error, try again */
				if (++sc->sc_tab.b_errcnt <= SDRETRY) {
					printf("%s: retry %d\n",
					    sc->sc_dk.dk_dev.dv_xname,
					    sc->sc_tab.b_errcnt);
					goto restart;
				}
				error = EIO;
			}
			bp->b_flags |= B_ERROR;
			bp->b_error = error;
		}
	}
	bp->b_resid = resid;
	sc->sc_tab.b_errcnt = 0;
	sc->sc_tab.b_actf = bp->b_actf;
	biodone(bp);
	if ((bp = sc->sc_tab.b_actf) == NULL)
		sc->sc_tab.b_active = 0;
	else {
restart:
		sdstart(sc, bp);
	}
}

int
sdioctl(dev_t dev, int cmd, register caddr_t data, int flag, struct proc *p)
{
	register struct sd_softc *sc = sdcd.cd_devs[sdunit(dev)];
	char *msg;
	int wlab;
	int error;
	int val;

	switch (cmd) {

	case SDIOCSFORMAT:
		/* take this device into or out of "format" mode */
		if (suser(p->p_ucred, &p->p_acflag))
			return (EPERM);
		if (*(int *)data) {
			if (sc->sc_format_pid)
				return (EPERM);
			sc->sc_format_pid = p->p_pid;
		} else
			sc->sc_format_pid = 0;
		break;

	case SDIOCGFORMAT:
		/* find out who has the device in format mode */
		*(int *)data = sc->sc_format_pid;
		break;

	case SDIOCADDCOMMAND:
		val = *(int *)data;

		if (val < 0 || val >= 256)
			return (EINVAL);

		scsi_legal_cmds[val] = 1;
		break;
			
	case SDIOCSCSICOMMAND:
#define cdb ((struct scsi_cdb *)data)
		/*
		 * Save what user gave us as SCSI cdb to use with next
		 * read or write to the char device.
		 */
		if (sc->sc_format_pid != p->p_pid)
			return (EPERM);
		if (scsi_legal_cmds[cdb->cdb_bytes[0]] == 0)
			return (EINVAL);
		sc->sc_cmd = *cdb;
#undef	cdb
		break;

	case SDIOCSENSE:
		/*
		 * Return the SCSI sense data saved after the last
		 * operation that completed with "check condition" status.
		 */
		*(struct scsi_fmt_sense *)data = sc->sc_sense;
		break;

	/* XXX this crap moves out when we get Chris's generic disk driver */
	case DIOCGDINFO:
		*(struct disklabel *)data = sc->sc_dk.dk_label;
		break;

	case DIOCGPART:
		((struct partinfo *)data)->disklab = &sc->sc_dk.dk_label;
		((struct partinfo *)data)->part =
		    &sc->sc_dk.dk_label.d_partitions[sdunit(dev)];
		break;    

	case DIOCSDINFO:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		error = setdisklabel(&sc->sc_dk.dk_label,
		    (struct disklabel *)data,
		    (sc->sc_dk.dk_state == DK_OPENRAW) ? 0 :
		    sc->sc_dk.dk_openmask);
		if (error == 0) {
			if (sc->sc_dk.dk_state == DK_OPENRAW)
				sc->sc_dk.dk_state = DK_OPEN;
#ifdef i386
			sc->sc_dk.dk_labelsector =
			    sc->sc_dk.dk_label.d_bsd_startsec + LABELSECTOR;
#endif
		}
		return (error);

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		sc->sc_dk.dk_wlabel = *(int *)data;
		break;

	case DIOCWDINFO:
#define	RAWPART	2	/* 'c' partition XXX */
		if ((flag & FWRITE) == 0)
			return (EBADF);
		if (error = setdisklabel(&sc->sc_dk.dk_label,
		    (struct disklabel *)data,
		    (sc->sc_dk.dk_state == DK_OPENRAW) ? 0 :
		    (sc->sc_dk.dk_bopenmask |
		    (sc->sc_dk.dk_copenmask &~ (1 << RAWPART)))))
			return (error);    
		if (sc->sc_dk.dk_state == DK_OPENRAW)
			sc->sc_dk.dk_state = DK_OPEN;
#ifdef i386
		sc->sc_dk.dk_labelsector = sc->sc_dk.dk_label.d_bsd_startsec +
		    LABELSECTOR;
#endif
		/* XXX waiting for Chris's generic disk driver :-) */
		wlab = sc->sc_dk.dk_wlabel;
		sc->sc_dk.dk_wlabel = 1;
		error = writedisklabel(RAWDEV(dev), sdstrategy,
		    &sc->sc_dk.dk_label, sc->sc_dk.dk_labelsector);
		sc->sc_dk.dk_wlabel = wlab;
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

int
sdsize(dev_t dev)
{
	register int unit = sdunit(dev);
	register struct sd_softc *sc;
	
	if (unit >= sdcd.cd_ndevs || (sc = sdcd.cd_devs[unit]) == NULL ||
	    (sc->sc_flags & SDF_ALIVE) == 0)
		return (-1);
	if (sdopen(dev, 0, 0, curproc))
		return (-1);
	return (sc->sc_dk.dk_label.d_partitions[sdpart(dev)].p_size);
}

/*
 * The new dump routine takes different parameters from the old.
 * In particular, certain ugly global variables are now passed
 * as parameters to the dump code.
 */
int
#ifdef __STDC__
sddump(dev_t dev)
#else
sddump(dev)
	dev_t dev;
#endif
{
	extern int dumpsize;

	return (sddump2(dev, (daddr_t)dumplo, (caddr_t)0, ctob(dumpsize), 1));
}

#define	sddump	sddump2

/*
 * Write `len' bytes from address `addr' to drive and partition in `dev',
 * at block blkoff from the beginning of the partition.  The address is
 * either kernel virtual or physical (some machines may never use one or
 * the other, but we need it in the protocol to stay machine-independent).
 */
int
sddump(dev_t dev, daddr_t blkoff, caddr_t addr, int len, int isphys)
{
	register struct sd_softc *sc;
	register struct partition *p;
	register daddr_t bn, n, nblks;
	register struct hba_softc *hba;
	register int stat, unit;
	struct scsi_cdb cdb;

	/* drive ok? */
	unit = sdunit(dev);
	if (unit >= sdcd.cd_ndevs || (sc = sdcd.cd_devs[unit]) == NULL ||
	    (sc->sc_flags & SDF_ALIVE) == 0)
		return (ENXIO);

	/* blocks in range? */
	p = &sc->sc_dk.dk_label.d_partitions[sdpart(dev)];
	n = (len + sc->sc_blksize - 1) >> DEV_BSHIFT;
	if (blkoff < 0 || blkoff >= p->p_size || blkoff + n > p->p_size)
		return (EINVAL);
	bn = blkoff + p->p_offset;
	bn >>= sc->sc_bshift;

	/* scsi bus idle? */
	hba = sc->sc_unit.u_hba;
	if (hba->hba_head) {
		(*hba->hba_driver->hd_reset)(hba, 0);
		printf("[reset %s] ", sc->sc_dk.dk_dev.dv_xname);
	}

	CDB10(&cdb)->cdb_cmd = CMD_WRITE10;
	CDB10(&cdb)->cdb_lun_rel = sc->sc_unit.u_unit << 5;
	CDB10(&cdb)->cdb_xxx = 0;
	CDB10(&cdb)->cdb_ctrl = 0;

#define	DUMP_MAX	(32 * 1024)	/* no more than 32k per write */
	for (;;) {
		if ((n = len) > DUMP_MAX)
			n = DUMP_MAX;
		CDB10(&cdb)->cdb_lbah = bn >> 24;
		CDB10(&cdb)->cdb_lbahm = bn >> 16;
		CDB10(&cdb)->cdb_lbalm = bn >> 8;
		CDB10(&cdb)->cdb_lbal = bn;
		nblks = n >> (DEV_BSHIFT + sc->sc_bshift);
		CDB10(&cdb)->cdb_lenh = nblks >> 8;
		CDB10(&cdb)->cdb_lenl = nblks;
		stat = hba->hba_driver->hd_dump(hba, sc->sc_unit.u_targ,
		    &cdb, addr, n, isphys);
		if ((stat & STS_MASK) != STS_GOOD) {
			printf("%s: scsi write error 0x%x\ndump ",
			    sc->sc_dk.dk_dev.dv_xname, stat);
			return (EIO);
		}
		if ((len -= n) == 0)
			return (0);
		addr += n;
		bn += nblks;
	}
}
