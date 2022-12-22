/*-
 * Copyright (c) 1992, 1993, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: fd.c,v 1.20 1994/01/29 22:42:32 karels Exp $
 */

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Don Ahn.
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
 *	@(#)fd.c	7.4 (Berkeley) 5/25/91
 */

#include "fd.h"
#if NFDC > 0

#if NFDC > 1
sorry, only 1 floppy disk controller is supported now
#endif

#if NFD > 2
sorry, only 2 disk drives are supported now (see fdprobe() and fdattach())
#endif

/* #define	FDDEBUG */
/* #define	FDTEST */
/* #define	FDOTHER */
/* int fddebug = 0; */

#include "param.h"
#include "systm.h"
#include "conf.h"
#include "file.h"
#include "ioctl.h"
#include "buf.h"
#include "uio.h"
#include "disklabel.h"
#include "syslog.h"
#include "rtc.h"
#include "device.h"
#include "disk.h"
#include "stat.h"

#include "dma.h"
#include "isavar.h"
#include "fdreg.h"
#include "icu.h"

#define FDTIMO	1		/* timeout in seconds */

/*
 * Minor number: low 3 bits are partition, next 2 are unit,
 * and remaining 3 are the type, if forcing a particular floppy type.
 */
#define	FDPART(s)	(minor(s) & 0x7)
#define	FDUNIT(s)	((minor(s) >> 3) & 0x3)
#define	FDTYPE(s)	((minor(s) >> 5) & 0x7)
#define	fdminor(unit,type)	(((unit) << 3) | (type) << 4)

#define b_cylin b_resid
#define FDBLK 512

/*
 * Description of floppy disk types, where the same type of disk
 * in different drives may require different parameters.
 */
struct fd_type {
	int	sectrac;		/* sectors per track         */
	int	secsizecode;		/* size code for sectors     */
	int	tracks;			/* total num of tracks       */
	int	size;			/* size of disk in sectors   */
	int	datalen;		/* data len when secsize = 0 */
	int	gap;			/* gap len between sectors   */
	int	fgap;			/* format gap len            */
	int	steptrac;		/* steps per cylinder        */
	int	trans;			/* transfer speed code       */
} fd_types[] = {
		/*** *principal types* ***/
	{  9,2,40, 720, 0xFF,0x2A,0x50,1,FDC_250KBPS },	/* 360k in DD 5.25*/
#define	DD5_360		0
	{ 15,2,80,2400, 0xFF,0x1B,0x54,1,FDC_500KBPS },	/* 1.2m in HD 5.25*/
#define	HD5_1200	1
	{  9,2,80,1440, 0xFF,0x2A,0x50,1,FDC_250KBPS },	/* 720k in DD 3.5 */
#define	DD3_720		2
 	{ 18,2,80,2880, 0xFF,0x1B,0x6C,1,FDC_500KBPS },	/* 1.44 in HD 3.5 */
#define	HD3_1440	3

		/*** additional types ***/
	{  9,2,80,1440, 0xFF,0x23,0x50,1,FDC_300KBPS },	/* 720k in HD 5.25*/
#define	HD5_720		4
	{  9,2,40, 720, 0xFF,0x23,0x50,2,FDC_300KBPS },	/* 360k in HD 5.25*/
#define	HD5_360		5
	{  9,2,80,1440, 0xFF,0x2A,0x50,1,FDC_250KBPS },	/* 720k in HD 3.5 */
#define	HD3_720		6

#ifdef notyet
 	{ 36,2,80,5760, 0xFF,???,???,1,FDC_500KBPS? },	/* 2.88 in ?D 3.5 */
#define	HD3_2880	7
#endif
};

#define	NUMTYPES	(sizeof(fd_types)/sizeof(fd_types[0]))


/*
 * For every type of drive (principal type), we list the
 * floppy types possible for this drive, in decreasing size.
 * The first one is the default if a disk is not readable.
 * Terminated with -1.
 */
static int fd_dd5[] = { DD5_360, -1 };
static int fd_hd5[] = { HD5_1200, HD5_720, HD5_360, -1 };
static int fd_dd3[] = { DD3_720, -1 };
static int fd_hd3[] = { HD3_1440, HD3_720, -1 };
#ifdef notyet
static int fd_qd3[] = { HD3_2880, HD3_1440, HD3_720, -1 };
#endif

/*
 * Hardware drive types.  The index into this array is one less than
 * the drive type returned by the hardware.
 * Drive type vs floppy disk type compatibility:
 *	(fd_drivetypes[drtype].diskcompat & (1<<fltype)) == 1
 * iff device supports fltype.
 */
#define	b(v)	(1 << (v)) 
struct fd_drivetype {
	char	*drivename;
	int	diskcompat;
	int	*disktypes;
} fd_drivetypes[] = {
	{ "360K DD 5.25", b(DD5_360), fd_dd5 },
	{ "1.2M HD 5.25", b(HD5_1200)|b(HD5_720)|b(HD5_360), fd_hd5 },
	{  "720K DD 3.5", b(DD3_720), fd_dd3 },
	{ "1.44M HD 3.5", b(HD3_1440)|b(HD3_720), fd_hd3 },
	{ NULL, 0, NULL },
#ifdef notyet
	{ "2.88M HD 3.5", b(HD3_2880)|b(HD3_1440)|b(HD3_720), fd_qd3 }
#else
	{ "2.88M HD 3.5 (using as 1.44)", b(HD3_1440)|b(HD3_720), fd_hd3 }
#endif
};

#undef b
#define	NUMDRTYPES	(sizeof(fd_drivetypes)/sizeof(fd_drivetypes[0]))

enum States { 	/* driver states */
	Idle,	/* Idle is first - as always :-) */
	Do_seek, Seek_done,
	Do_cmd, Cmd_done,
	Do_specify, Do_reset
};

struct fdc_softc {
	struct	device sc_dev;	/* base device */
	struct	isadev sc_id;	/* ISA device */
	struct	intrhand sc_ih;	/* interrupt vectoring */
	int	base;		/* io base register */
	int	dmachan;
	int	ndrives;	/* number of drives on this controller */
	enum	States state;
	int	unit;		/* selected unit */
	short	status[7];
	struct	buf head;	/* drive queue */
	int	quiet;
} fdc_softc;

struct fd_softc {
	struct	device sc_dev;	/* base device */
	struct	isadev sc_id;	/* ISA device */
	int	drtype;		/* Drive type - index to fd_drivetypes */
	int	drnum;		/* Drive number */
	struct	fd_type *type;	/* current disk parms */
	int	motor:1,	/* Motor on flag */
		reset:1,
		nodrive:1,	/* no drive for this unit */
		haslabel:1,	/* dlabel has disk label now */
		settype:1,	/* type set for disk */
		quiet:1,	/* doing fdauto read, quiet about errs */
		wlabel:1;	/* label writable? */
	int	track;		/* current track */
		/* current transfer */
	long	daddr;		/* disk address */
	char	*addr;		/* memory addr */
	int	count;		/* bytes left */
	int	retry;		/* number of retries */
	struct	buf head;	/* Head of buf chain */
	struct	disklabel dlabel;
	int	open_state;
	int	openmask;
	int	bopenmask;
	int	copenmask;
} fd_softc[NFD];

extern int hz;

/****************************************************************************/
/*                      autoconfiguration stuff                             */
/****************************************************************************/

static	int fdprobe();
int	Fdstrategy(), fdturnoff(), fdcprobe();
static	void fdattach();
void	fdgetlabel(), Fdattach(), fdcattach();

/* controller */
struct cfdriver fdccd =
	{ NULL, "fdc", fdcprobe, fdcattach, sizeof(struct fdc_softc) };

/* drives */
struct cfdriver fdcd =
	{ NULL, "fd", fdprobe, fdattach, sizeof(struct fd_softc) };

/* ARGSUSED */
static int
fdprint(aux, name)
	void *aux;
	char *name;
{
	struct fd_softc *sc = (struct fd_softc *)aux;

	if (name)
		printf(" at %s", name);
	printf(" slave %d: %s", sc->drnum, 
	    fd_drivetypes[sc->drtype].drivename);
	
	return (UNCONF);
}
	
fdcprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	void fdforceintr();

	if (cf->cf_unit != 0) {
		printf("fdc%d not supported\n", cf->cf_unit);
		return (0);
	}
	fdc_softc.base = ia->ia_iobase;
	ia->ia_drq = fdc_softc.dmachan = 2;	/* DMA channel 2 */
	if (!fdccheck(&fdc_softc))
		return (0);
	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(fdforceintr, NULL);
		fdturnoff(0);
		if (ffs(ia->ia_irq) - 1 == 0)
			return (0);
	}
	ia->ia_iosize = FD_NPORT;
