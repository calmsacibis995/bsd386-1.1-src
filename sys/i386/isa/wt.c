/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: wt.c,v 1.16 1993/11/06 02:21:02 karels Exp $
 */

/*
 * Wangtek QIC-02/QIC-36 / Archive Viper QIC-02 streamer tape driver
 */

/* #define WTDEBUG */

#include "wt.h"
#include "param.h"
#include "buf.h"
#include "file.h"
#include "proc.h"
#include "user.h"
#include "tprintf.h"
#include "ioctl.h"
#include "mtio.h"
#include "device.h"

#include "icu.h"
#include "isavar.h"
#include "dma.h"
#include "wtreg.h"

#ifdef WTDEBUG
static	int wtdebug = 1;
#define dprintf(x)	if (wtdebug) printf x
#else
#define dprintf(x)
#endif

/*
 * Minor number decoding
 */
#define	wtunit(dev)	(minor(dev) & 03)

/* Density selection; higher-to-lower, unlike 9 track */
#define  T_QIC150	T_800BPI	/* 150 Mb */
#define  T_QIC120	T_1600BPI	/* 120 Mb */
#define  T_QIC24	T_6250BPI	/* 60 Mb */
#define  T_QIC11	T_BADBPI	/* An ancient format-- not supported */

/* "Tape continuation is allowed" bit in device minor */
#define T_DOCONT	040

/*
 * Buffer queue parameters
 */
#define WTHIWAT		32	/* 128 KB -- max Q length */
#define WTLOWAT		16	/* 64 KB -- tape needs more blocks on write */
#define WTBUFSIZE	4096	/* must be >= page size */

/* Number of wait loops */
#define WAIT_LOOPS	100000

#define WTPRI	(PZERO+10)

/*
 * Drive state information
 */
struct wtsoftc {
	struct	device wts_dev;	/* base device */
	struct	isadev wts_id;	/* ISA device */
	struct 	intrhand wts_ih; /* interrupt vectoring */
	short	wts_port;	/* Controller base port # */
	short	wts_statport;	/* Controller status port */
	char	wts_statmask;	/* Mask for READY & EXCEP bits */
	char	wts_ready;	/* READY bit */
	char	wts_excep;	/* EXCEP bit */
	char	wts_chan;	/* DMA channel */
	short	wts_repeat;	/* Repeat counter for seek ops */
	short	wts_error;	/* Error flags from controller */
	short	wts_qlen;	/* Queue length in 512-blocks */
	short	wts_vol;	/* Tape volume # */
	int	wts_flags;	/* Current drive status */
	int	wts_nwblks;	/* The number of written blocks */
	short	wts_file;	/* The number of filemarks from BOT */
	short	wts_cden;	/* The current selected density */
	struct	buf *wts_bip;	/* The current i/o buffer */
	struct	buf *wts_qh;	/* Head of i/o queue */
	struct	buf *wts_qt;	/* Tail of i/o queue */
	struct	buf *wts_avail;	/* Available buffers list */
	struct	buf *wts_wbuf;	/* The write collection buffer */
	tpr_t	wts_tpr;	/* tprintf handle */
};

/*
 * Drive status flags
 */
#define WTS_IDLE	0x00000	/* Tape is idle */
#define WTS_READ	0x00001	/* Reading is in progress */
#define WTS_WRITE	0x00002	/* Writing is in progress */
#define WTS_REWIND	0x00003	/* Tape is rewinding */
#define WTS_FSF		0x00004	/* Forward skip file */
#define WTS_WTM		0x00005	/* Writing tape mark */
#define WTS_DELAY	0x00006	/* Device delays... */
#define WTS_DSEL	0x00007	/* Density select op */
#define WTS_ACTIVE	0x00007	/* Current OP mask */

#define	WTS_OREAD	0x00008	/* Device is opened for reading */
#define WTS_OWRITE	0x00010	/* Device is opened for writing */
#define WTS_OPENED	(WTS_OREAD|WTS_OWRITE)

#define WTS_NEEDRESET	0x00020	/* Cartridge probably was changed */
#define WTS_DEAD	0x00040	/* Device died */
#define WTS_RSHUT	0x00080	/* Read-ahead shutdown */
#define WTS_RSHUTREW	0x00100	/* Rewind on close wanted */
#define WTS_EOF		0x00200	/* Tape mark found during read */
#define WTS_FSFPEND	0x00400	/* Seek till EOF's pending */
#define WTS_RONLY	0x00800	/* Tape is read-only */
#define WTS_LASTWRITE	0x01000	/* Last op was write */
#define WTS_ERR		0x02000	/* Tape is BAD / hit EOM */
#define WTS_TCHANGE	0x04000	/* Tape needs to be changed */
#define WTS_RCONT	0x08000	/* Reading is to be continued on new tape */
#define WTS_WCONT	0x10000	/* Writing is to be continued on new tape */
#define WTS_TIMO	0x20000	/* Timeout second-pass flag */
#define WTS_DOCONT	0x40000	/* Continue on the next tape if EOT hit */
#define	WTS_RDOVERRUN	0x80000	/* Read overrun */
#define WTS_LASTREAD	0x100000 /* Last op was read */
#define WTS_INITED	0x200000 /* Drive initalized -- ignore interrupt */
#define WTS_ARC		0x400000 /* This is an Archive drive */

/*
 * Tape controllers ports
 */
#define	wt_stat(sp)	(wt_statport((sp)->wts_port))	/* read, status */
#define wt_ctl(sp)	(wt_ctlport((sp)->wts_port))	/* write, control */
#define wt_data(sp) 	(wt_dataport((sp)->wts_port))	/* read, data */
#define wt_cmd(sp)	(wt_cmdport((sp)->wts_port))	/* write, command */

#define	arc_stat(sp)	(arc_statport((sp)->wts_port))	/* read, status */
#define arc_ctl(sp)	(arc_ctlport((sp)->wts_port))	/* write, control */
#define arc_data(sp) 	(arc_dataport((sp)->wts_port))	/* read, data */
#define arc_cmd(sp)	(arc_cmdport((sp)->wts_port))	/* write, command */
#define arc_sdma(sp)	(arc_sdmaport((sp)->wts_port))	/* write, start dma */
#define arc_rdma(sp)	(arc_rdmaport((sp)->wts_port))	/* write, reset dma */

int	wttimer_active;
extern	int hz;
int	wtpoll();
void	wtattach __P((struct device *, struct device *, void *));
int	wtintr __P((struct wtsoftc *));
int	wtprobe __P((struct device *, struct cfdata *, void *));

static	char wtio[] = "wtio";
static	char wtcntl[] = "wtcntl";
static	char wtrew[] = "wtrew";
static	char wtmotion[] = "wtmotion";

