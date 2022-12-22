/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: st.c,v 1.12 1994/02/04 20:47:28 karels Exp $
 */

/*
 * Machine-independent SCSI tape driver
 */

#ifndef SCSI
you must define SCSI to use this driver
#endif

#include "param.h"
#include "systm.h"
#include "buf.h"
#include "device.h"
#include "errno.h"
#include "fcntl.h"
#include "ioctl.h"
#include "malloc.h"
#include "mtio.h"
#include "proc.h"
#include "tprintf.h"
#include "sys/tape.h"

#include "scsi.h"
#include "disktape.h"
#include "scsivar.h"
#include "scsi_ioctl.h"
#include "scsi/tape.h"

struct st_softc {
	struct	tpdevice sc_tp;	/* base tape device, must be first */
#define	TP_EXABYTE	0x8000	/* special Exabyte flag */
	struct	unit sc_unit;	/* scsi unit */

	dev_t	sc_devt;	/* for b_dev in tape op calls */
	long	sc_resid;	/* save hardware resid byte count on error */

	int	sc_ansi;	/* ANSI X3.131 conformance level */

	/* should be in tpdevice?? */
	struct	buf sc_tab;	/* transfer queue */

	/* for user formatting and tape ops (?!) */
	struct	scsi_fmt sc_fmt;
};

/* XXX these should probably go in scsi_subr.c */
static int scsi_mode_select __P((struct unit *u, char *buf, int len,
    int pf, int sp));
static int scsi_mode_sense __P((struct unit *u, char *buf, int len,
    int pc, int pagecode));

void stattach __P((struct device *parent, struct device *self, void *aux));
static int stblkno __P((struct st_softc *sc));
static int stcache __P((struct st_softc *sc, int mode));
int stclose __P((dev_t dev, int flags, int ifmt, struct proc *p));
static int stcommand __P((struct st_softc *sc, struct scsi_cdb *cdb,
    caddr_t buf, int len));
static int stdensity __P((struct st_softc *sc, int flags, int d));
int stdump __P((dev_t dev, daddr_t blkoff, caddr_t addr, int len, int isphys));
static int sterror __P((struct st_softc *sc, int stat));
void stgo __P((struct device *sc0, struct scsi_cdb *cdb));
void stigo __P((struct device *sc0, struct scsi_cdb *cdb));
void stintr __P((struct device *sc0, int stat, int resid));
int stioctl __P((dev_t dev, int cmd, caddr_t data, int flag, struct proc *p));
int stmatch __P((struct device *parent, struct cfdata *cf, void *aux));
static int stoffline __P((struct st_softc *sc));
int stopen __P((dev_t dev, int flags, int ifmt, struct proc *p));
void streset __P((struct unit *u));
static int strewind __P((struct st_softc *sc));
static int stspace __P((struct st_softc *sc, int mark, int count));
static void ststart __P((struct st_softc *sc, struct buf *bp));
void ststrategy __P((struct buf *bp));
static void sttype __P((struct st_softc *sc, char *vendor, char *rev));
static int stweof __P((struct st_softc *sc, int count));

#ifdef notyet
static struct sttpdriver = { ststrategy };
#endif

struct cfdriver stcd = 
    { NULL, "st", stmatch, stattach, sizeof (struct st_softc) };

static struct unitdriver stunitdriver = { /* stgo, stintr, */ streset };

int
stmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct scsi_attach_args *sa = aux;

#ifdef notyet
	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != sa->sa_unit)
		return (0);
#endif
	if ((sa->sa_inq_status & STS_MASK) != STS_GOOD)
		return (0);
	if ((sa->sa_si.si_type & TYPE_TYPE_MASK) != TYPE_SAD)
		return (0);
	return (1);
}

/*
 * Classify different types of tape devices.
 * This routine currently has a rather naive view of the world.
 * Note that we get called once with a vendor name at attach time,
 * and we also get called at open time to get current parameters.
 */