#ifdef notdef
	return (rtcin(RTC_EQUIP) & 1);	/* some machines get this wrong! */
#else
	return (1);
#endif
}

/*
 * Used by isa_discoverintr to force card to tell use where it is.
 */
void
fdforceintr()
{
	int unitstate;

	fdc_softc.ndrives = 1;
	fdturnon(0);
	out_fdc(NE7CMD_SEEK);
	out_fdc(0);	/* unit */
	out_fdc(1);	/* track */ 
}

fdprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	if (fd_softc[cf->cf_unit].nodrive)
		return (0);
	else
		return (1);
}

/*
 * one call per controller 
 */
void
fdcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;	
{	
	register struct fdc_softc *sc = (struct fdc_softc *) self;	
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int d, t, sep = 0;
	register struct fd_softc *fd;
	int fdintr();
	
	printf("\n");
	for (d = 0; d < NFD; d++) {
		fd = &fd_softc[d];
		t = rtcin(RTC_FLOPPY);
		t = (d ? t : t >> 4) & 0xf;
		if (t == 0 || t > NUMDRTYPES ||
		    fd_drivetypes[t - 1].drivename == NULL) {
			if (t)
				printf("fd%d: unknown drive type %d\n", d, t);
			fd->nodrive = 1;
			continue;
		} 
		--t;
		fd->drtype = t;
		fd->nodrive = 0;
		fd->drnum = d;
		(void) config_found(&sc->sc_dev, (void *) fd, fdprint);
		fd->track = -1;
		fdturnoff(d);
	}	
	if (fdc_softc.ndrives)
		at_setup_dmachan(fdc_softc.dmachan, FDBLK);
	fdc_softc.sc_dev.dv_unit = sc->sc_dev.dv_unit;

	isa_establish(&sc->sc_id, &sc->sc_dev);
	sc->sc_ih.ih_fun = fdintr;
	sc->sc_ih.ih_arg = (void *) sc;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_DISK);
}