struct cfdriver wtcd = 
	{ NULL, "wt", wtprobe, wtattach, sizeof(struct wtsoftc) };

/*
 * Probe & attach routines
 */
/* ARGSUSED */
wtprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	void wtforceintr();
	register base = ia->ia_iobase;

	dprintf(("wtprobe %x\n", ia->ia_iobase));
	if (ia->ia_drq < 1 || ia->ia_drq > 3) {
		printf("wt%d: bad DRQ number %d (hardware supports only 1-3)\n",
		    cf->cf_unit, ia->ia_drq);
		return (0);
	}

	/* Try Wangtek reset action first */
	outb(wt_ctlport(base), WT_RESET);
	DELAY(2000);	/* 20 usec */
	outb(wt_ctlport(base), 0);
	if ((inb(wt_statport(base)) & WT_RESETMASK) == WT_RESETVAL ||
	    (inb(wt_statport(base)) & WT_RESETMASK) == WT_RESETVAL) {
		outb(wt_ctlport(base), WT_IENB(ia->ia_drq));
		ia->ia_aux = (void *) 0;
		goto resetok;
	}
	dprintf(("wt reset failed\n"));

	/* Try Archive now */
	outb(arc_ctlport(base), ARC_RESET);
	DELAY(2500);	/* 25 usec */
	outb(arc_ctlport(base), 0);
	if ((inb(arc_statport(base)) & ARC_RESETMASK) == ARC_RESETVAL) {
		outb(arc_rdmaport(base), 0);
		outb(arc_ctlport(base), ARC_IEN);
		ia->ia_aux = (void *) 1;
		goto resetok;
	}
	dprintf(("arc reset failed\n"));
	return (0);

resetok:
	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(wtforceintr, aux);
		if (ffs(ia->ia_irq) - 1 == 0) {
			dprintf(("no interrupt\n"));
			if (cf->cf_unit == 0) {
				printf("wt0: assuming default irq\n");
				ia->ia_irq = IRQ5;
			} else
				return (0);
		}
	}
	ia->ia_iosize = WT_NPORT;
	return (1);
}

void
wtforceintr(aux)
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int cnt = WAIT_LOOPS;

	if ((int)(ia->ia_aux) == 0) {
		dprintf(("forceintr wt\n"));
		outb(wt_cmdport(ia->ia_iobase), WT_QIC150);
		outb(wt_ctlport(ia->ia_iobase), WT_REQUEST|WT_ONLINE);
		while ((inb(wt_statport(ia->ia_iobase)) & WT_READY) && --cnt)
			;
		outb(wt_ctlport(ia->ia_iobase), WT_IENB(ia->ia_drq));
	} else {
		dprintf(("forceintr arc\n"));
		outb(arc_cmdport(ia->ia_iobase), WT_QIC150);
		outb(arc_ctlport(ia->ia_iobase), ARC_REQUEST);
		while ((inb(arc_statport(ia->ia_iobase)) & ARC_READY) && --cnt)
			;
		outb(arc_ctlport(ia->ia_iobase), ARC_IEN);
	}
	DELAY(2000);	/* 20 usec */
}

/* ARGSUSED */
void
wtattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct wtsoftc *sp = (struct wtsoftc *) self;
        register struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int s;

	sp->wts_port = ia->ia_iobase;
	sp->wts_chan = ia->ia_drq;
	sp->wts_cden = -1;	/* always switch */
	if ((int)(ia->ia_aux) == 0) {	/* Wangtek stuff */
		sp->wts_flags = WTS_NEEDRESET|WTS_INITED;
		sp->wts_statport = wt_stat(sp);
		sp->wts_excep = WT_EXCEP;
		sp->wts_ready = WT_READY;
		sp->wts_statmask = WT_STATMASK;
	} else {			/* Archive stuff */
		sp->wts_flags = WTS_NEEDRESET|WTS_INITED|WTS_ARC;
		sp->wts_statport = arc_stat(sp);
		sp->wts_excep = ARC_EXCEP;
		sp->wts_ready = ARC_READY;
		sp->wts_statmask = ARC_STATMASK;
	}

	/* Clean up controller state */
	(void) wtstatus(sp);
	dprintf(("\nwtstatus1: %x\n", sp->wts_error));
	s = splhigh();
	splx(0);        /* enable all interrupts (should be only ia->ia_irq?) */
	DELAY(100000);  /* delay 0.1 sec */
	splx(s);

	printf(": %s\n", (sp->wts_flags & WTS_ARC) ?
	    "Archive 2150L" : "Wangtek 5150");

	isa_establish(&sp->wts_id, &sp->wts_dev);
	sp->wts_ih.ih_fun = wtintr;
	sp->wts_ih.ih_arg = sp;
	intr_establish(ia->ia_irq, &sp->wts_ih, DV_TAPE);
	at_setup_dmachan(sp->wts_chan, WTBLKSIZE);
}

/*
 * Device open routine
 */
wtopen(dev, flag, type, proc)
	dev_t dev;
	struct proc *proc;
{
	register struct wtsoftc *sp;
	struct buf *bp;
	int i, error;

	if (wtunit(dev) >= wtcd.cd_ndevs ||
	    (sp = wtcd.cd_devs[wtunit(dev)]) == NULL)
		return (ENXIO);

	/*
	 * Tape allows exclusive access only
	 */
	if (sp->wts_flags & WTS_DEAD)
		return (EIO);
	if (sp->wts_flags & WTS_OPENED)
		return (EBUSY);
	dprintf(("wtopen: wait for i/o completion (state %d)\n"));
	i = splbio();
	while (sp->wts_flags & WTS_ACTIVE) {
		if (error = tsleep((caddr_t) sp, PCATCH|WTPRI, wtcntl, 0)) {
			splx(i);
			return (error);
		}
	}
	splx(i);

	/*
	 * Get status of the tape
	 */
	if (wttimer_active == 0) {
		wttimer_active++;
		timeout(wtpoll, 0, hz * 2);
	}
	sp->wts_tpr = tprintf_open(proc);
	if (sp->wts_flags & WTS_FSFPEND) {
		if (sp->wts_flags & WTS_RONLY)
			sp->wts_error = WTE_WRP;
	} else if (wtstatus(sp)) {
#ifdef notdef
		printf("wt%d: cannot initialize controller - device dead\n",
		    sp->wts_dev.dv_unit);
		sp->wts_flags |= WTS_DEAD;
		error = EIO;
		goto bad;
#endif
	}
	dprintf(("wtopen: status=%x\n", sp->wts_error));
	if (sp->wts_error & (WTE_USL|WTE_CNI)) {
		wterror(sp);
		error = EIO;
		goto bad;
	}
	if (sp->wts_error & WTE_POR)
		sp->wts_flags |= WTS_NEEDRESET;
	sp->wts_flags &= ~(WTS_RONLY|WTS_DOCONT);
	if (sp->wts_error & WTE_WRP)
		sp->wts_flags |= WTS_RONLY;
	if (minor(dev) & T_DOCONT)
		sp->wts_flags |= WTS_DOCONT;