static void
sttype(sc, vendor, rev)
	struct st_softc *sc;
	char *vendor;
	char *rev;
{
	struct scsi_modesense *mss;
	struct scsi_ms_bdesc *mbd;
	struct tpdevice *tp = &sc->sc_tp;
	char modebuf[128];
	int i;
	int pc = PC_DEFAULT;

	/*
	 * If this device is SCSI-1, it won't respond to
	 * a MODE SENSE for its default parameters.
	 * If we recognize the vendor name, use that instead.
	 * Crude but fairly effective...
	 */
	if (sc->sc_ansi == 1) {
		if (strcmp(vendor, "EXABYTE") == 0) {
			tp->tp_flags |= TP_8MMVIDEO|TP_EXABYTE;
			return;
		}
		/* XXX should we test <= ? on revisions?  maybe others? */
		if (strcmp(vendor, "SANKYO") == 0 ||
		    strcmp(vendor, "WangDAT") == 0 ||
		    (strcmp(vendor, "WANGTEK") == 0 &&
		    rev != NULL && strcmp(rev, "C230") == 0)) {
			tp->tp_flags |= TP_NODENS;
			printf("(no density support)");
		}
		/* the Archive has a special block address command */
		if (bcmp(vendor, "ARCHIVE VIPER", 13) == 0)
			tp->tp_flags |= TP_PHYS_ADDR;

		return;
	}
	if (strcmp(vendor, "EXABYTE") == 0) {
		/*
		 * Exabyte drives which claim to be SCSI-2
		 * are non-conformant in at least a couple of ways.
		 * (1) A page control value other than PC_CURRENT is illegal.
		 * (2) When reporting default values with PC_CURRENT,
		 * the drive sends a density code of 0 (grrr...),
		 * so the density/format detection code fails.
		 * We just hack in the correct format.
		 */
		pc = PC_CURRENT;
		tp->tp_flags |= TP_8MMVIDEO|TP_EXABYTE;
	}

	i = scsi_mode_sense(&sc->sc_unit, modebuf, sizeof modebuf,
	    pc, 0);
	if ((i & STS_MASK) != STS_GOOD) {
#ifdef DEBUG
		printf(" (mode sense failed)\n");
#endif
		return;
	}

	mss = (struct scsi_modesense *)modebuf;
	if (mss->ms_bdl == 0) {
		printf(" (no block descriptor found)\n");
		return;
	}
	mbd = (struct scsi_ms_bdesc *)(modebuf + sizeof *mss);
	tp->tp_reclen = mbd->blh << 16 | mbd->blm << 8 | mbd->bll;
	if (tp->tp_reclen)
		tp->tp_flags |= TP_STREAMER;
	else
		tp->tp_flags &= ~TP_STREAMER;

	switch (mbd->dc) {
	case SCSI_MSEL_DC_9T_800BPI:
		printf(" 800"); goto reel;
	case SCSI_MSEL_DC_9T_1600BPI:
		printf(" 1600"); goto reel;
	case SCSI_MSEL_DC_9T_3200BPI:
		printf(" 3200"); goto reel;
	case SCSI_MSEL_DC_9T_6250BPI:
		printf(" 6250");
	reel:
		printf(" bpi reel-to-reel");
		tp->tp_flags |= TP_DOUBLEEOF;
		break;
	case SCSI_MSEL_DC_HIC_XX5:
	case SCSI_MSEL_DC_HIC_XX6:
	case SCSI_MSEL_DC_HI_TC1:
	case SCSI_MSEL_DC_HI_TC2:
	case SCSI_MSEL_DC_HIC_XXB:
	case SCSI_MSEL_DC_HIC_XXC:
		printf(" 1/2-inch cartridge");
		break;
	case SCSI_MSEL_DC_QIC_XX1:
	case SCSI_MSEL_DC_QIC_XX2:
	case SCSI_MSEL_DC_QIC_XX3:
	case SCSI_MSEL_DC_QIC_XX7:
		printf(" QIC-11 or QIC-24 cartridge");
		break;
	case SCSI_MSEL_DC_QIC_120:
		printf(" QIC-120 cartridge");
		break;
	case SCSI_MSEL_DC_QIC_150:
		printf(" QIC-150 cartridge");
		break;
	case SCSI_MSEL_DC_QIC_320:
		printf(" QIC-320 cartridge");
		break;
	case SCSI_MSEL_DC_QIC_1350:
		printf(" QIC-1350 cartridge");
		break;
	case SCSI_MSEL_DC_CS_XX4:
		printf(" 4-track cassette");
		break;
	case SCSI_MSEL_DC_DAT:
		printf(" DAT cassette");
		break;
	case SCSI_MSEL_DC_8MM_XX9:
	case SCSI_MSEL_DC_8MM_XXA:
		printf(" 8mm video cassette");
		tp->tp_flags |= TP_8MMVIDEO;
		if (strcmp(vendor, "EXABYTE") == 0)
			tp->tp_flags |= TP_EXABYTE;
		break;
	case 0:
		/* some drives won't provide a default density code */
		break;
	default:
		printf(" unknown format 0x%x", mbd->dc);
		break;
	}
	if (tp->tp_flags & TP_STREAMER)
		printf(" streaming (%d)", tp->tp_reclen);
	printf(" tape");
}

void
stattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct st_softc *sc = (struct st_softc *)self;
	struct scsi_attach_args *sa = aux;
	int ansi;
	char vendor[10], drive[17], rev[5];

	sc->sc_unit.u_driver = &stunitdriver;
	scsi_establish(&sc->sc_unit, &sc->sc_tp.tp_dev, sa->sa_unit);

	ansi = (sa->sa_si.si_version >> VER_ANSI_SHIFT) & VER_ANSI_MASK;
	if (ansi == 1 || ansi == 2) {
		scsi_inq_ansi((struct scsi_inq_ansi *)&sa->sa_si,
		    vendor, drive, rev);
		printf(": %s %s", vendor, drive);
		if (rev[0])
			printf(" rev %s", rev);
		if (ansi == 2)
			printf(" (SCSI-2)");
		else if ((sa->sa_si.si_v2info & V2INFO_RDF_MASK) ==
		    V2INFO_RDF_CCS)
			printf(" (CCS)");	/* XXX never seen one */
		else
			printf(" (SCSI-1)");
		sc->sc_ansi = ansi;
	} else {
		bcopy("<unknown>", vendor, 10);
		bcopy("<unknown>", drive, 10);
		printf(": type 0x%x, qual 0x%x, ver %d",
		    sa->sa_si.si_type, sa->sa_si.si_qual,
		    sa->sa_si.si_version);
		sc->sc_ansi = 1;		/* XXX */    
	}

	sttype(sc, vendor, rev);
	printf("\n");
}