/* ARGSUSED */
void
fdattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;	
{	
	struct fd_softc *sc = (struct fd_softc *) aux;

	printf("\n");
	fdc_softc.ndrives = sc->drnum + 1;
	fdc_softc.unit = sc->drnum;
}

/****************************************************************************/
/*                           fdopen/fdclose                                 */
/****************************************************************************/
/* ARGSUSED */
Fdopen(dev, flags, fmt, p)
        dev_t	dev;
        int	flags, fmt;
	struct	proc *p;
{
	register struct fd_softc *fd;
	register unsigned int unit = FDUNIT(dev);
	unsigned int type = FDTYPE(dev);
	int error = 0;
	int s;
	int mask;
	
	if (unit >= fdc_softc.ndrives || (fd = &fd_softc[unit])->nodrive)
		return (ENXIO);

	s = splbio();
	while (fd->open_state != DK_OPEN && fd->open_state != DK_OPENRAW &&
	    fd->open_state != DK_CLOSED)
		if (error = tsleep((caddr_t)fd, PRIBIO | PCATCH, devopn, 0))
			break;
	splx(s);
	if (error)
		return (error);
	if (fd->openmask)
		goto checkpart;

	fdchangedisk(FDUNIT(dev));

	if (flags & O_NONBLOCK)
		fd->open_state = DK_WANTOPENRAW;
	else
		fd->open_state = DK_WANTOPEN;

	if (type) {
		type--;			/* array has origin 0 */
		if (type >= NUMTYPES || 
		    (fd_drivetypes[fd->drtype].diskcompat & (1<<type)) == 0) {
			error = ENXIO;
			goto done;
		}
		if (fd->settype && fd->type != &fd_types[type]) {
			error = EBUSY;
			goto done;
		}
		fd->type = &fd_types[type];
		fd->settype = 1;
	} else if (!fd->settype) {
		if (fdauto(dev, fd)) {
			log(LOG_WARNING, "fd%d: no disk in drive\n",
			    FDUNIT(dev));
			error = EIO;
			goto done;
		}
	}
	if (!fd->haslabel)
		fdgetlabel(dev, fd);

	if (fd->open_state == DK_WANTOPENRAW)
		fd->open_state = DK_OPENRAW;
	else
		fd->open_state = DK_OPEN;

checkpart:
        if (FDPART(dev) >= fd->dlabel.d_npartitions) {
                error = ENXIO;
		goto done;
	}

	mask = 1 << FDPART(dev);
	fd->openmask |= mask;
	switch (fmt) {
	case S_IFCHR:
		fd->copenmask |= mask;
		break;
	case S_IFBLK:
		fd->bopenmask |= mask;
		break;
	}

done:
	if (error && (fd->open_state == DK_WANTOPEN ||
	    fd->open_state == DK_WANTOPENRAW))
		fd->open_state = DK_CLOSED;

	wakeup((caddr_t) fd);
	return (error);
}

void
fdgetlabel(dev, fd)
	dev_t dev;
	register struct fd_softc *fd;
{
	register struct disklabel *lp = &fd->dlabel;
	struct fd_type *fdt = fd->type;
	
	/*
	 * Fake up a disk label for floppies without one.
	 */
	bzero(lp, sizeof(struct disklabel));
	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_type = DTYPE_FLOPPY;
	lp->d_secsize = FDBLK;
	lp->d_nsectors = fdt->sectrac;
	lp->d_ncylinders = fdt->tracks;
	lp->d_secperunit = fdt->size;
	lp->d_secpercyl = lp->d_secperunit / lp->d_ncylinders;
	lp->d_ntracks = lp->d_secpercyl / lp->d_nsectors;
	lp->d_rpm = 360;
	lp->d_partitions[0].p_size = lp->d_secperunit;
	lp->d_partitions[0].p_offset = 0;
	lp->d_partitions[0].p_fsize = DEV_BSIZE;
	lp->d_partitions[0].p_fstype = FS_BSDFFS;
	lp->d_partitions[0].p_frag = 8;
	lp->d_partitions[0].p_cpg = 32;
	lp->d_partitions[2] = lp->d_partitions[0];
	lp->d_npartitions = 3;

	if (fd->open_state == DK_WANTOPENRAW) {
		fd->haslabel = 1;
		return;
	}

	/*
	 * If readdisklabel fails, we will use the label constructed above
	 * according to the drive and disk types.  Don't print warning
	 * message, as most floppies will be unlabeled.
	 *
	 * Also, turn on quiet flag so we don't chatter about an 
	 * unformatted floppy at this point.  Let the single error
	 * message come out when the user level read happens.
	 */
	fd->quiet = 1;
#ifdef FDDEBUG
	{
	char *msg = readdisklabel(dev, Fdstrategy, lp, LABELSECTOR);

	if (msg && fddebug)
		printf("%s\n", msg);
	}
#else
	(void) readdisklabel(dev, Fdstrategy, lp, LABELSECTOR);
#endif
	fd->haslabel = 1;
	fd->quiet = 0;
}