	/*
	 * Mark device as opened and rewind the tape if necessary
	 */
	if (flag & FWRITE) {
		if (sp->wts_error & WTE_WRP) {
			tprintf(sp->wts_tpr, "wt%d: write protected\n",
			    sp->wts_dev.dv_unit);
			error = ENXIO;
			goto bad;
		}
		sp->wts_flags |= WTS_OWRITE;
	}
	if (flag & FREAD)
		sp->wts_flags |= WTS_OREAD;
	if (sp->wts_flags & WTS_NEEDRESET) {
		wtrewind(sp);
		i = splbio();
		while ((sp->wts_flags & WTS_ACTIVE) == WTS_REWIND) {
			if (error = tsleep((caddr_t) sp, PCATCH|WTPRI,
			    wtrew, 0)) {
				splx(i);
				goto bad;
			}
		}
		splx(i);
	}

	/*
	 * Select the tape density
	 */
	if (sp->wts_cden != (dev & T_DENSEL)) {
		if (sp->wts_file != 0) {
			tprintf(sp->wts_tpr,
			    "wt%d: can't change density in mid-tape\n",
			    sp->wts_dev.dv_unit);
			error = ENXIO;
			goto bad;
		}
		if (error = wtdensity(sp, dev))
			goto bad;
	}

	/*
	 * Allocate WTHIWAT buffers.
	 * We assume that buffers are physically contiguous
	 * and don't cross 64K (physical) boundary.
	 * It makes the life easier in future (oh, 8237 ideosyncracy!)
	 * Fortunately, 4K buffers should be page-aligned.
	 */
	dprintf(("wtopen: alloc bufs... "));
	sp->wts_avail = NULL;
	for (i = 0; i < WTHIWAT; i++) {
		bp = geteblk(WTBUFSIZE);
		if ((u_long) bp->b_un.b_addr & PGOFSET)
			panic("wt buf not aligned");
		bp->b_next = sp->wts_avail;
		sp->wts_avail = bp;
	}
	sp->wts_qh = sp->wts_qt = NULL;
	sp->wts_qlen = 0;
	sp->wts_vol = 0;
	sp->wts_bip = NULL;

	dprintf((" done\n"));
	return (0);

bad:
	sp->wts_flags &= ~WTS_OPENED;
	tprintf_close(sp->wts_tpr);
	sp->wts_tpr = NULL;
	return (error);
}

/*
 * Close the tape device
 */
wtclose(dev, flag)
	dev_t dev;
{
	register struct wtsoftc *sp = wtcd.cd_devs[wtunit(dev)];
	struct buf *bp;
	int i, err;

	dprintf(("wtclose\n"));

	/*
	 * Flush buffers
	 */
	wtflush(sp, !(minor(dev) & T_NOREWIND));

	/*
	 * Rewind tape on closing
	 */
	if (!(minor(dev) & T_NOREWIND))
		wtrewind(sp);

	/*
	 * Release buffers
	 */
	i = 0;
	while ((bp = sp->wts_avail) != NULL) {
		i++;
		sp->wts_avail = bp->b_next;
		bp->b_flags = B_INVAL;
		brelse(bp);
	}
	dprintf(("CLOSE: freed %d buffers\n", i));
	err = (sp->wts_flags & WTS_ERR) ? EIO : 0;
	sp->wts_flags &= ~(WTS_OPENED|WTS_ERR|WTS_TCHANGE|
		WTS_RCONT|WTS_WCONT|WTS_RDOVERRUN);
	tprintf_close(sp->wts_tpr);
	sp->wts_tpr = NULL;
	return (err);
}

/*
 * Flush buffers and wait for i/o completion
 */
wtflush(sp, wantrewind)
	register struct wtsoftc *sp;
{
	register struct buf *bp;
	int s, l;
	int err;

	dprintf(("wtflush: "));
	s = splbio();
	sp->wts_flags |= WTS_RSHUT;
	if (wantrewind)
		sp->wts_flags |= WTS_RSHUTREW;
	while ((sp->wts_flags & WTS_ACTIVE) == WTS_READ ||
	    (sp->wts_flags & WTS_ACTIVE) == WTS_FSF) {
		dprintf((" [waiting for read-ahead shut]"));
		(void) tsleep((caddr_t) sp, WTPRI, wtio, 0);
	}
	sp->wts_flags &= ~(WTS_RSHUT|WTS_RSHUTREW);

	if ((bp = sp->wts_wbuf) != NULL) {
		sp->wts_wbuf = NULL;
		bp->b_next = NULL;
		bp->b_dirtyoff = 0;
		l = bp->b_bcount % WTBLKSIZE;
		if (l > 0) {
			l = WTBLKSIZE - l;
			dprintf((" [zero trail]"));
			bzero(bp->b_un.b_addr + bp->b_bcount, l);
			bp->b_bcount += l;
		}
		if (sp->wts_qlen++ == 0)
			sp->wts_qh = bp;
		else
			sp->wts_qt->b_next = bp;
		sp->wts_qt = bp;
	}
	
	if ((bp = sp->wts_qh) != NULL) {
		if ((bp->b_flags & B_READ) || (sp->wts_flags & WTS_WCONT)) {
			dprintf((" [tossing read/unwritten blks]"));
	flushb:
			do {
				sp->wts_qh = bp->b_next;
				sp->wts_qlen--;
				bp->b_next = sp->wts_avail;
				sp->wts_avail = bp;
				bp = sp->wts_qh;
			} while (bp != NULL);
			if ((bp = sp->wts_bip) != NULL) {
				bp->b_next = sp->wts_avail;
				sp->wts_avail = bp;
				sp->wts_bip = NULL;
			}
		} else {
			dprintf((" [waiting for write queue drain]"));
			wtwstart(sp);
			while ((sp->wts_flags & WTS_ACTIVE) == WTS_WRITE)
				(void) tsleep((caddr_t) sp, WTPRI, wtio, 0);
			if (sp->wts_flags & WTS_ERR) {
				dprintf((" [flushing blocks on write error]"));
				goto flushb;
			}
		}
	}
	if ((sp->wts_flags & (WTS_LASTWRITE | WTS_WCONT)) == WTS_LASTWRITE)
		(void) wtwfm(sp);
	sp->wts_flags &= ~WTS_LASTWRITE;
	splx(s);
	dprintf((" done\n"));
	return (0);
}