void
streset(u)
	struct unit *u;
{

	/* XXX recover from SCSI bus reset? */
	printf(" %s", u->u_dev->dv_xname);
}

/*
 * Execute a (possibly blocking) tape operation with a prepared cdb.
 * XXX do we need to pass a read/write indication too?
 */
static int
stcommand(sc, cdb, buf, len)
	struct st_softc *sc;
	struct scsi_cdb *cdb;
	caddr_t buf;
	int len;
{
	struct buf *bp;
	int error;

	MALLOC(bp, struct buf *, sizeof *bp, M_DEVBUF, M_WAITOK);
	bp->b_flags = B_BUSY;		/* XXX B_READ? */
	bp->b_error = 0;
	bp->b_dev = sc->sc_devt;
	bp->b_resid = 0;
	bp->b_proc = curproc;
	bp->b_iodone = 0;
	bp->b_vp = 0;
	bp->b_un.b_addr = buf;
	bp->b_bcount = len;
	bp->b_blkno = 0;

	sc->sc_fmt.sf_format_pid = curproc ? curproc->p_pid : -1;
	sc->sc_fmt.sf_cmd = *cdb;

	ststrategy(bp);
	error = biowait(bp);

	FREE(bp, M_DEVBUF);
	sc->sc_fmt.sf_format_pid = 0;

	return (error);
}

static int
strewind(sc)
	struct st_softc *sc;
{
	struct scsi_cdb cdb = { CMD_REWIND };

	sc->sc_tp.tp_flags &= ~(TP_WRITING|TP_MOVED|TP_SEENEOF);
	sc->sc_tp.tp_fileno = 0;
	cdb.cdb_bytes[1] = sc->sc_unit.u_unit << 5;
	return (stcommand(sc, &cdb, (caddr_t)0, 0));
}

static int
stoffline(sc)
	struct st_softc *sc;
{
	struct scsi_cdb cdb = { CMD_LOAD_UNLOAD };

	sc->sc_tp.tp_flags &= ~(TP_WRITING|TP_MOVED|TP_SEENEOF);
	sc->sc_tp.tp_fileno = 0;
	cdb.cdb_bytes[1] = sc->sc_unit.u_unit << 5;
	return (stcommand(sc, &cdb, (caddr_t)0, 0));
}

static int
stspace(sc, code, count)
	struct st_softc *sc;
	int code, count;
{
	struct scsi_cdb cdb = { CMD_SPACE };

	sc->sc_tp.tp_flags &= ~(TP_WRITING|TP_SEENEOF);
	/* XXX what if we backspace to BOT? */
	sc->sc_tp.tp_flags |= TP_MOVED;
	if (code == SCSI_CMD_SPACE_FMS)
		sc->sc_tp.tp_fileno += count;
	cdb.cdb_bytes[1] = sc->sc_unit.u_unit << 5 | code;
	cdb.cdb_bytes[2] = count >> 16;
	cdb.cdb_bytes[3] = count >> 8;
	cdb.cdb_bytes[4] = count;
	return (stcommand(sc, &cdb, (caddr_t)0, 0));
}

static int
stweof(sc, count)
	struct st_softc *sc;
	int count;
{
	struct scsi_cdb cdb = { CMD_WRITE_FILEMARK };

	sc->sc_tp.tp_flags |= TP_WRITING|TP_MOVED;
	sc->sc_tp.tp_fileno += count;
	cdb.cdb_bytes[1] = sc->sc_unit.u_unit << 5;
	cdb.cdb_bytes[2] = count >> 16;
	cdb.cdb_bytes[3] = count >> 8;
	cdb.cdb_bytes[4] = count;
	return (stcommand(sc, &cdb, (caddr_t)0, 0));
}

static int
scsi_mode_sense(u, buf, len, pc, pagecode)
	struct unit *u;
	char *buf;
	int len;
	int pc, pagecode;
{
	struct scsi_cdb ms = { CMD_MODE_SENSE };

	ms.cdb_bytes[1] = u->u_unit << 5;
	ms.cdb_bytes[2] = pc << 6 | pagecode & 0x3f;
	ms.cdb_bytes[4] = len;

	return ((*u->u_hbd->hd_icmd)(u->u_hba,
	    u->u_targ, &ms, buf, len, B_READ));
}