Fdclose(dev, flags, fmt)
	dev_t dev;
	int flags;
	int fmt;
{
	struct fd_softc *fd = &fd_softc[FDUNIT(dev)];
	int mask = 1 << FDPART(dev);

	switch (fmt) {
	case S_IFCHR:
		fd->copenmask &= ~mask;
		break;
	case S_IFBLK:
		fd->bopenmask &= ~mask;
		break;
	}

	if (((fd->copenmask | fd->bopenmask) & mask) == 0)
		fd->openmask &= ~mask;

	if (fd->openmask == 0) {
		int s = splhigh();

		while (fd->head.b_actf)
			sleep((caddr_t)fd, PRIBIO);
		splx(s);
		fd->open_state = DK_CLOSED;
		fdturnoff(FDUNIT(dev));
	}

	return (0);
}

/****************************************************************************/
/*                               fdstrategy                                 */
/****************************************************************************/
Fdstrategy(bp)
	register struct buf *bp;	/* IO operation to perform */
{
	register struct buf *dp;
	long nblocks, blknum;
	int s;
 	dev_t dev = bp->b_dev;
 	int unit = FDUNIT(dev);
	struct fd_softc *fd = &fd_softc[unit];
	struct partition *p;

#ifdef FDTEST
	if (fddebug)
		printf("fdstrat%d, blk = %d, bcount = %d, addr = %x|",
			unit, bp->b_blkno, bp->b_bcount,bp->b_un.b_addr);
#endif

	if ((unit >= NFD) || (bp->b_blkno < 0))
		goto bad;

	/* don't start transfers after disk change */
	if (fd->settype == 0) {
		bp->b_error = EIO;
		goto bad2;
	}

	/*
	 * check block number and floppy/partition size.
	 */
	p = &fd->dlabel.d_partitions[FDPART(dev)]; 
	blknum = (unsigned long) bp->b_blkno * DEV_BSIZE / FDBLK;
	nblocks = p->p_size;

	if (blknum + (bp->b_bcount / FDBLK) > nblocks) {
		/* if reading exactly at end of disk, return an EOF */
		if (blknum == nblocks && bp->b_flags & B_READ) {
			bp->b_resid = bp->b_bcount;
			biodone(bp);
			return;
		}
		/* truncate the operation if part of it fits */
		if ((nblocks -= blknum) <= 0)
			goto bad;
		bp->b_bcount = nblocks * FDBLK;
	}
	/* XXX 2 assumes double-sided? */
 	bp->b_cylin = (blknum + p->p_offset) / (fd->type->sectrac * 2);

	dp = &fd_softc[unit].head;
	s = splbio();
	disksort(dp, bp);
	if (dp->b_active == 0)
		fdustart(unit);		/* start drive if idle */
	splx(s);
	return;

bad:
	bp->b_error = EINVAL;
bad2:
#ifdef FDDEBUG
	if (fddebug) {
		printf("fd%d: strat: blkno = %d, bcount = %d\n",
			unit, bp->b_blkno, bp->b_bcount);
		/* pg("fd:error in fdstrategy"); */
	}
#endif
	bp->b_flags |= B_ERROR;
	biodone(bp);
}

/****************************************************************************/
/*                            motor control stuff                           */
/****************************************************************************/
set_motor(unit, reset)
	int unit, reset;
{
	static int moen[] = {FDO_MOEN0, FDO_MOEN1, FDO_MOEN2, FDO_MOEN3};
	int out = unit;
	int i;
	
	if (!reset)
		out |= FDO_FRST | FDO_FDMAEN;
		
	for (i = 0; i < fdc_softc.ndrives; i++)
		if (fd_softc[i].motor)	
			out |= moen[i];
	outb(fdc_softc.base + fdout, out);
}

fdturnoff(unit)
	int unit;
{
	int s = splbio();

	fd_softc[unit].motor = 0;
	set_motor(fdc_softc.unit, 0);		/* ??? */
	splx(s);
}

fdturnon(unit)
	int unit;
{

	fd_softc[unit].motor = 1;
	set_motor(unit, 0);
}

/****************************************************************************/
/*                             fdc in/out                                   */
/****************************************************************************/
/* XXX should take controller structure pointer */
int
in_fdc()
{
	int i;

	while ((i = inb(fdc_softc.base+fdsts) & (NE7_DIO|NE7_RQM)) !=
	    (NE7_DIO|NE7_RQM))
		if (i == NE7_RQM)
			return (-1);
	return (inb(fdc_softc.base + fddata));
}

/* XXX should take controller structure pointer */
out_fdc(x)
	int x;
{
	int r, errcnt;
#ifdef FDOTHER
	static int maxcnt = 0;
#endif

#define MAX_TRIES 10000
	for (errcnt = 0; errcnt < MAX_TRIES; errcnt++) {
		r = (inb(fdc_softc.base+fdsts) & (NE7_DIO|NE7_RQM));
		if (r == NE7_RQM)
			break;
		if (r == (NE7_DIO|NE7_RQM)) {
			/* error: direction. eat up output */
			fdgetstatus();
			fdshowstatus("unexpected result data");
		}
	}
#ifdef FDOTHER
	if (errcnt > maxcnt) {
		maxcnt = errcnt;
		if (fddebug)
			printf("New MAX = %d\n",maxcnt);
	}
#endif
	if (errcnt >= MAX_TRIES) {
#ifdef FDTEST
		if (fdc_softc.quiet == 0 || fddebug)
#else
		if (fdc_softc.quiet == 0)
#endif
		    printf("fd: out_fdc() failed: status 0x%x\n", r);
		return (0);
	}
	outb(fdc_softc.base+fddata, x);
	return (1);
#undef MAX_TRIES
}