/* Wait for ready or exception */
#define WAIT_RDYE(s, sp, cnt) \
	for ((cnt) = WAIT_LOOPS; --(cnt); ) { \
		if ((((s) = inb((sp)->wts_statport)) & (sp)->wts_statmask) != (sp)->wts_statmask) \
			break; \
	}

/* Wait for ready */
#define WAIT_RDY(sp, cnt) \
	for ((cnt) = WAIT_LOOPS; (inb((sp)->wts_statport) & (sp)->wts_ready) && --(cnt);)

/* Wait for not ready */
#define WAIT_NRDY(sp, cnt) \
	for ((cnt) = WAIT_LOOPS; !(inb((sp)->wts_statport) & (sp)->wts_ready) && --(cnt);)

/*
 * Issue a command
 */
wtcmd(sp, cmd)
	register struct wtsoftc *sp;
{
	int s, cnt;

	sp->wts_flags &= ~(WTS_LASTREAD|WTS_LASTWRITE);
	WAIT_RDYE(s, sp, cnt);
	if (!(s & sp->wts_excep)) {
		wtstatus(sp);	/* lower exception flag */
		dprintf(("wtcmd got exception: %x\n", sp->wts_error));
	}
	if (sp->wts_flags & WTS_ARC) {
		outb(arc_cmd(sp), cmd);
		outb(arc_ctl(sp), ARC_REQUEST);
		WAIT_RDY(sp, cnt);
		outb(arc_ctl(sp), ARC_IEN);
	} else {
		outb(wt_cmd(sp), cmd);
		outb(wt_ctl(sp), WT_REQUEST|WT_ONLINE);
		WAIT_RDY(sp, cnt);
		outb(wt_ctl(sp), WT_IENB(sp->wts_chan)|WT_ONLINE);
	}
	WAIT_NRDY(sp, cnt);	/* we have a lot of time to catch it... */
}

/*
 * Read controller status
 */
wtstatus(sp)
	register struct wtsoftc *sp;
{
	int cnt, s;
	char buf[6], *p;
	int nretry = 0;

	/* Wait for controller ready */
	sp->wts_error = WTE_DEAD;

	/* Issue read status command */
retry:
	if (sp->wts_flags & WTS_ARC) {
		outb(arc_cmd(sp), WT_RDSTAT);
		outb(arc_ctl(sp), ARC_REQUEST);
		WAIT_RDY(sp, cnt);
		outb(arc_ctl(sp), 0);
	} else {
		outb(wt_cmd(sp), WT_RDSTAT);
		outb(wt_ctl(sp), WT_REQUEST|WT_ONLINE);
		WAIT_RDY(sp, cnt);
		outb(wt_ctl(sp), WT_ONLINE);
	}
	WAIT_NRDY(sp, cnt);

	p = buf;
	while (p < &buf[6]) {
		/* Wait for controller ready */
		WAIT_RDYE(s, sp, cnt);
		if (cnt == 0) {
			dprintf(("wtstatus: no status byte? --retry (byte %d)\n",
			    p - buf));
			if (++nretry < 7)
				goto retry;
			return (1);
		}
		if (!(s & (sp)->wts_excep)) {
			dprintf(("wtstatus: exception (s=%x) --retry (byte %d)\n",
			    s, p - buf));
			if (++nretry < 7)
				goto retry;
			return (1);
		}

		if (sp->wts_flags & WTS_ARC) 
			*p++ = inb(arc_data(sp));
		else
			*p++ = inb(wt_data(sp));

		/* Twiddle request bit */
		if (sp->wts_flags & WTS_ARC) {
			outb(arc_cmd(sp), WT_RDSTAT);
			outb(arc_ctl(sp), ARC_REQUEST);
			WAIT_NRDY(sp, cnt);
			/* DELAY(50);	 */
			outb(arc_ctl(sp), 0);
		} else {
			outb(wt_cmd(sp), WT_RDSTAT);
			outb(wt_ctl(sp), WT_REQUEST|WT_ONLINE);
			WAIT_NRDY(sp, cnt);
			/* DELAY(50);	 */
			outb(wt_ctl(sp), WT_ONLINE);
		}
	}
	WAIT_RDY(sp, cnt);
	sp->wts_error = 0;
	if (buf[0] & WT_ERRFLAG)
		sp->wts_error = buf[0] & WT_ERRMASK;
	if (buf[1] & WT_ERRFLAG)
		sp->wts_error |= (buf[1] & WT_ERRMASK) << 8;
	if ((sp->wts_error & (WTE_CNI|WTE_ILL)) == WTE_CNI) {
		sp->wts_flags |= WTS_NEEDRESET;
		sp->wts_flags &= ~WTS_FSFPEND;
	}
	/* This will not cause an extra interrupt */
	if (sp->wts_flags & WTS_ARC)
		outb(arc_ctl(sp), ARC_IEN);
	else
		outb(wt_ctl(sp), WT_ONLINE | WT_IENB(sp->wts_chan));
	return (0);
}

/*
 * Report an error
 */
wterror(sp)
	register struct wtsoftc *sp;
{
	char *p;

	if (sp->wts_error & WTE_DEAD)
		p = "read status failed";
	else if (sp->wts_error & WTE_USL)
		p = "drive not online";
	else if (sp->wts_error & WTE_ERM)
		p = "end of recorded media";
	else if (sp->wts_error & WTE_UDA)
		p = "unrecoverable data error";
	else if (sp->wts_error & WTE_NDT)
		p = "no data detected";
	else if (sp->wts_error & WTE_ILL)
		p = "illegal command";
	else if (sp->wts_error & WTE_BNL)
		p = "block not located";
	else if (sp->wts_error & WTE_CNI)
		p = "no tape inserted";
	else
		return;
	tprintf(sp->wts_tpr, "wt%d: %s\n", sp->wts_dev.dv_unit, p);
}

/*
 * Rewind the tape
 */
wtrewind(sp)
	struct wtsoftc *sp;
{
	int cnt;

	dprintf(("wtrewind --"));
	if ((sp->wts_flags & WTS_ACTIVE) == WTS_REWIND)
		return;
	/*
	 * Archive controller cannot drive ONLINE, but Archive
	 * drive allows using REWIND when READ/WRITE is in progress
	 */
	if (sp->wts_flags & WTS_ARC) {
		dprintf((" rewind cmd\n"));
		wtcmd(sp, WT_REWIND);
	} else if (sp->wts_flags & WTS_FSFPEND) {
		dprintf((" drop online\n"));
		/* lower WT_ONLINE */
		outb(wt_ctl(sp), WT_IENB(sp->wts_chan));
	} else {
		dprintf((" rewind cmd\n"));
		wtcmd(sp, WT_REWIND);
	}
	sp->wts_flags &= ~(WTS_ACTIVE|WTS_FSFPEND|WTS_ERR);
	sp->wts_flags |= WTS_REWIND;
}