static int
scsi_mode_select(u, buf, len, pf, sp)
	struct unit *u;
	char *buf;
	int len;
	int pf, sp;
{
	struct scsi_cdb ms = { CMD_MODE_SELECT };
	struct scsi_modesense *mss = (struct scsi_modesense *)buf;

	/* is this necessary? */
	mss->ms_len = 0;
	mss->ms_mt = 0;
	mss->ms_wbs &= 0x7f;	/* turn off write protect? */
	/* XXX assume that len is consistent with mss->ms_bdl */

	ms.cdb_bytes[1] = u->u_unit << 5 | (pf & 1) << 4 | sp & 1;
	ms.cdb_bytes[4] = len;

	return ((*u->u_hbd->hd_icmd)(u->u_hba,
	    u->u_targ, &ms, buf, len, B_WRITE));
}

static int
stcache(sc, mode)
	struct st_softc *sc;
	int mode;
{
	struct scsi_modesense *mss;
	char modebuf[128];	/* assume exactly one block descriptor */
	int i, error;

resense:
	i = scsi_mode_sense(&sc->sc_unit, modebuf, sizeof modebuf, 0, 0);
	if ((i & STS_MASK) != STS_GOOD && (error = sterror(sc, i))) {
		if (error == EAGAIN)
			goto resense;
		return (error);
	}

	mss = (struct scsi_modesense *)modebuf;
	mss->ms_wbs &= 0xf;
	mss->ms_wbs |= (mode & 7) << 4;

reselect:
	i = scsi_mode_select(&sc->sc_unit, modebuf, sizeof modebuf, 0, 0);
	if ((i & STS_MASK) != STS_GOOD && (error = sterror(sc, i))) {
		if (error == EAGAIN)
			goto reselect;
		return (error);
	}
	return (0);
}

static int
stdensity(sc, flags, d)
	struct st_softc *sc;
	int flags, d;
{
	struct scsi_modesense *mss;
	struct scsi_ms_bdesc *mbd;
	struct tpdevice *tp = &sc->sc_tp;
	char modebuf[128];
	int i, select_len;

	/*
	 * Get parameters from the drive.
	 * We do this regardless of whether we can set them.
	 */
	i = scsi_mode_sense(&sc->sc_unit, modebuf, sizeof modebuf, 0, 0);
	if ((i & STS_MASK) != STS_GOOD)
		return (i);

	mss = (struct scsi_modesense *)modebuf;
	if (mss->ms_bdl == 0)
		return (-1);	/* XXX never happens? */
	mbd = (struct scsi_ms_bdesc *)(modebuf + sizeof *mss);
	tp->tp_reclen = mbd->blh << 16 | mbd->blm << 8 | mbd->bll;
	if (tp->tp_reclen)
		tp->tp_flags |= TP_STREAMER;
	else
		tp->tp_flags &= ~TP_STREAMER;

	if (tp->tp_flags & TP_NODENS)
		/* Tapes with bad density selection */
		return (STS_GOOD);

	mbd->dc = d;

	select_len = sizeof(*mss) + mss->ms_bdl;
	if (tp->tp_flags & TP_8MMVIDEO) {
		/* horrible kluge follows... */
		if (d == SCSI_MSEL_DC_8MM_XX9 || d == SCSI_MSEL_DC_8MM_XXA) {
			/* force fixed length 1024-byte records */
			tp->tp_reclen = 1024;
			tp->tp_flags |= TP_STREAMER;
			mbd->blh = mbd->bll = 0;
			mbd->blm = 1024 >> 8;
		} else {
			/* use variable length records */
			tp->tp_reclen = 0;
			tp->tp_flags &= ~TP_STREAMER;
			mbd->blh = mbd->blm = mbd->bll = 0;
		}
		if (tp->tp_flags & TP_EXABYTE) {
			/* Exabyte insists on zero length in mode select */
			mbd->nbh = mbd->nbm = mbd->nbl = 0;
			/* set NBE in Exabyte vendor-unique parameters page */
			modebuf[12] |= 8;
			select_len = mss->ms_len + sizeof(mss->ms_len);
		}
		mbd->dc = 0;
	}
	if (select_len > sizeof(modebuf))
		select_len = sizeof(modebuf);

	return (scsi_mode_select(&sc->sc_unit, modebuf, select_len, 0, 0));
}

static int
stblkno(sc)
	struct st_softc *sc;
{
	struct scsi_cdb rp = { CMD_READ_POSITION };
	u_char posbuf[20];
	int i;

	if (sc->sc_ansi == 2) {
		/*
		 * Report the logical block number.
		 */
		rp.cdb_bytes[1] = sc->sc_unit.u_unit << 5;
		i = (*sc->sc_unit.u_hbd->hd_icmd)(sc->sc_unit.u_hba,
		    sc->sc_unit.u_targ, &rp, (char *)posbuf, sizeof posbuf,
		    B_READ);
		if ((i & STS_MASK) != STS_GOOD)
			return (-1);
		if (posbuf[0] & READ_POS_BPU)
			return (-1);
		return (posbuf[4] << 24 | posbuf[5] << 16 | posbuf[6] << 8 |
		    posbuf[7]);
	}