/* see if fdc responding.  used only from probe... */
int
fdccheck(fdp)
	struct fdc_softc *fdp;
{
	int i;

	for (i = 0; i < 100 ; i++) 
		if ((inb(fdp->base + fdsts) & NE7_RQM) == NE7_RQM) 
			return (1);
	return (0);
}

/****************************************************************************/
/*                          fdustart/fdstart                                */
/****************************************************************************/

fdustart(unit)
	int unit;
{
	struct buf *cdp = &fdc_softc.head;
	struct fd_softc *fd = &fd_softc[unit];
	struct buf *dp = &fd->head;
	int s = splbio();
		
	untimeout(fdturnoff, unit);
	if (dp->b_active == 0) {
		dp->b_active = 1;
		fd->track = -1;  /* force seek on first xfer ??? */
	}
	if (!fd->motor) {
		fdturnon(unit);
		timeout(fdustart, unit, hz/2);	/* Wait for 1/2 sec */
	} else {
		dp->b_forw = NULL;
		if (cdp->b_actf == NULL)
			cdp->b_actf = dp;
		else
			cdp->b_actl->b_forw = dp;
		cdp->b_actl = dp;
		if (cdp->b_active == 0)
			fdstart();		/* start controller if idle */
	}
	splx(s);
}

/* XXX should take ctlr number/fdp as parameter */
fdstart()
{
	register struct buf *dp = fdc_softc.head.b_actf;
	register struct buf *bp = dp->b_actf;
	int unit = FDUNIT(bp->b_dev);
	register struct fd_softc *fd = &fd_softc[unit];
	dev_t dev = bp->b_dev;
	int s;

#ifdef FDTEST
	if (fddebug)
		printf("st%d|",unit);
#endif 
	s = splbio();
	fdc_softc.head.b_active = 1;
	fdc_softc.quiet = fd->quiet;
	fd->addr = bp->b_un.b_addr;
	fd->count = bp->b_bcount;
	fd->retry = 0;
	fd->daddr = (long)bp->b_blkno * DEV_BSIZE / FDBLK +
	    fd->dlabel.d_partitions[FDPART(bp->b_dev)].p_offset;
	if (fd->reset) {
		fdtransrate(fd->type->trans);
		if (fdc_softc.unit != unit) {
			fdc_softc.unit = unit;
			set_motor(unit, 0);	/* turn on light */
			fdc_softc.state = Do_specify;
		} else if (bp->b_cylin != fd->track)
			fdc_softc.state = Do_seek;
		else 
			fdc_softc.state = Do_cmd;
	} else {			/* DO a RESET */
		fdc_softc.unit = unit;
		fdc_softc.state = Do_reset;
		fd->reset = 1;
	}
	fdintr(&fdc_softc);	/* XXX */
	splx(s);
}

/* ARGSUSED */
fdtimeout(x)
	int x;
{
	int i, j, s;

	s = splbio();
	if (fdc_softc.quiet == 0) {
		printf("fd%d: timeout: ", fdc_softc.unit);
		if (out_fdc(NE7CMD_SENSED)) {
			out_fdc(fdc_softc.unit);
			i = in_fdc();
			printf("drive status %x, ", i);

			out_fdc(NE7CMD_SENSEI);
			i = in_fdc();
			j = in_fdc();
			printf("ST0 = %x, cyl = %x\n", i, j);
		} else
			printf("\n");
	}

	if (fdc_softc.head.b_actf) {
		at_dma_abort(fdc_softc.dmachan);
		fdbadtrans(&fdc_softc, 1);
	}
	splx(s);
}

/****************************************************************************/
/*                                 fdintr                                   */
/****************************************************************************/
fdintr(sc)
	struct fdc_softc *sc;
{
	struct buf *cdp = &fdc_softc.head;
	register struct buf *dp, *bp;
	struct fd_type *ft;
	register struct fd_softc *fd;
	int unit, i, cyl;
	int ctlr = sc->sc_dev.dv_unit;

#ifdef FDTEST
	if (fddebug)
	printf("intr: state %d, ctlr %d dr %d|", fdc_softc.state, 
	    ctlr, fdc_softc.unit);
#endif