/* Fake routines */
wtstrategy(bp)
	struct buf *bp;
{
	printf("wt%d: block i/o is not supported\n", wtunit(bp->b_dev));
	bp->b_error = ENXIO;
	biodone(bp);
}

/*
 * Start the tape reading
 */
wtrstart(sp)
	register struct wtsoftc *sp;
{

	if (sp->wts_flags & (WTS_ACTIVE|WTS_EOF|WTS_RSHUT|WTS_TCHANGE))
		return;
	if (sp->wts_avail == NULL)
		return;

 	if (sp->wts_flags & WTS_RDOVERRUN) {
 		dprintf(("delayed interrupt\n"));
 		sp->wts_flags &= ~WTS_RDOVERRUN;
 		sp->wts_flags |= WTS_READ;
 		/* simulate delayed interrupt */
 		wtXintr(sp);
 		return;
 	}

	dprintf(("start read"));
	wtcmd(sp, WT_RDDATA);
	sp->wts_flags |= WTS_READ|WTS_LASTREAD;
}

/*
 * Start the tape writing
 */
wtwstart(sp)
	register struct wtsoftc *sp;
{
	register struct buf *bp;

	if (sp->wts_flags & (WTS_WRITE|WTS_ERR|WTS_TCHANGE))
		return;

	dprintf(("start write"));
	if (sp->wts_flags & WTS_LASTWRITE) {	/* pending write... */
		bp = sp->wts_qh;
		sp->wts_qh = bp->b_next;
		if (--(sp->wts_qlen) == 0)
			sp->wts_qt = NULL;
		sp->wts_bip = bp;
		dprintf(("Q=%d ", sp->wts_qlen));
		sp->wts_flags |= WTS_WRITE;
		wtdma(sp, bp);
	} else {
		wtcmd(sp, WT_WRDATA);
		sp->wts_flags &= ~WTS_ACTIVE;
		sp->wts_flags |= WTS_WRITE|WTS_LASTWRITE;
	}
}

/*
 * Initiate DMA operation
 */
wtdma(sp, bp)
	register struct wtsoftc *sp;
	register struct buf *bp;
{
	caddr_t addr;
	
	addr = bp->b_un.b_addr +
	    ((bp->b_flags & B_READ) ? bp->b_bcount : bp->b_dirtyoff);

	if (sp->wts_flags & WTS_ARC)
		outb(arc_sdma(sp), 0);
	at_dma(bp->b_flags & B_READ, addr, WTBLKSIZE, sp->wts_chan);
	dprintf(("wtdma: addr=%x bcnt=%d chan=%d ",
	    addr, bp->b_bcount, sp->wts_chan));
}

wtintr(sp)
	register struct wtsoftc *sp;
{
	int s;

	dprintf(("wtintr "));
	s = inb(sp->wts_statport);
	if (!(s & sp->wts_excep)) {
		if ((sp->wts_flags & WTS_ACTIVE) == WTS_IDLE) {
			if (sp->wts_flags & WTS_INITED) {
				dprintf(("initintr - EXCEP\n"));
				sp->wts_flags &= ~WTS_INITED;
			}
			if (!(sp->wts_flags & WTS_NEEDRESET))
				wtstatus(sp);
			return (1);
		}
		wtstatus(sp);
	} else if (!(s & sp->wts_ready)) {
		if ((sp->wts_flags & WTS_ACTIVE) == WTS_IDLE) {
			if (sp->wts_flags & WTS_INITED) {
				dprintf(("initintr\n"));
				sp->wts_flags &= ~WTS_INITED;
			} else
				printf("wt%d: extra interrupt\n",
				    sp->wts_dev.dv_unit);
			return (1);
		}
		sp->wts_error = 0;
	} else {
		printf("wt%d: bogus interrupt status=%x\n",
		    sp->wts_dev.dv_unit, s);
		return (0);
	}
	wtXintr(sp);
	return (1);
}