	/*
	 * If it's a Viper, opcode #2 returns the physical block number.
	 * Is this a CCS thing, an Archive thing, something else?
	 */
	if (sc->sc_tp.tp_flags & TP_PHYS_ADDR) {
		rp.cdb_bytes[0] = 0x2;
		i = (*sc->sc_unit.u_hbd->hd_icmd)(sc->sc_unit.u_hba,
		    sc->sc_unit.u_targ, &rp, (char *)posbuf, 3, B_READ);
		if ((i & STS_MASK) != STS_GOOD)
			return (-1);
		return (posbuf[0] << 16 | posbuf[1] << 8 | posbuf[0]);
	}

	return (-1);
}

int
#ifdef __STDC__
stopen(dev_t dev, int flags, int ifmt, struct proc *p)
#else
stopen(dev, flags, ifmt, p)
	dev_t dev;
	int flags, ifmt;
	struct proc *p;
#endif
{
	struct st_softc *sc;
	int unit = tpunit(dev);
	int i;
	int error;

	if (unit >= stcd.cd_ndevs)
		return (ENXIO);
	if ((sc = stcd.cd_devs[unit]) == NULL)
		/* XXX try attaching on the fly? */
		return (ENXIO);
	if (sc->sc_tp.tp_flags & TP_OPEN)
		return (EBUSY);
	/* do this now, in case we block */
	sc->sc_tp.tp_flags |= TP_OPEN;

	/*
	 * Send most error messages to the controlling tty,
	 * since they typically don't require administrator attention.
	 * If there is no controlling tty, output goes to the console.
	 */
	sc->sc_tp.tp_ctty = tprintf_open(p);

again:
	i = scsi_test_unit_ready(sc->sc_unit.u_hba, sc->sc_unit.u_targ,
	    sc->sc_unit.u_unit);
	if ((i & STS_MASK) != STS_GOOD && (error = sterror(sc, i))) {
		/* XXX try a load command if the tape appears to be offline? */
		if (error == EAGAIN)
			goto again;
		tprintf_close(sc->sc_tp.tp_ctty);
		sc->sc_tp.tp_flags &= ~TP_OPEN;
		return (error);
	}
	sc->sc_devt = dev;

	/*
	 * Set tape density.  The SCSI-2 standard defines
	 * 24 different density codes; it's tough to map
	 * these onto small numbers for the set that any
	 * given drive supports (e.g. there are a half dozen
	 * different QIC formats) so we don't bother.
	 * The high 5 bits of the minor device number are
	 * used as the low 5 bits of the density code.
	 * Zero is the default density.
	 */
density:
	i = stdensity(sc, flags, minor(dev) >> 3);
	if ((i & STS_MASK) != STS_GOOD && (error = sterror(sc, i))) {
		if (error == EAGAIN)
			goto density;
		tprintf(sc->sc_tp.tp_ctty,
		    "%s: unsupported density selection\n",
		    sc->sc_tp.tp_dev.dv_xname);
		tprintf_close(sc->sc_tp.tp_ctty);
		sc->sc_tp.tp_flags &= ~TP_OPEN;
		return (error);
	}

	/*
	 * On a QIC streamer, we can only write from BOT or from EOD.
	 * If we need to write and we're not at BOT, move to EOD.
	 * XXX what if we want to read and then write?
	 */
	if (flags & FWRITE &&
	    (sc->sc_tp.tp_flags & (TP_STREAMER|TP_8MMVIDEO|TP_MOVED))
	     == (TP_STREAMER|TP_MOVED))
		stspace(sc, SCSI_CMD_SPACE_PEOD, 1);

	return (0);
}

int
#ifdef __STDC__
stclose(dev_t dev, int flags, int ifmt, struct proc *p)
#else
stclose(dev, flags, ifmt, p)
	dev_t dev;
	int flags, ifmt;
	struct proc *p;
#endif
{
	struct st_softc *sc = stcd.cd_devs[tpunit(dev)];
	int error = 0;

	if (sc->sc_tp.tp_flags & TP_WRITING)
		error = stweof(sc, sc->sc_tp.tp_flags & TP_DOUBLEEOF ? 2 : 1);
	if (!tpnorew(dev))
		if (!error)
			error = strewind(sc);
		else
			(void)strewind(sc);
	tprintf_close(sc->sc_tp.tp_ctty);
	sc->sc_tp.tp_flags &= ~(TP_OPEN|TP_WRITING|TP_SEENEOF);
	sc->sc_fmt.sf_format_pid = 0;	/* XXX exclusive open... */
	/* XXX pick up sense data for 'mt status'? */
	return (error);
}

void
ststrategy(bp)
	struct buf *bp;
{
	struct st_softc *sc = stcd.cd_devs[tpunit(bp->b_dev)];
	struct buf *dp = &sc->sc_tab;
	int s;