	dp = cdp->b_actf;
	if (dp == NULL) {
		fdunexpectedintr(ctlr);
		return (0);
	}
	bp = dp->b_actf;
	unit = FDUNIT(bp->b_dev);
	/* fdc.unit = FDUNIT(bp->b_dev); */
	if (fddiskchanged(unit)) {
		fdchangedisk(unit);
		fdc_softc.state = Do_reset;
	}
	fd = &fd_softc[unit];
	ft = fd->type;

loop:
	switch (fdc_softc.state) {
	case Seek_done:		/* IF SEEK DONE, START DMA */
		/* Make sure seek really happened*/
		out_fdc(NE7CMD_SENSEI);
		i = in_fdc();
		cyl = in_fdc();
		if (!(i & NE7_ST0_SEEK_COMPLETE) ||
		    cyl != bp->b_cylin * ft->steptrac) {
#ifdef FDDEBUG
			if (fddebug)
			    printf("seek error: ST0 0x%x, cyl %d should be %d\n",
				unit, i, cyl, bp->b_cylin * ft->steptrac);
#endif
			if ((i & 0xE0) != 0xC0)
			    /* not change ready line? */
			    printf("fd%d: seek error: ST0 0x%x, cyl %d should be %d\n",
				unit, i, cyl, bp->b_cylin * ft->steptrac);
			fdc_softc.state = Do_seek;
			goto retry;
		}
		fd->track = bp->b_cylin;
		/*FALLTHROUGH*/

	case Do_cmd:		/* SEEK DONE, START DMA */
	    {
		int read, head, sec;
		int count = FDBLK;

		read = bp->b_flags & B_READ;
		if (count > fd->count)		/* as in fdformat() */
			count = fd->count;
		at_dma(read, fd->addr, count, fdc_softc.dmachan);
		sec = fd->daddr % (ft->sectrac * 2);
		head = sec / ft->sectrac;
		if (bp->b_flags & B_FORMAT) {
			out_fdc(NE7_FORMAT);
			out_fdc(head << 2 | unit);	/* head & unit */
			out_fdc(ft->secsizecode);
			out_fdc(ft->sectrac);		/* sectors/track */
			out_fdc(ft->fgap);		/* format gap size */
			out_fdc(0xDA);		/* filling pattern: why not?
						 * DA - yes in Russian, 
						 * AD, DAD, 
						 * french DADA,
						 * ADA by Nabokov
						 */
		} else {
			sec = sec % ft->sectrac + 1;
			if (read) 
				out_fdc(NE7_READ);
			else
				out_fdc(NE7_WRITE);
			out_fdc(head << 2 | unit);	/* head & unit */
			out_fdc(fd->track);		/* track */
			out_fdc(head);
			out_fdc(sec);			/* sector */
			out_fdc(ft->secsizecode);	/* sector size */
			out_fdc(ft->sectrac);		/* sectors/track */
			out_fdc(ft->gap);		/* gap size */
			out_fdc(ft->datalen);		/* data length */
		}
		fdc_softc.state = Cmd_done;

		/* XXX PARANOIA */
		untimeout(fdtimeout, 0);
		timeout(fdtimeout, 0, FDTIMO * hz);
		break;
	    }

	case Cmd_done:		/* IO DONE, post-analyze */
		untimeout(fdtimeout, 0);
		at_dma_terminate(fdc_softc.dmachan);
		fdgetstatus();
		if (fdc_softc.status[0]&NE7_ST0_NOT_GOOD) {
#ifdef FDOTHER
			if (fddebug)
				fdshowstatus("err status");
#endif
			if (fdc_softc.status[0] & NE7_ST0_NOT_READY) {
				printf("fd%d: drive not ready\n", unit);
				goto err;
			}
			if (fdc_softc.status[1] & NE7_ST1_WRITE_PROTECTED) {
				printf("fd%d: write protected\n", unit);
				goto err;
			}
			goto retry;
		}
		/* All OK */
		fd->count -= FDBLK;
		if (fd->count <= 0) {
#ifdef FDTEST
			if (fddebug)
			printf("DONE %d\n", bp->b_blkno);
#endif
			/* ALL DONE */
			bp->b_resid = 0;
			dp->b_actf = bp->av_forw;
			biodone(bp);
			fdnextop(&fdc_softc, unit);
			break;
		} 
		/* set up next part of transfer */
#ifdef FDTEST
		if (fddebug)
		printf("next|");
#endif
		fd->addr += FDBLK;
		fd->daddr++;
		bp->b_cylin = fd->daddr / (ft->sectrac * 2);
		if (bp->b_cylin == fd->track) {
			fdc_softc.state = Do_cmd;
			goto loop;
		}
		/*FALLTHROUGH*/

	case Do_seek:
#ifdef FDOTHER
		if (fddebug)
			printf("Seek %d %d|", bp->b_cylin, ft->steptrac);
#endif
		out_fdc(NE7CMD_SEEK);
		out_fdc(fdc_softc.unit);
		out_fdc(bp->b_cylin * ft->steptrac);
		fdc_softc.state = Seek_done;
		/* XXX PARANOIA - now twice */
		untimeout(fdtimeout, 0);
		timeout(fdtimeout, 0, FDTIMO * hz);
		break;

	case Do_specify:
		out_fdc(NE7CMD_SPECIFY); 
		out_fdc(0xDF);	/* step rate 0xd; head unload 0xf(*0x10) */
		out_fdc(2);		/* head load - 2msec; DMA */
		out_fdc(NE7CMD_RECAL); 
		out_fdc(unit);

		fdc_softc.state = Do_seek;
		/* XXX PARANOIA */
		untimeout(fdtimeout, 0);
		timeout(fdtimeout, 0, hz);
		break;

	case Do_reset:
#ifdef FDOTHER
		if (fddebug)
		printf("**RESET**\n");
#endif
		/* Try a reset, keep motor on */
		set_motor(fdc_softc.unit, 1);	/* initiate reset */
		DELAY(2000);
		set_motor(fdc_softc.unit, 0);	/* end reset */
		fdtransrate(ft->trans);
		fdc_softc.state = Do_specify;
		/* XXX PARANOIA */
		untimeout(fdtimeout, 0);
		timeout(fdtimeout, 0, hz);
		break;

	default:
		fdunexpectedintr(ctlr);
		return (0);
	}
	return (1);

retry:
	if (fd->retry < 8) {
		if (fd->retry == 4)
			fdc_softc.state = Do_reset;
		else
			fdc_softc.state = Do_seek;
		fd->retry++;
		goto loop;
	}
	if (fdc_softc.state == Cmd_done && fd->quiet == 0)
		fdshowstatus("hard error");
err:
	fdbadtrans(&fdc_softc, 0);
	return (1);
}