wtXintr(sp)
	register struct wtsoftc *sp;
{
	register struct buf *bp, *bp1;
	int s, cnt;

	s = sp->wts_flags & WTS_ACTIVE;
	sp->wts_flags &= ~(WTS_ACTIVE|WTS_TIMO);
	switch (s) {
	case WTS_DSEL:
		dprintf(("DSEL err=%x\n", sp->wts_error));
		break;	/* simply wakeup */

	case WTS_DELAY:
		dprintf(("INTERRUPT DURING DELAY err=%x\n"));
		return;

	case WTS_FSF:
		dprintf(("FSF err=%x\n", sp->wts_error));
		sp->wts_file++;
		if (!(sp->wts_error & WTE_HARD) && --(sp->wts_repeat) > 0) {
			wtcmd(sp, WT_RDMARK);
			sp->wts_flags |= WTS_FSF;
			return;
		}
		break;

	case WTS_READ:
		dprintf(("READ err=%x\n", sp->wts_error));
		/*
		 * Get a buffer to put result to
		 */
		sp->wts_nwblks++;
		bp = sp->wts_bip;
		if (bp == NULL) {
			bp = sp->wts_avail;
			sp->wts_avail = bp->b_next;
			bp->b_bcount = 0;
			bp->b_flags = B_READ;
		} else {
			at_dma_terminate(sp->wts_chan);
			bp->b_bcount += WTBLKSIZE;	/* prev DMA ok */
		}

		/*
		 * Look for error conditions
		 */
		if (sp->wts_error & WTE_EOM && sp->wts_flags & WTS_DOCONT) {
			/* this block will be repeated on the next tape... */
			bp->b_bcount -= WTBLKSIZE;
			tprintf(sp->wts_tpr,
			    "wt%d: insert tape #%d to continue reading...\n",
			    sp->wts_dev.dv_unit, sp->wts_vol + 2);
			sp->wts_flags &= ~WTS_NEEDRESET;
			sp->wts_flags |= WTS_TCHANGE|WTS_RCONT|WTS_REWIND;
			if (sp->wts_flags & WTS_ARC)
				/* Archive: issue BOT */
				wtcmd(sp, WT_REWIND);
			else
				/* Wangtek: drop ONLINE to terminate reading */
				outb(wt_ctl(sp), WT_IENB(sp->wts_chan));
			if (bp->b_bcount > bp->b_bufsize - WTBLKSIZE) {
				sp->wts_bip = NULL;
				bp->b_next = NULL;
				bp->b_dirtyoff = 0;
				if (sp->wts_qlen++ == 0)
					sp->wts_qh = bp;
				else
					sp->wts_qt->b_next = bp;
				sp->wts_qt = bp;
			}
			/* do not wake up */
			return;
		} else if (sp->wts_error & (WTE_HARD|WTE_EOM)) {
			if (sp->wts_error & WTE_EOM) {
				tprintf(sp->wts_tpr,
				    "wt%d: hit end of medium\n",
				    sp->wts_dev.dv_unit);
				sp->wts_flags |= WTS_ERR;
		    	} else
				dprintf(("read err=%x\n", sp->wts_error));
			bp->b_flags |= B_ERROR;
		} else if (sp->wts_error & WTE_FIL) {
			sp->wts_file++;
			sp->wts_flags |= WTS_EOF;
			sp->wts_flags &= ~WTS_FSFPEND;
		} else if (sp->wts_flags & WTS_RSHUT) {
			dprintf(("read shutdown\n"));
			sp->wts_flags |= WTS_FSFPEND;
			if (sp->wts_flags & WTS_RSHUTREW)
				wtrewind(sp);
			sp->wts_error = 0;
		} else {
			/*
			 * Tape is moving and we're willing to continue
			 */
			if (bp->b_bcount <= bp->b_bufsize - WTBLKSIZE) {
				sp->wts_bip = bp;
				wtdma(sp, bp);
				sp->wts_flags |= WTS_READ;
				return;
			}
			if (sp->wts_avail != NULL) {
				bp1 = sp->wts_avail;
				sp->wts_avail = bp1->b_next;
				bp1->b_flags = B_READ;
				bp1->b_bcount = 0;
				sp->wts_bip = bp1;
				wtdma(sp, bp1);
				sp->wts_flags |= WTS_READ;
				goto qbuf;
			}
			dprintf(("no buffers, qlen=%d\n", sp->wts_qlen));
			sp->wts_flags |= WTS_RDOVERRUN;
		}
		sp->wts_bip = NULL;

		/*
		 * Put the complete block into queue
		 */
qbuf:		bp->b_next = NULL;
		bp->b_dirtyoff = 0;
		if (sp->wts_qlen++ == 0)
			sp->wts_qh = bp;
		else
			sp->wts_qt->b_next = bp;
		sp->wts_qt = bp;
		break;

	case WTS_WTM:
		dprintf(("WTM err=%x\n", sp->wts_error));
		break;

	case WTS_WRITE:
		dprintf(("WRITE err=%x\n", sp->wts_error));
		at_dma_terminate(sp->wts_chan);
		sp->wts_nwblks++;
		bp = sp->wts_bip;
		if (sp->wts_error & WTE_EOM && sp->wts_flags & WTS_DOCONT) {
			tprintf(sp->wts_tpr,
			    "wt%d: insert tape #%d to continue writing...\n",
			    sp->wts_dev.dv_unit, sp->wts_vol + 2);
			sp->wts_flags &= ~WTS_NEEDRESET;
			sp->wts_flags |= WTS_TCHANGE|WTS_WCONT|WTS_REWIND;
			if (sp->wts_flags & WTS_ARC)
				/* Archive: issue BOT to write FM and rewind */
				wtcmd(sp, WT_REWIND);
			else
				/* Wangtek: drop ONLINE to write FM and end */
				outb(wt_ctl(sp), WT_IENB(sp->wts_chan));
			return;
		} else if (sp->wts_error & (WTE_HARD | WTE_EOM)) {
			if (sp->wts_error & WTE_EOM)
				tprintf(sp->wts_tpr, "wt%d: hit end of medium\n",
				    sp->wts_dev.dv_unit);
			sp->wts_flags |= WTS_ERR;
			/* wtflush will release that wts_bip block */
			break;
		} else if (bp != NULL) {
			bp->b_dirtyoff += WTBLKSIZE;
			/* release buffer if it was successfully written */
			if (bp->b_dirtyoff > bp->b_bcount - WTBLKSIZE) {
				sp->wts_bip = NULL;
				bp->b_next = sp->wts_avail;
				sp->wts_avail = bp;
				bp = NULL;
			}
		}

		if (bp == NULL) {	/* allocate a new buffer to dma to */
			if (sp->wts_qlen == 0) {
				dprintf(("empty write Q\n"));
				break;	/* lowered WTS_WRITE */
			}
			bp = sp->wts_qh;
			sp->wts_qh = bp->b_next;
			if (--(sp->wts_qlen) == 0)
				sp->wts_qt = NULL;
			sp->wts_bip = bp;
			dprintf(("Q=%d ", sp->wts_qlen));
		}
		sp->wts_flags |= WTS_WRITE;
		wtdma(sp, bp);
		if (sp->wts_qlen > 0 && sp->wts_qlen != WTLOWAT)
			return;	/* don't wakeup */
		break;

	case WTS_REWIND:
		dprintf(("REWIND err=%x\n", sp->wts_error));
		if ((sp->wts_error & WTE_HARD) == 0) {
			sp->wts_file = 0;
			sp->wts_nwblks = 0;
			if ((sp->wts_flags & (WTS_NEEDRESET|WTS_TCHANGE)) ==
			    (WTS_NEEDRESET|WTS_TCHANGE)) {
				sp->wts_flags &= ~WTS_TCHANGE;
				if (sp->wts_flags & WTS_WCONT) {
					if (sp->wts_error & WTE_WRP) {
						tprintf(sp->wts_tpr,
						    "wt%d: write protected\n",
						    sp->wts_dev.dv_unit);
						wtrewind(sp);
						return;
					}
					sp->wts_flags &= ~(WTS_LASTWRITE|WTS_WCONT);
					wtwstart(sp);
				} else if (sp->wts_flags & WTS_RCONT) {
					wtrstart(sp);
					sp->wts_flags &= ~WTS_RCONT;
				}
				sp->wts_vol++;
			}
			sp->wts_flags &= ~WTS_NEEDRESET;
		}
		break;
	}
	if (sp->wts_error & WTE_HARD)
		wterror(sp);

	wakeup((caddr_t) sp);
}

/*
 * Polling routine: check for lost interrupts,
 * or tape changes.  We can't currently do anything
 * about hung operations (controller still busy).
 */