	bp->av_forw = NULL;
	s = splbio();
	if (dp->b_actf == 0)
		dp->b_actf = bp;
	else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	if (dp->b_active == 0) {
		dp->b_active = 1;
		ststart(sc, bp);
	}
	splx(s);
}

/*
 * Report errors to the user, rather than the console.
 * Tape errors are typically not critical system problems.
 */
static int
sterror(sc, stat)
	struct st_softc *sc;
	int stat;
{
	struct scsi_sense *sn;
	struct tpdevice *tp = &sc->sc_tp;
	char *s = tp->tp_dev.dv_xname;
	char *m;
	tpr_t t = tp->tp_ctty;
	int key;

	sc->sc_resid = 0;
	sc->sc_fmt.sf_sense.status = stat;
	if ((stat & STS_MASK) == STS_CHECKCOND) {
		sn = (struct scsi_sense *)sc->sc_fmt.sf_sense.sense;
		stat = scsi_request_sense(sc->sc_unit.u_hba,
		    sc->sc_unit.u_targ, sc->sc_unit.u_unit,
		    (caddr_t)sn, sizeof *sn);
		if ((stat & STS_MASK) != STS_GOOD) {
			sc->sc_fmt.sf_sense.status = stat;	/* ??? */
			tprintf(t, "%s: sense failed, status %x\n", s, stat);
			return (EIO);
		}
		if (!SENSE_ISXSENSE(sn) || !XSENSE_ISSTD(sn)) {
			tprintf(t, "%s: scsi sense class %d, code %d\n",
			    s, SENSE_ECLASS(sn), SENSE_ECODE(sn));
			return (EIO);
		}
		if (XSENSE_IVALID(sn)) {
			sc->sc_resid = XSENSE_INFO(sn);
			if (tp->tp_flags & TP_STREAMER)
				sc->sc_resid *= tp->tp_reclen;
		}
		key = XSENSE_KEY(sn);
		if (key == SKEY_ATTENTION) {
			if (tp->tp_flags & TP_MOVED)
			    tprintf(t, "%s: tape changed without rewind\n", s);
			tp->tp_flags &= ~(TP_MOVED|TP_WRITING);
			tp->tp_fileno = 0;
			return (EAGAIN);
		}
		if (key != SKEY_NONE && key != SKEY_BLANK) {
			if (XSENSE_HASASCQ(sn))
				m = "%s: scsi sense: %s: asc 0x%x, ascq 0x%x\n";
			else if (XSENSE_HASASC(sn))
				m = "%s: scsi sense: %s: asc 0x%x\n";
			else
				m = "%s: scsi sense: %s\n";
			tprintf(t, m, s, SCSISENSEKEYMSG(key),
			    XSENSE_ASC(sn), XSENSE_ASCQ(sn));
			if (key != SKEY_RECOVERED)
				return (EIO);
		}
		if (XSENSE_ILI(sn)) {
			/* XXX use the SILI bit? */
			if (tp->tp_flags & TP_STREAMER) {
				/* fixed length record mismatch */
				tprintf(t, "%s: short record (%d bytes)\n",
				    s, tp->tp_reclen - sc->sc_resid);
				return (EIO);
			}
			if (sc->sc_resid < 0) {
			    tprintf(t, "%s: overlength error, excess = %d\n",
				s, -sc->sc_resid);
				sc->sc_resid = 0;
				return (EIO);
			}
		}
		if (XSENSE_FM(sn)) {
			/* EOF on read */
			tp->tp_flags |= TP_SEENEOF;
			return (0);
		}
		if (XSENSE_EOM(sn)) {
			tprintf(t, "%s: end of medium\n", s);
			return (ENOSPC);
		}
		return (0);
	}
#ifdef DEBUG
	if ((sc->sc_fmt.sf_sense.status & STS_MASK) != STS_GOOD)
		tprintf(t, "%s: peculiar scsi status %d\n", s,
		    sc->sc_fmt.sf_sense.status);
#endif
	return ((sc->sc_fmt.sf_sense.status & STS_MASK) != STS_GOOD ?
	    EAGAIN : 0);
}

static void
ststart(sc, bp)
	struct st_softc *sc;
	struct buf *bp;
{

	/*
	 * If we've loaded a SCSI command into sf_cmd and
	 * the table indicates that it completes quickly,
	 * use polling to get the result.
	 */
	if (sc->sc_fmt.sf_format_pid &&
	    scsi_legal_cmds[sc->sc_fmt.sf_cmd.cdb_bytes[0]] > 0) {
		(*sc->sc_unit.u_start)(sc->sc_unit.u_updev,
		    &sc->sc_unit.u_forw, (struct buf *)NULL,
		    stigo, &sc->sc_tp.tp_dev);
		return;
	}
	(*sc->sc_unit.u_start)(sc->sc_unit.u_updev, &sc->sc_unit.u_forw,
	    bp, stgo, &sc->sc_tp.tp_dev);
}