fdunexpectedintr(ctlr)
	int ctlr;
{
#if 1
	printf("fdc%d: Unexpected interrupt\n", ctlr);
#else
	int i, sec;
		
	printf("fdc%d: Unexpected interrupt ", ctlr);
	
	/*
	 * This code hangs on some notebooks and other systems,
	 * probably because the controller has been powered down.
	 * It is only diagnostic, skip it for now.
	 */
	if (out_fdc(NE7CMD_SENSEI)) {
		i = in_fdc();
		sec = in_fdc();
		printf("ST0 = %x, cyl = %x\n", i, sec);
		out_fdc(NE7CMD_READID|NE7CMD_MFM); 	/*  read Id Field */
		out_fdc(fdc_softc.unit);
		fdgetstatus();
		fdshowstatus("status");
	} else 
		printf("\n");
#endif
}

fdtransrate(rate)
	int rate;
{

	outb(fdc_softc.base+fdctl, rate);
}

fdgetstatus()
{
	int i;
	short *cp;

	for (i = 0, cp = &fdc_softc.status[0]; i < 7; i++, cp++) {
		*cp = in_fdc();
		if (*cp < 0) break;
	}
}

fdshowstatus(s)
	char *s;
{
	int i;
	short *cp;

	printf("fd %s:", s);
	for (i = 0, cp = &fdc_softc.status[0]; i < 7; i++, cp++) {
		printf(" %x", *cp);
		if (*cp < 0) break;
	}
	printf("\n");
}

fdbadtrans(fdp, from_timeout)
	struct fdc_softc *fdp;
	int from_timeout;
{
	struct buf *dp = fdp->head.b_actf;
	register struct buf *bp = dp->b_actf;
	int unit = FDUNIT(bp->b_dev);
	struct fd_softc *fd = &fd_softc[unit];

	if (from_timeout)
		/* cause a reset before the next operation */
		fdchangedisk(unit);

	if (fd->quiet == 0) {
		diskerr(bp, "fd", 
		    from_timeout ? "no disk in drive" : "hard error",
		    LOG_PRINTF,
		    (fd->daddr - bp->b_blkno) * FDBLK / DEV_BSIZE, &fd->dlabel);
		printf("\n");
	}
	bp->b_flags |= B_ERROR;
	bp->b_error = from_timeout ? ENXIO : EIO;
	bp->b_resid = fd->count;
	dp->b_actf = bp->av_forw;
	biodone(bp);
	fdnextop(fdp, unit);
}

/*
 * fdnextop: After a transfer is done, continue processing
 * requests.  Remove current drive from controller queue,
 * thus servicing drives in round-robin fashion.
 * If the current drive's queue is empty, set timeout
 * to turn off drive in 2 seconds, otherwise prepare
 * for the next operation.  Set the controller state to idle
 * if nothing else to do.
 */
fdnextop(fdp, unit)
	struct fdc_softc *fdp;
	int unit;		/* unit for prev. operation */
{
	struct buf *cdp = &fdp->head;
	struct buf *dp = cdp->b_actf;

	cdp->b_actf = dp->b_forw;
	if (dp->b_actf == NULL) {
		dp->b_active = 0;
		untimeout(fdturnoff, unit);
		timeout(fdturnoff, unit, 2 * hz);
	} else
		fdustart(unit);
	if (cdp->b_actf)
		fdstart();
	else {
		fdp->state = Idle;
		cdp->b_active = 0;
	}
}

int
Fdioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	caddr_t addr;
	int cmd, flag;
	struct proc *p;
{
	int unit = FDUNIT(dev);
	register struct fd_softc *fd = &fd_softc[unit];
	int error = 0;
	struct uio auio;
	struct iovec aiov;
	int fdformat();