wtpoll()
{
	register struct wtsoftc *sp;
	int s, active = 0, err, i;

	dprintf(("p"));
	for (i = 0; i < wtcd.cd_ndevs; i++) {
		if ((sp = wtcd.cd_devs[i]) == NULL)
			continue;
		active++;	/* assume active */
		if ((sp->wts_flags & WTS_ACTIVE) == WTS_DELAY)
			continue;	/* do not disturb! */
		s = inb(sp->wts_statport);
		if ((s & sp->wts_statmask) == sp->wts_statmask)
			continue;

		if ((sp->wts_flags & (WTS_ACTIVE|WTS_FSFPEND)) == WTS_IDLE) {
			if ((sp->wts_flags & WTS_OPENED) == 0) {
				active--;	/* doing nothing */
				continue;
			}
			dprintf(("-A"));
			if (sp->wts_flags & WTS_TCHANGE) {
				wtstatus(sp);
				dprintf(("[%x]", sp->wts_error));
				if ((sp->wts_error & ~WTE_WRP) == 0 &&
				    (sp->wts_flags & WTS_NEEDRESET))
					wtrewind(sp);
			}
			continue;
		}

		if ((sp->wts_flags & WTS_TIMO) == 0) {
			sp->wts_flags |= WTS_TIMO;
			continue;
		}
		sp->wts_flags &= ~WTS_TIMO;

		sp->wts_error = 0;
		if (!(s & sp->wts_excep))
			wtstatus(sp);
		dprintf(("WTPOLL "));
		s = splbio();
		wtXintr(sp);
		splx(s);
	}
	if (active)
		timeout(wtpoll, 0, hz * 2);
}

/*
 * Magtape ioctl routine
 */
wtioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register struct wtsoftc *sp;
	struct mtop *op;
	struct mtget *mg;
	int opcnt, err, s;

	dprintf(("wtioctl "));
	sp = wtcd.cd_devs[wtunit(dev)];
	/* Wait for the end of op here */
	switch (cmd) {
	case MTIOCGET:
		dprintf(("MTIOCGET\n"));
		mg = (struct mtget *) data;
		mg->mt_type = 0x11;	/* MT_QIC150 */
		mg->mt_dsreg = sp->wts_flags;
		mg->mt_erreg = sp->wts_error;
		mg->mt_resid = 0;
		return (0);

	case MTIOCTOP:
		dprintf(("MTIOCTOP\n"));
		op = (struct mtop *) data;
		opcnt = op->mt_count;
		if (opcnt < 1)
			return (EINVAL);
		switch (op->mt_op) {
		case MTCACHE:
		case MTNOCACHE:
		case MTNOP:
			return (0);

		case MTREW:
		case MTOFFL:
			wtflush(sp, 1);
			wtrewind(sp);
			return (0);

		case MTWEOF:
			if ((sp->wts_flags & WTS_OWRITE) == 0)
				return (ENXIO);
			if (err = wtcheckpend(sp, FWRITE))
				return (err);
			wtflush(sp, 0);
			if (err = wtwfm(sp))
				return (err);
			return ((sp->wts_flags & WTS_ERR) ? EIO : 0);

		case MTFSF:
			wtflush(sp, 0);
			if (sp->wts_flags & WTS_FSFPEND)
				opcnt++;
			sp->wts_repeat = opcnt;
			sp->wts_flags &= ~(WTS_FSFPEND|WTS_ACTIVE);
			wtcmd(sp, WT_RDMARK);
			sp->wts_flags |= WTS_FSF;
			sp->wts_nwblks = WTBOTFF + 1;
			s = splbio();
			while (sp->wts_flags & WTS_ACTIVE) {
				if (err = tsleep((caddr_t) sp, PCATCH|WTPRI,
				    wtmotion, 0)) {
					splx(s);
					return (err);
				}
			}
			splx(s);
			return (sp->wts_repeat ? EIO : 0);

		case MTBSF:
			wtflush(sp, 0);
			if (err = wtcheckpend(sp, FREAD|FWRITE))
				return (err);
			opcnt = sp->wts_file - opcnt - 1;
			if (opcnt < 0)
				opcnt = 0;

			/* Rewind to the beginning */
			wtrewind(sp);
			s = splbio();
			while (sp->wts_flags & WTS_ACTIVE) {
				if (err = tsleep((caddr_t) sp, PCATCH|WTPRI,
				    wtrew, 0)) {
					splx(s);
					return (err);
				}
			}
			splx(s);
			if (opcnt == 0)
				return (0);

			/* Skip forward opcnt filemarks */
			sp->wts_repeat = opcnt;
			sp->wts_flags &= ~WTS_ACTIVE;
			wtcmd(sp, WT_RDMARK);
			sp->wts_flags |= WTS_FSF;
			sp->wts_nwblks = WTBOTFF + 1;
			s = splbio();
			while (sp->wts_flags & WTS_ACTIVE) {
				if (err = tsleep((caddr_t) sp, PCATCH|WTPRI,
				    wtmotion, 0)) {
					splx(s);
					return (err);
				}
			}
			splx(s);
			return (sp->wts_repeat ? EIO : 0);

		case MTFSR:
#ifdef WTDEBUG
			wtdebug = 1;
#endif
			break;

		case MTBSR:
#ifdef WTDEBUG
			wtdebug = 0;
#endif
			break;
		}
		return (EINVAL);

	default:
		dprintf(("UNKNOWN\n"));
		return (EINVAL);
	}
	/* NOTREACHED */
}

/*
 * Check pending operations
 */
wtcheckpend(sp, allowed_after)
	register struct wtsoftc *sp;
	int allowed_after;
{
	int s, err;

	if ((sp->wts_flags & WTS_LASTWRITE) && !(allowed_after & FWRITE))
		return (EIO);
	if ((sp->wts_flags & WTS_LASTREAD) && !(allowed_after & FREAD))
		return (EIO);
	s = splbio();
	if ((sp->wts_flags & WTS_ACTIVE) == WTS_REWIND) {
		dprintf(("checkpend: wait for rewind...\n"));
		while (sp->wts_flags & WTS_ACTIVE)
			if (err = tsleep((caddr_t) sp, PCATCH|WTPRI,
			    wtrew, 0)) {
			    	splx(s);
				return (err);
			}
		dprintf(("done\n"));
	} else if (sp->wts_flags & WTS_FSFPEND) {
		dprintf(("checkpend: skip to TM..."));
		wtcmd(sp, WT_RDMARK);
		sp->wts_flags &= ~(WTS_ACTIVE|WTS_FSFPEND);
		sp->wts_flags |= WTS_FSF;
		sp->wts_repeat = 1;
		while (sp->wts_flags & WTS_ACTIVE) {
			if (err = tsleep((caddr_t) sp, PCATCH|WTPRI,
			    wtmotion, 0)) {
			    	splx(s);
				return (err);
			}
			
		}
		dprintf(("done\n"));
	}
	splx(s);
	return (0);
}

/*
 * Write file mark
 */