void
stigo(sc0, cdb)
	struct device *sc0;
	struct scsi_cdb *cdb;
{
	struct st_softc *sc = (struct st_softc *)sc0;
	struct buf *bp = sc->sc_tab.b_actf;
	int error;
	int stat;

again:
	sc->sc_resid = 0;
	stat = (*sc->sc_unit.u_hbd->hd_icmd)(sc->sc_unit.u_hba,
	    sc->sc_unit.u_targ, &sc->sc_cmd, bp->b_un.b_addr, bp->b_bcount,
	    bp->b_flags & B_READ);
	sc->sc_sense.status = stat;
	if ((stat & STS_MASK) != STS_GOOD && (error = sterror(sc, stat))) {
		if (error == EAGAIN)
			goto again;
		bp->b_flags |= B_ERROR;
		bp->b_error = error;
	}

	(*sc->sc_unit.u_rel)(sc->sc_unit.u_updev);
	bp->b_resid = sc->sc_resid;
	sc->sc_tab.b_errcnt = 0;
	sc->sc_tab.b_actf = bp->b_actf;
	biodone(bp);
	/* XXX what happens if sf_format_pid is set? */
	if ((bp = sc->sc_tab.b_actf) == NULL)
		sc->sc_tab.b_active = 0;
	else
		ststart(sc, bp);
}

void
stgo(sc0, cdb)
	struct device *sc0;
	struct scsi_cdb *cdb;
{
	struct st_softc *sc = (struct st_softc *)sc0;
	struct buf *bp = sc->sc_tab.b_actf;
	int n;

	if (sc->sc_format_pid) {
		*cdb = sc->sc_fmt.sf_cmd;
		n = 0;
	} else {
		if (bp->b_flags & B_READ) {
			sc->sc_tp.tp_flags &= ~TP_WRITING;
			if (sc->sc_tp.tp_flags & TP_SEENEOF) {
				bp->b_resid = bp->b_bcount;
				sc->sc_tp.tp_flags &= ~TP_SEENEOF;
				++sc->sc_tp.tp_fileno;
				goto next;
			}
		} else
			sc->sc_tp.tp_flags |= TP_WRITING;
		sc->sc_tp.tp_flags |= TP_MOVED;
		sc->sc_tp.tp_flags &= ~TP_SEENEOF;
		cdb->cdb_bytes[0] = bp->b_flags & B_READ ?
		    CMD_READ : CMD_WRITE;
		cdb->cdb_bytes[1] = sc->sc_unit.u_unit << 5;
		n = bp->b_bcount;
		if (sc->sc_tp.tp_flags & TP_STREAMER) {
			cdb->cdb_bytes[1] |= 1;		/* fixed records */
			n /= sc->sc_tp.tp_reclen;
#if 0
			if (n * sc->sc_tp.tp_reclen != bp->b_bcount)
				tprintf(sc->sc_tp.tp_ctty,
    "%s: record length %d is not a multiple of tape record length %d\n",
				    sc->sc_tp.tp_dev.dv_xname,
				    bp->b_bcount, sc->sc_tp.tp_reclen);
#endif
		}
		cdb->cdb_bytes[2] = n >> 16;
		cdb->cdb_bytes[3] = n >> 8;
		cdb->cdb_bytes[4] = n;
		cdb->cdb_bytes[5] = 0;
	}
	if ((*sc->sc_unit.u_go)(sc->sc_unit.u_updev, sc->sc_unit.u_targ,
	    stintr, (void *)sc, bp, 0) == 0)
		return;

	bp->b_flags |= B_ERROR;
	bp->b_error = EIO;
	bp->b_resid = 0;
next:
	(*sc->sc_unit.u_rel)(sc->sc_unit.u_updev);
	sc->sc_tab.b_errcnt = 0;
	sc->sc_tab.b_actf = bp->b_actf;
	biodone(bp);
	/* XXX what happens if sf_format_pid is set? */
	if ((bp = sc->sc_tab.b_actf) == NULL)
		sc->sc_tab.b_active = 0;
	else
		ststart(sc, bp);
}

void
stintr(sc0, stat, resid)
	struct device *sc0;
	int stat, resid;
{
	struct st_softc *sc = (struct st_softc *)sc0;
	struct buf *bp = sc->sc_tab.b_actf;
	int error;

	if (bp == NULL)
		panic("stintr");
	sc->sc_resid = 0;	/* can be set by sterror() */
	if ((stat & STS_MASK) != STS_GOOD && (error = sterror(sc, stat))) {
		if (error == EAGAIN)
			goto again;
		bp->b_flags |= B_ERROR;
		bp->b_error = error;
	}
	bp->b_resid = sc->sc_resid ? sc->sc_resid : resid;
#ifdef DEBUG
	if (bp->b_resid < 0 || bp->b_resid > bp->b_bcount) {
printf("%s: excessive resid, count = %d, hba resid = %d, xsense resid = %d\n",
		    sc->sc_tp.tp_dev.dv_xname, bp->b_bcount,
		    resid, sc->sc_resid);
		bp->b_resid = bp->b_bcount;
	}
#endif
	if (bp->b_resid == bp->b_bcount) {
		/* we'll see it this time rather than next time */
		if (sc->sc_tp.tp_flags & TP_SEENEOF)
			++sc->sc_tp.tp_fileno;
		sc->sc_tp.tp_flags &= ~TP_SEENEOF;
	}
	sc->sc_tab.b_errcnt = 0;
	sc->sc_tab.b_actf = bp->b_actf;
	biodone(bp);
	if ((bp = sc->sc_tab.b_actf) == NULL)
		sc->sc_tab.b_active = 0;
	else
again:
		ststart(sc, bp);
}