	switch (cmd) {

	case DIOCGDINFO:
		*(struct disklabel *)addr = fd->dlabel;
		break;

        case DIOCGPART:
		((struct partinfo *)addr)->disklab = &fd->dlabel;
		((struct partinfo *)addr)->part =
		    &fd->dlabel.d_partitions[FDPART(dev)];
                break;

        case DIOCSDINFO:
                if ((flag & FWRITE) == 0)
                        error = EBADF;
                else
                        error = setdisklabel(&fd->dlabel,
					(struct disklabel *)addr, 0);
                break;

        case DIOCWLABEL:
                if ((flag & FWRITE) == 0)
                        error = EBADF;
                else
                        fd->wlabel = *(int *)addr;
                break;

        case DIOCWDINFO:
                if ((flag & FWRITE) == 0)
                        error = EBADF;
                else if ((error = setdisklabel(&fd->dlabel,
					(struct disklabel *)addr, 0)) == 0) {
                        int wlab;

                        wlab = fd->wlabel;
                        fd->wlabel = 1;
                        error = writedisklabel(dev, Fdstrategy, 
				&fd->dlabel, LABELSECTOR);
                        fd->wlabel = wlab;
                }
                break;
#ifdef notyet
	case DIOCGDINFOP:
		*(struct disklabel **)addr = &(fd->dlabel);
		break;
#endif
	case DIOCWFORMAT:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else {
			register struct format_op *fop;

			fop = (struct format_op *)addr;
			aiov.iov_base = fop->df_buf;
			aiov.iov_len = fop->df_count;
			auio.uio_iov = &aiov;
			auio.uio_iovcnt = 1;
			auio.uio_resid = fop->df_count;
			auio.uio_segflg = 0;
			auio.uio_rw = UIO_WRITE;
			auio.uio_procp = p;
			auio.uio_offset = fop->df_startblk * FDBLK;
			error = physio(fdformat, (struct buf *) NULL, dev,
			    B_WRITE, minphys, &auio);
			fop->df_count -= auio.uio_resid;
#ifdef notdef
			/* ??? what should be in df_reg[] */
			fop->df_reg[0] = error;
			fop->df_reg[1] = error;
#endif
		}
		break;
	default:
		error = ENOTTY;
		break;
	}
	return (error);
}

fdformat(bp)
	struct buf *bp;
{

	/*
	 * We assume that physio will clear B_FORMAT as needed.
	 */
	bp->b_flags |= B_FORMAT;
	return (Fdstrategy(bp));
}

/* ARGSUSED */
Fddump(dev)
	dev_t dev;
{

	return (EIO);
}

Fdsize(dev)
	dev_t dev;
{
	register int unit = FDUNIT(dev);
	register int part;
	register struct fd_softc *fd;

	if (unit >= fdc_softc.ndrives)
		return (-1);
#ifdef notdef
	/* 
	 * problem during swapping initialization - timer not working yet
	 */
	if (Fdopen(dev, 0, 0) > 0)
		return (-1);
#endif
	part = FDPART(dev);
	fd = &fd_softc[unit];
	if (fd->haslabel)
		return ((int)((u_long)fd->dlabel.d_partitions[part].p_size *
			fd->dlabel.d_secsize / DEV_BSIZE));
	return (0);
}

/* ARGSUSED */
fddiskchanged(unit)
	int unit;
{
	/* set_motor... */
	/* ??? inb(fdc_softc.base+fdin) & FDI_DCHG; */
	/* ??? but how check unit ??? */
	return (0);
}

fdchangedisk(unit)
	int unit;
{
	register struct fd_softc *fd = &fd_softc[unit];
	int s;
	
	s = splbio();
	fd->haslabel = 0;
	fd->wlabel = 0;
	fd->motor = 0;
	fd->track = -1;
	fd->settype = 0;
	fd->reset = 0;
#ifdef notdef	/* This is wrong, and may crash on next interrupt... */
	register struct buf *dp, *bp;
	/* get out of queue all blocks for this unit */
	for (dp = &fd->head, i = 0; (bp = dp->av_forw) != 0; i++) {
		if (FDUNIT(bp->b_dev) == unit) {
			bp->b_error = EIO;
			bp->b_resid = i ? bp->b_bcount : fd->count;
			dp->av_forw = bp->av_forw;
			biodone(bp);
		} else
			dp = bp->av_forw;
	}
#endif
	splx(s);
}

fdauto(dev, fd)
	dev_t dev;
	register struct fd_softc *fd;
{
	int *p;
	register struct buf *bp = geteblk(FDBLK);
	struct fd_drivetype *fdt = &fd_drivetypes[fd->drtype];
	int unit = FDUNIT(dev);
	int nodisk = 0;

	/*
	 * Attempt to read the last sector for each type in the
	 * list for this drive type; the first success gives us
	 * the floppy disk type.
	 */
	fd->quiet = 1;
	fd->settype = 1;
	bp->b_dev = makedev(major(dev), fdminor(unit, 0));
	fd->dlabel.d_partitions[0].p_offset = 0;
	fd->dlabel.d_partitions[0].p_size = LONG_MAX;
	for (p = fdt->disktypes; *p != -1; p++) {
		fd->type = &fd_types[*p];
		bp->b_bcount = FDBLK;
		bp->b_blkno = fd_types[*p].size - 1;	/* last block */
		bp->b_flags = B_READ;
		Fdstrategy(bp);
		biowait(bp);
		if ((bp->b_flags & B_ERROR) == 0)
			break;
		if (bp->b_error == ENXIO) {
			nodisk = 1;
			break;
		}
	}

	/*
	 * If nothing worked, default to the first format in the list.
	 */
	if (bp->b_flags & B_ERROR) {
		fd->type = &fd_types[fdt->disktypes[0]];
#ifdef FDTEST
		if (fddebug)
			printf("fdauto err, dflt %d\n", fd->type->size);
#endif
	}

	fd->quiet = 0;
	bp->b_flags = B_INVAL | B_AGE;
	brelse(bp);
	return (nodisk);
}
#endif