wtwfm(sp)
	register struct wtsoftc *sp;
{
	int s, ftoskip, err;
	extern wakeup();

	ftoskip = 0;
	s = splbio();
	sp->wts_flags &= ~WTS_ACTIVE;

	/* A kludge to work around their stupid bug */
	if (sp->wts_nwblks <= WTBOTFF && !(sp->wts_flags & WTS_ARC)) {
		dprintf(("WTM bug avoidance\n"));
		outb(wt_ctl(sp), WT_IENB(sp->wts_chan)); /* drop WT_ONLINE */
		ftoskip = sp->wts_file + 1;
		sp->wts_flags |= WTS_REWIND;
	} else {
		dprintf(("write FM delay\n"));
		sp->wts_flags &= ~WTS_ACTIVE;
		sp->wts_flags |= WTS_DELAY;
		(void) tsleep((caddr_t) sp + 1, PCATCH|WTPRI, wtcntl, hz * 5);

		dprintf(("write FM\n"));
		sp->wts_flags &= ~WTS_ACTIVE;
		wtcmd(sp, WT_WRMARK);
		sp->wts_flags |= WTS_WTM;
		sp->wts_file++;
	}

	/* Wait for op completion */
	while (sp->wts_flags & WTS_ACTIVE)
		if (err = tsleep((caddr_t) sp, PCATCH|WTPRI, wtio, 0)) {
			splx(s);
			return (err);
		}

	/* Continue the bug avoidance mode */
	if (ftoskip > 0) {
		sp->wts_repeat = ftoskip;
		wtcmd(sp, WT_RDMARK);
		sp->wts_flags |= WTS_FSF;
		while (sp->wts_flags & WTS_ACTIVE)
			if (err = tsleep((caddr_t) sp, PCATCH|WTPRI,
			    wtmotion, 0)) {
			    	splx(s);
				return (s);
			}
	}
	splx(s);
	return (0);
}

/*
 * Switch tape density
 */
wtdensity(sp, dev)
	register struct wtsoftc *sp;
	int dev;
{
	int s, cmd, err;

	switch (dev & T_DENSEL) {
	case T_QIC150:
		cmd = WT_QIC150;
		break;
	case T_QIC120:
		cmd = WT_QIC120;
		break;
	case T_QIC24:
		cmd = WT_QIC24;
		break;
	default:
		return (ENXIO);
	}

	s = splbio();
	sp->wts_flags &= ~WTS_ACTIVE;

	dprintf(("switch density\n"));
	wtcmd(sp, cmd);
	sp->wts_flags |= WTS_DSEL;

	/* Wait for op completion */
	while (sp->wts_flags & WTS_ACTIVE)
		if (err = tsleep((caddr_t) sp, PCATCH|WTPRI, wtcntl, 0)) {
			splx(s);
			return (err);
		}

	sp->wts_cden = dev & T_DENSEL;
	splx(s);
	return (0);
}

/*
 * Read from the raw device
 */
wtread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct wtsoftc *sp = wtcd.cd_devs[wtunit(dev)];
	register struct buf *bp;
	int s, err;
	int first = 1;

	if (err = wtcheckpend(sp, FREAD))
		return (err);

	while (uio->uio_resid > 0) {
		/*
		 * Wait for blocks from tape
		 */
		s = splbio(); 
		while (sp->wts_qlen == 0) {
			if (sp->wts_flags & WTS_EOF) {
				if (first)
					sp->wts_flags &= ~WTS_EOF;
				splx(s);
				return (0);
			}
			if (sp->wts_flags & WTS_ERR) {
				splx(s);
				return (EIO);	/* after EOM */
			}
			wtrstart(sp);
			(void) tsleep((caddr_t) sp, WTPRI, wtio, 0);
		}
		splx(s);

		/*
		 * Check for i/o error
		 */
		bp = sp->wts_qh;	/* cannot be null */
		if (bp->b_bcount == 0) {
			if (bp->b_flags & B_ERROR) {
				if (!first)
					return (0);
				/* Release erroneous block */
				s = splbio();
				bp->b_flags = B_INVAL;
				sp->wts_qh = bp->b_next;
				sp->wts_qlen--;
				bp->b_next = sp->wts_avail;
				sp->wts_avail = bp;
				splx(s);
				return (EIO);
			}
			goto zeroblk;
		}
		first = 0;

		/*
		 * Copy block's body to user memory
		 */
		s = uio->uio_resid;
		err = uiomove(bp->b_un.b_addr + bp->b_dirtyoff,
		    bp->b_bcount, uio);
		if (err)
			return (err);

		/*
		 * Reduce block size or remove block from queue
		 */
		if (s < bp->b_bcount || (bp->b_flags & B_ERROR)) {
			bp->b_bcount -= s;
			bp->b_dirtyoff += s;
		} else {
	zeroblk:
			s = splbio();
			bp->b_flags = B_INVAL;
			sp->wts_qh = bp->b_next;
			sp->wts_qlen--;
			bp->b_next = sp->wts_avail;
			sp->wts_avail = bp;
			splx(s);
		}
	}
	return (0);
}

wtwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct wtsoftc *sp = wtcd.cd_devs[wtunit(dev)];
	register struct buf *bp;
	int s, err;

	if (err = wtcheckpend(sp, FWRITE))
		return (err);

	/*
	 * Write 0 bytes -- a special case
	 */
	if (uio->uio_resid == 0) {
		wtflush(sp, 0);
		if (err = wtwfm(sp))
			return (err);
		return ((sp->wts_flags & WTS_ERR) ? EIO : 0);
	}

	/*
	 * A regular write routine
	 */
	while (uio->uio_resid > 0) {
		if (sp->wts_flags & WTS_ERR)
			return (EIO);	/* alas! */

		/*
		 * Pick up the next vacant buffer (or part of buffer)
		 */
		s = splbio();
		bp = sp->wts_wbuf;
		if (bp == NULL) {
			while (sp->wts_avail == NULL &&
			    !(sp->wts_flags & WTS_ERR))
				(void) tsleep((caddr_t) sp, WTPRI, wtio, 0);
			if (sp->wts_flags & WTS_ERR) {
				splx(s);
				return (EIO);
			}
			bp = sp->wts_avail;
			sp->wts_avail = bp->b_next;
			bp->b_flags = B_WRITE;
			bp->b_bcount = 0;
			bp->b_dirtyoff = 0;
		}
		splx(s);

		/*
		 * Copy data into buffer
		 */
		s = bp->b_bufsize - bp->b_bcount;
		if (s > uio->uio_resid)
			s = uio->uio_resid;
		err = uiomove(bp->b_un.b_addr + bp->b_bcount, s, uio);
		if (err)
			return (err);
		bp->b_bcount += s;

		/*
		 * If buffer if full, put it into queue
		 */
		if (bp->b_bcount == bp->b_bufsize) {
			bp->b_next = NULL;
			s = splbio();
			if (sp->wts_qlen++ == 0)
				sp->wts_qh = bp;
			else
				sp->wts_qt->b_next = bp;
			sp->wts_qt = bp;
			wtwstart(sp);
			splx(s);
			bp = NULL;
		}
		sp->wts_wbuf = bp;
	}
	return (0);
}

wtsize()
{

	return (0);
}

wtdump()
{

	return (ENXIO);
}