int
#ifdef __STDC__
stioctl(dev_t dev, int cmd, caddr_t data, int flag, struct proc *p)
#else
stioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	struct proc *p;
#endif
{
	struct st_softc *sc = stcd.cd_devs[tpunit(dev)];
	u_char *sn;
	struct mtop *op;
	struct mtget *mtget;
	int cnt;
	int error;

	switch (cmd) {

	/*
	 * XXX ALL SCSI 'FORMAT' CODE SHOULD GET MERGED...
	 * XXX WE SHOULD ALSO CHANGE THE NAME TO SOMETHING OTHER THAN 'FORMAT'
	 * XXX SINCE IT DOES A LOT MORE THAN FORMATTING, EVEN WITH DISKS.
	 * XXX WE NEED TO CHANGE IOCTL NAMES TOO (SDIOC... => SCSIIOC...?).
	 */

	case SDIOCSFORMAT:
		if (suser(p->p_ucred, &p->p_acflag))
			return (EPERM);
		if (*(int *)data) {
			if (sc->sc_fmt.sf_format_pid)
				return (EPERM);
			sc->sc_fmt.sf_format_pid = p->p_pid;
		} else
			sc->sc_fmt.sf_format_pid = 0;
		break;

	case SDIOCGFORMAT:
		*(int *)data = sc->sc_fmt.sf_format_pid;
		break;

	case SDIOCSCSICOMMAND:
#define cdb ((struct scsi_cdb *)data)
		if (sc->sc_fmt.sf_format_pid != p->p_pid)
			return (EPERM);
		if (scsi_legal_cmds[cdb->cdb_bytes[0]] == 0)
			return (EINVAL);
		sc->sc_fmt.sf_cmd = *cdb;
#undef	cdb
		break;

	case SDIOCSENSE:
		*(struct scsi_fmt_sense *)data = sc->sc_fmt.sf_sense;
		break;

	case MTIOCTOP:
		/*
		 * XXX a normal tape op mixed with SCSI ops will
		 * XXX wipe out the SCSI format_pid state...
		 */
		error = 0;
		op = (struct mtop *)data;
		cnt = op->mt_count;
		switch(op->mt_op) {

		case MTWEOF:
			error = stweof(sc, cnt);
			break;
		case MTFSR:
			error = stspace(sc, SCSI_CMD_SPACE_BLOCKS, cnt);
			break;
		case MTBSR:
			error = stspace(sc, SCSI_CMD_SPACE_BLOCKS, -cnt);
			break;
		case MTFSF:
			error = stspace(sc, SCSI_CMD_SPACE_FMS, cnt);
			break;
		case MTBSF:
			error = stspace(sc, SCSI_CMD_SPACE_FMS, -cnt);
			break;
		case MTREW:
			error = strewind(sc);
			break;
		case MTOFFL:
			error = stoffline(sc);
			break;
		case MTNOP:
			/* XXX get status? */
			break;
		case MTCACHE:
			error = stcache(sc, SCSI_MSEL_BM_BUFFERED);
			break;
		case MTNOCACHE:
			error = stcache(sc, SCSI_MSEL_BM_UNBUFFERED);
			break;

		default:
			return (EINVAL);
		}
		return (error);

	case MTIOCGET:
		mtget = (struct mtget *)data;
		sn = sc->sc_fmt.sf_sense.sense;
		mtget->mt_type = 0;		/* XXX */
		mtget->mt_dsreg = scsi_request_sense(sc->sc_unit.u_hba,
		    sc->sc_unit.u_targ, sc->sc_unit.u_unit,
		    (caddr_t)sn, sizeof *sn);
		mtget->mt_erreg = sn[0] << 8 | sn[2];
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_fileno = sc->sc_tp.tp_fileno;
		mtget->mt_blkno = stblkno(sc);
		break;

	default:
		return (EINVAL);
	}

	return (0);
}

int
#ifdef __STDC__
stdump(dev_t dev, daddr_t blkoff, caddr_t addr, int len, int isphys)
#else
stdump(dev, blkoff, addr, len, isphys)
	dev_t dev;
	daddr_t blkoff;
	caddr_t addr;
	int len, isphys;
#endif
{
#ifdef notyet
	should be fairly similar to sddump()
#endif
	return (EIO);
}
