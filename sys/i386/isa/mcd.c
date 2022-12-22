/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: mcd.c,v 1.14 1994/01/29 05:29:30 karels Exp $
 */

/*
 * Block and raw disk driver for the Mitsumi CD-ROM model CRMC
 *
 * Pace Willisson
 * Berkeley Software Design, Inc.
 * August 1992
 *
 * Tested with:
 *
 * Model	Firmware version
 * LU002	M2
 * LU005	M4
 * FX001	D2
 * 
 * Config info:
 *
 * 	device		mcd0	at isa? port 0x334 irq 9 drq ?
 *
 * This board cannot auto configure its interrupt, so you have
 * to give it explicitly, if you want to use it.  If you have a 
 * fast machine, you can do without interrupts - just leave off
 * the irq specification in the config file.  This depends on
 * the timeout queue being run fairly often.  It seems to work
 * on a 33MHz 486, but on a 25MHz 386, you get corrupted data
 * in about .1% of the transfers.
 *
 * This driver used to use programmed i/o, but that also causes 
 * corrupted data on slow machines.  Now, we only support using
 * the card in DMA mode.  The old non-dma code is still present,
 * if you want to take a chance.
 *
 * If you give "drq ?", then the dma request will be set to 3 if
 * you are using the 8 bit controller card, or 6 if you are using
 * the 16 bit card.  You can override the default by placing an
 * explicit channel number here.
 *
 * DMA with the 8 bit controller is very strange.  The DMA request line is a 
 * level sensitive signal, and the card leaves it active between
 * transfers.  It only drops it a little while after the read command
 * is given, then starts wiggling it as you would expect when the
 * data is ready.  If you enable the dma controller before the 
 * card drops the request, then it will happily runs cycles as
 * fast as it can, and store random trash in the data buffer.
 * Therefore, the code just after sending the MCD_SEEK_AND_READ
 * command is careful to wait until the request drops before loading
 * the dma chip.
 *
 * DMA on the new 16 bit controller is a little easier.  It usually
 * keeps the request line low.  The only rule is that you have
 * to wait 10 milliseconds after getting the initial status from the
 * MCD_SEEK_AND_READ command before enabling the dma controller.
 *
 * Also, the card apparantly ignores commands sometimes, so there
 * are a couple of places where the command is repeated until the
 * board finally responds.  All of this was extracted from the dos
 * assembly language driver.
 *
 * All references to the manual are to the 16 page document called
 * "Standard Specification:  CD-ROM Drive CRMC-LU002S"  June 14, 1991
 *
 * Page 13 of the manual describes the timing constraints for sending
 * commands to the board.  The time between bytes must be between
 * 500ns and 30us.  According to "AT Bus Design" by Edward Solari,
 * the minimum i/o write is about 660ns on an 8.33MHz bus (See page
 * 6-91 parameters 8 plus 13).  8.33MHz is the standard speed for an 
 * EISA bus, and the maximum safe speed for an ISA bus.  Therefore, we 
 * do not need to insert delays in a string of output instructions.
 * Some machines allow you to increase the bus speed, but it is well
 * known that this breaks some perpherials.  We don't know if this
 * card can successfully do fast cycles, even if they are properly
 * separated.  If it turns out that it can, and we want to support
 * such a configuration, then set the flag MCD_BUS_TOO_FAST.
 *
 * The timing diagrams don't say anything about how quickly you can
 * do status and non-dma data reads.  The one hint is that the peak
 * data transfer rate is claimed to be 1.6 Megabytes/s, which is
 * 62.5 ns/byte.  Therefore, we should be able to do data reads
 * with an in-string instruction.
 */

/* 
 * Note: This is not tested for multiple controllers.  The only code
 * that I know is lacking is that the timeout handling is wrong if
 * one board has hardware interrupts and another board doesn't.
 */

#include "param.h"
#include "buf.h"
#include "time.h"
#include "kernel.h"
#include "errno.h"
#include "device.h"
#include "malloc.h"
#include "ioctl.h"
#include "stat.h"

#include "icu.h"
#include "isavar.h"

#include "dma.h"

#include "mcdreg.h"
#include "mcdioctl.h"

int wakeup();

int mcddebug;
extern int autodebug;

#define	MCD_TIMEOUT	8 /* max seconds to wait for command complete */

#define	BLOCKS_PER_SECOND	75
#define	SECONDS_PER_MINUTE	60
#define	BLOCKS_PER_MINUTE	(BLOCKS_PER_SECOND * SECONDS_PER_MINUTE)

#define	MCD_BSIZE		2048
#define	MCD_BLKBUF_DEFAULT_SIZE	(32*1024)

#define	MCD_BLK_OPENED		1
#define	MCD_CHR_OPENED		2

#define	MCD_OPEN_MODE_FLAG(mode) \
	(((mode)==S_IFBLK) ? MCD_BLK_OPENED : MCD_CHR_OPENED)

/* states */
enum mcd_states {
	MCD_IDLE = 0,
	MCD_CHECK_FOR_COMMAND_SENT,
	MCD_AWAIT_DATA, 
	MCD_AWAIT_STATUS_AFTER_DATA,
	MCD_AWAIT_STATUS_AFTER_CMD,
	MCD_AWAIT_TERMINAL_COUNT,
	MCD_AWAIT_STATUS_AFTER_TERMINAL_COUNT,
	MCD_CHECK_FOR_COMMAND_SENT_NEW,
};

struct mcd_softc {
	struct	device sc_dev;
	struct	isadev sc_id;
	struct	intrhand sc_ih;

	int	opened;
	int	open_lock;

	int	iobase;
	struct	buf cmdbuf;
	struct	buf requests;

	enum	mcd_states state;

	int	start_cdblk;	/* MCD_BSIZE blocks */
	int	end_cdblk;

	int	nblks;		/* DEV_BSIZE blocks */

	int	disk_changed;

	char	args[MCDMAXARGS];
	int	nargs;
	char	results[MCDMAXARGS];
	int	nresults;

	time_t	start_time;

	int	blkno;
	char	*addr;
	int	resid;

	int	cdblk;

	int	chunk_blkno;	/* DEV_BSIZE blocks */
	int	chunk_ncdblks;	/* MCD_BSIZE blocks */
	int	chunk_done;
	char	*chunk_ptr;

	/* 
	 * The blkbuf is used for two purposes:  First, to allow
	 * random 512 byte accesses, even though the disk uses
	 * 2048 byte blocks.  Second, it allows us to do large 
	 * read aheads for greatly increased performance.
	 */
	int	use_blkbuf;
	int	blkbuf_blkno;
	int	blkbuf_valid; /* in DEV_BSIZE blocks */
	int	blkbuf_size;
	char	*blkbuf;

	int	use_dma;
	int	dmachan;

	int	block_retry_count;
	int	command_retry_count;
	int	timer;

	int	chn;
	int	ver_letter;
	int	ver_number;
	int	drive_type;
};

#define MCD_DMA_PEND(mcd) (inb((mcd->dmachan < 4) ? DMA1_STATUS:DMA2_STATUS)&\
			   DMA_REQPEND(mcd->dmachan & 3))

#define MCD_DMA_TERM(mcd) (inb((mcd->dmachan < 4) ? DMA1_STATUS:DMA2_STATUS)&\
			   DMA_TERM(mcd->dmachan & 3))

static int mcd_tradeoff;

static int mcd_timeout_needed;
static int mcd_timeout_running;

#define	mcd_unit(dev) (minor(dev) >> 3)

#define	decode_bcd(val) ((((val) >> 4) & 0xf) * 10 + ((val) & 0xf))
#define	encode_bcd(val) ((((val) / 10) << 4) | ((val) % 10))

static int mcdcmd __P((struct mcd_softc *mcd, char *args, int nargs,
	       char *results, int nresults, enum uio_seg results_seg));
static void mcdtimeout __P((void *));
static void mcd_output_command __P((struct mcd_softc *, char *, int));

#define	b_cylin b_resid

int mcdprobe(), mcdintr();
void mcdattach(); 

struct cfdriver mcdcd =
	{ NULL, "mcd", mcdprobe, mcdattach, sizeof (struct mcd_softc) };

int
mcd_busy_wait_for_status (int port)
{
	int i;
	for (i = 0; i < 50; i++)
		if (mcd_get_flags(port) & MCD_STATUS_AVAIL)
			return (0);
	return (-1);
}

/*
 * This is called from probe and attach.
 */
int
mcd_get_version(int port, int *letterp, int *numberp)
{
	int try;
	int i;
	u_char results[3];

	try = 0;
 retry:
	try++;

	if (try > 3)
		return (-1);

	if (autodebug && try > 1)
		printf("mcd %x: retry get version\n", port);

	/* flush old status */
	for (i = 0; i < 10; i++)
		inb(port + MCD_DATA);

	/* send command */
	outb(port + MCD_DATA, MCD_REQUEST_VERSION);
	DELAY(1000);

	/* wait up to .1 sec for first reply byte */
	for (i = 0; i < 10; i++) {
		if (mcd_get_flags(port) & MCD_STATUS_AVAIL)
			break;
		DELAY(10000);
	}

	for (i = 0; i < 3; i++) {
		if (mcd_busy_wait_for_status(port) < 0) {
			if (autodebug)
				printf("mcd %x: only got %d status bytes\n",
				       port, i);
			goto retry;
		}
		results[i] = inb(port + MCD_DATA);
	}

	if (autodebug)
		printf("mcd %x: version data %x %x %x\n", port, 
			results[0], results[1], results[2]);

	if (results[0] == 0xff) {
		if (autodebug)
			printf("mcd %x: command not accepted\n");
		goto retry;
	}

	*letterp = results[1];
	*numberp = results[2];

	return (0);
}

int
mcd_drive_type(int letter, int number)
{
	/*
	 * M02 -> LU002, 8 bit controller
	 * M04 -> LU005, 16 bit controller
	 * D02 -> FX001, 16 bit controller, double speed drive
	 */
	if (letter == 'M' && number == 0x02)
		return (0);
	else
		return (1);
}


/* ARGSUSED */
int
mcdprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	int status;
	int irq;
	int flags;
	int i;
	int letter, number;
	int c;
	int drive_type;

	outb(ia->ia_iobase + MCD_FLAGS, 0); /* reset controller */
	DELAY(500000);

	for (i = 0; i < 10; i++)
		inb(ia->ia_iobase + MCD_DATA);

	if ((flags = mcd_get_flags(ia->ia_iobase)) & MCD_STATUS_AVAIL) {
		if (autodebug)
			printf("mcd %x: can't make STATUS_AVAIL go low: %x\n",
				ia->ia_iobase, flags);
		return (0);
	}

	if (mcd_get_version(ia->ia_iobase, &letter, &number) < 0)
		return (0);

	/*
	 * Since get_version was successful, we know that MCD_STATUS_AVAIL
	 * has started off, then went on for three bytes.  If it is
	 * off again now, then we assume we're really looking at a
	 * Mitsumi drive, instead of some other card.  Just in case a
	 * future version of the drive returns more data, try to soak
	 * it up before checking the STATUS_AVAIL bit.
	 */

	for (i = 0; i < 10; i++)
		inb(ia->ia_iobase + MCD_DATA);

	if ((mcd_get_flags(ia->ia_iobase) & MCD_STATUS_AVAIL) != 0) {
		if (autodebug)
			printf("mcd %x: STATUS_AVAIL stuck on %x\n", flags);
		return (0);
	}

	drive_type = mcd_drive_type(letter, number);

	/* 
	 * Unfortunately, the controller will not interrupt unless
	 * a disk is loaded.  Since we can't depend on this 
	 * at boot time, we have to use the compiled-in irq.
	 * We can also run with no hardware interrupts at all (on
	 * a fast machine).
	 */

	if (ia->ia_irq == IRQUNK)
		ia->ia_irq = 0;

	if (ia->ia_drq < 0 || ia->ia_drq >= 8) {
		switch (drive_type) {
		case 0:
			ia->ia_drq = 3;
			break;
		case 1:
			ia->ia_drq = 6;
			break;
		}
	}

	/*
	 * mcd_tradeoff:
	 *
	 * 1: no dma, no driver level read ahead
	 *    Safest choice for fast machines.  Gets full speed out
	 *    of LU005 and FX001, but is a factor of 10 slower on 
	 *    the LU002.
	 * 2: no dma, 32kbyte read ahead
	 *    Makes the LU002 usable on fast machines, however,
	 *    it seems that the drive is more likely to return bad data.
	 * 3: dma
	 *    Only safe choice for slow machines (otherwise the drive
	 *    returns bad data, but no error bits, about once every
	 *    100 megabytes).  However, dma doesn't seem to work
	 *    on all machines.
	 *
	 * It's hard to say what the boundary between slow and fast is.
	 * A 25 MHz 386 seems to fall into the slow catagory, while a
	 * 33 MHz 486 is fast, for this purpose.
	 */
	mcd_tradeoff = cf->cf_flags;

	if (mcd_tradeoff == 0) {
		if (drive_type == 0)
			/* LU002 needs dma to be usable */
			mcd_tradeoff = 3; 
		else
			/* LU005 and FX001 can usually use non-dma transfers */
			mcd_tradeoff = 1; 
	}

	if (autodebug) {
		while (1) {
			printf("Choose Mitsumi data transfer option ");
			printf("(see the release notes for details).\n");
			printf("(1..3) [%d]? ", mcd_tradeoff);

			if ((c = getchar ()) == '\n')
				break;

			printf("%c\n", c);

			if (c >= '1' && c <= '3') {
				mcd_tradeoff = c - '0';
				break;
			}
		}
	}

	if (mcd_tradeoff != 3)
		ia->ia_drq = 0;

	ia->ia_iosize = MCD_NPORT;
	return (1);
}

/* ARGSUSED */
void
mcdattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct isa_attach_args *ia = (struct isa_attach_args *) aux;
	struct mcd_softc *mcd = (struct mcd_softc *) self;
	int off;
	int i, j;

	if (mcd_get_version(ia->ia_iobase,
			     &mcd->ver_letter, &mcd->ver_number) < 0) {
		printf("[error getting version in attach] ");
		mcd->ver_letter = 'M';
		mcd->ver_number = 0x04;
	}

	if (mcd->ver_letter < ' ' || mcd->ver_letter >= 0177)
		mcd->ver_letter = '?';
	printf(" firmware version %c%x transfer option %d\n",
	       mcd->ver_letter, mcd->ver_number, mcd_tradeoff);

	mcd->drive_type = mcd_drive_type(mcd->ver_letter, mcd->ver_number);

	if (mcd_tradeoff == 1)
		mcd->blkbuf_size = 2048;

	mcd->iobase = ia->ia_iobase;

	if (ia->ia_drq) {
		int bad_drq;

		bad_drq = 0;
		if (mcd->drive_type == 0) {
			if (ia->ia_drq != 1 && ia->ia_drq != 3)
				bad_drq = 1;
		} else {
			if (ia->ia_drq < 5 || ia->ia_drq > 7)
				bad_drq = 1;
		}
		
		if (bad_drq)
			printf("mcd%d: warning: illegal drq: %d\n",
				mcd->sc_dev.dv_unit, ia->ia_drq);

		mcd->dmachan = ia->ia_drq;
		mcd->use_dma = 1;
		at_setup_dmachan(mcd->dmachan, MCD_BSIZE);
	} else if (mcd->drive_type == 0) {
		printf("mcd%d: dma disabled - ", mcd->sc_dev.dv_unit);
		printf("beware of undetected read errors on slow machines\n");
	}

	isa_establish(&mcd->sc_id, &mcd->sc_dev);
	
	mcd->chn = mcd->dmachan;

	if (ia->ia_irq) {
		switch (ffs(ia->ia_irq) - 1) {
		case 2: mcd->chn |= 0x10; break;
		case 9: mcd->chn |= 0x10; break;
		case 3: mcd->chn |= 0x20; break;
		case 5: mcd->chn |= 0x30; break;
		case 10: mcd->chn |= 0x40; break;
		case 11: mcd->chn |= 0x50; break;
		case 12: mcd->chn |= 0x60; break;
		case 15: mcd->chn |= 0x70; break;
		default:
			printf("mcd%d: warning: illegal irq: %x\n",
				mcd->sc_dev.dv_unit, ia->ia_irq);
			break;
		}

		mcd->sc_ih.ih_fun = mcdintr;
		mcd->sc_ih.ih_arg = mcd;
		intr_establish(ia->ia_irq, &mcd->sc_ih, DV_DISK);
	}
}

int mcd_dma_timeout = 72; /* = 144 milliseconds */

/* ARGSUSED */
int
mcdopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	char args[10];
	char results[10];
	int unit;
	struct mcd_softc *mcd;
	int error = 0;
	int s;
	struct buf *bp;
	int i, j;

	if ((unit = mcd_unit(dev)) >= mcdcd.cd_ndevs ||
	    (mcd = mcdcd.cd_devs[unit]) == NULL)
		return (ENODEV);

	while (mcd->open_lock)
		if (error = tsleep((caddr_t)mcd, PZERO, "cdopen", 0))
			return (error);
		
	if (mcd->opened) {
		mcd->opened |= MCD_OPEN_MODE_FLAG(mode);
		return (0);
	}

	mcd->open_lock = 1;
			
	s = splsoftclock();
	mcd_timeout_needed = 1;
	if (mcd_timeout_running == 0) {
		mcd_timeout_running = 1;
		timeout(mcdtimeout, NULL, 1);
	}
	splx(s);

	/*
	 * The probe routine resets the drive, and that causes it to
	 * go through about a 7 second spin up sequence.  So, we might
	 * need to wait here before we try the first disk op.  This
	 * will probably also kick in if someone puts a disk in the 
	 * drive, then immediately tries to access it.
	 */
	for (i = 0; i < 10; i++) {
		args[0] = MCD_REQUEST_STATUS;
		if (error = mcdcmd(mcd, args, 1, results, 1, UIO_SYSSPACE))
			goto done;

		/* check for disk present */
		if (results[0] & 0x40)
			break;

		if (i == 0) {
			printf("mcd%d: waiting up to 10 seconds for spin up\n",
			       mcd->sc_dev.dv_unit);
		}

		timeout(wakeup, mcd, hz);
		if (error = tsleep((caddr_t)mcd, PZERO|PCATCH, "cdopen", 0))
			goto done;
	}

	if ((results[0] & 0x40) == 0) {
		printf("mcd%d: no disk in drive\n", mcd->sc_dev.dv_unit);
		error = EIO;
		goto done;
	}

	if (mcd->use_dma && mcd->drive_type == 1) {
		/* new 16 bit dma controller */
		outb(mcd->iobase + MCD_CHN, mcd->chn);
		DELAY(100);
		outb(mcd->iobase + MCD_HCON, 8);
		DELAY(100);
	}

	if (mcd->use_dma) {
		args[0] = MCD_DRIVE_CONFIGURATION;
		args[1] = 8;
		args[2] = mcd_dma_timeout; /* dma timeout in millisec */
		if (error = mcdcmd(mcd, args, 3, results, 1, UIO_SYSSPACE))
			goto done;

		args[0] = MCD_DRIVE_CONFIGURATION;
		args[1] = 2;
		args[2] = 1; /* dma mode */
		if (error = mcdcmd(mcd, args, 3, results, 1, UIO_SYSSPACE))
			goto done;
	} else {
		args[0] = MCD_DRIVE_CONFIGURATION;
		args[1] = 2;
		args[2] = 0; /* non dma mode */
		if (error = mcdcmd(mcd, args, 3, results, 1, UIO_SYSSPACE))
			goto done;
	}

	/* enable interrupts */
	args[0] = MCD_DRIVE_CONFIGURATION;
	args[1] = 0x10;
	args[2] = 7;
	if (error = mcdcmd(mcd, args, 3, results, 1, UIO_SYSSPACE))
		goto done;

	args[0] = MCD_REQUEST_TOC_DATA;
	if (error = mcdcmd(mcd, args, 1, results, 9, UIO_SYSSPACE))	
		goto done;

	mcd->start_cdblk = decode_bcd(results[6]) * BLOCKS_PER_MINUTE +
	    decode_bcd(results[7]) * BLOCKS_PER_SECOND +
	    decode_bcd(results[8]);

	mcd->end_cdblk = decode_bcd(results[3]) * BLOCKS_PER_MINUTE +
	    decode_bcd(results[4]) * BLOCKS_PER_SECOND + decode_bcd(results[5]);
	
	mcd->nblks = (mcd->end_cdblk - mcd->start_cdblk) *
	    (MCD_BSIZE / DEV_BSIZE);

	mcd->disk_changed = 0;
	mcd->blkbuf_valid = 0;

	if (mcd->blkbuf_size == 0)
		mcd->blkbuf_size = MCD_BLKBUF_DEFAULT_SIZE;

	if (mcd->blkbuf == NULL)
		MALLOC(mcd->blkbuf, char *, mcd->blkbuf_size,
		       M_DEVBUF, M_WAITOK);

done:
	if (error == 0 && mcd->use_dma && mcd->drive_type == 1 &&
	    (results[0] & MCD_STATUS_DISK_TYPE) == 0) {
		/*
		 * The first DMA from the drive always returns trash
		 * in the first byte, so if this is a data disk, we 
		 * have to do a read here before the user's first read
		 */
		bp = geteblk(MCD_BSIZE);
		bp->b_blkno = 0;
		bp->b_dev =  dev;
		bp->b_bcount = MCD_BSIZE;
		bp->b_flags = B_BUSY | B_READ;
		mcdstrategy(bp);
		error = biowait(bp);
		bp->b_flags = B_INVAL | B_AGE;
		brelse(bp);
	}

	if (error == 0)
		mcd->opened |= MCD_OPEN_MODE_FLAG(mode);

	mcd->open_lock = 0;
	wakeup((caddr_t)mcd);

	return (error);
}

/* ARGSUSED */
int
mcdclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int unit;
	struct mcd_softc *mcd;

	if ((unit = mcd_unit(dev)) >= mcdcd.cd_ndevs ||
	    (mcd = mcdcd.cd_devs[unit]) == NULL)
		return (ENODEV);

	mcd->opened &= ~MCD_OPEN_MODE_FLAG(mode);

	if (mcd->opened == 0) {
		if (mcd->blkbuf) {
			FREE(mcd->blkbuf, M_DEVBUF);
			mcd->blkbuf = NULL;
		}

		outb(mcd->iobase + MCD_FLAGS, 0); /* reset controller */

		for (unit = 0; unit < mcdcd.cd_ndevs; unit++)
			if ((mcd = mcdcd.cd_devs[unit]) != NULL && mcd->opened)
				break;

		if (unit == mcdcd.cd_ndevs)
			mcd_timeout_needed = 0;
	}

	return (0);
}

/* ARGSUSED */
int
mcdioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	void *addr;
	int flag;
{
	int unit;
	struct mcd_softc *mcd;
	int error = 0;
	int size;
	struct mcd_cmd *cmdp;

	if ((unit = mcd_unit(dev)) >= mcdcd.cd_ndevs ||
	    (mcd = mcdcd.cd_devs[unit]) == NULL)
		return (ENODEV);

	switch (cmd) {
	case MCDIOCSETBUF:
		size = *(int *)addr;

		if (size <= 0 || size > 256 * 1024 || (size % MCD_BSIZE) != 0) {
			error = EINVAL;
			break;
		}

		FREE(mcd->blkbuf, M_DEVBUF);

		mcd->blkbuf_size = size;
		MALLOC(mcd->blkbuf, char *, mcd->blkbuf_size,
		       M_DEVBUF, M_WAITOK);

		mcd->blkbuf_valid = 0;
		break;
		
	case MCDIOCCMD:
		cmdp = (struct mcd_cmd *)addr;
		if (error = mcdcmd(mcd, cmdp->args, cmdp->nargs,
		    cmdp->results, cmdp->nresults, UIO_USERSPACE))
			return (error);
		break;
		
	default:
		error = ENOTTY;
		break;
	}

	return (error);
}

static int
mcdcmd(mcd, args, nargs, results, nresults, results_seg)
	struct mcd_softc *mcd;
	char *args;
	int nargs;
	char *results;
	int nresults;
	enum uio_seg results_seg;
{
	struct buf *bp;
	int error;

	if (nargs < 0 || nargs >= sizeof mcd->args ||
	    nresults < 0 || nresults >= sizeof mcd->results)
		return (EINVAL);

	bp = &mcd->cmdbuf;
	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANTED;
		if (error = tsleep((caddr_t)bp, PZERO, "cdrom cmd", 0))
			return (error);
	}
	bp->b_flags = B_BUSY | B_READ;
	bp->b_error = 0;

	bcopy(args, mcd->args, nargs);
	mcd->nargs = nargs;
	mcd->nresults = nresults;
	mcdstrategy(bp);
	error = biowait(bp);

	if (results_seg == UIO_SYSSPACE)
		bcopy(mcd->results, results, nresults);
	else
		error = copyout(mcd->results, results, nresults);

	bp->b_flags &= ~B_BUSY;
	if (bp->b_flags & B_WANTED) {
		bp->b_flags &= ~B_WANTED;
		wakeup((caddr_t)bp);
	}
	return (error);
}

int
mcdstrategy(bp)
	struct buf *bp;
{
	int s;
	int unit;
	struct mcd_softc *mcd;

	if ((bp->b_flags & B_READ) == 0 ||
	    (unit = mcd_unit(bp->b_dev)) >= mcdcd.cd_ndevs ||
	    (mcd = mcdcd.cd_devs[unit]) == NULL) {
		bp->b_flags |= B_ERROR;
		biodone(bp);
		return (0);
	}

	bp->b_cylin = bp->b_blkno;
	s = splbio();
	disksort(&mcd->requests, bp);
	if (mcd->state == MCD_IDLE)
		mcdintr(mcd);
	splx(s);
	return (0);
}

static void
mcd_output_command(mcd, buf, n)
	struct mcd_softc *mcd;
	char *buf;
	int n;
{
	int i;
	
	for (i = 0; i < n; i++) {
		outb(mcd->iobase + MCD_DATA, buf[i]);
#ifdef MCD_BUS_TOO_FAST
		/*
		 * 0x80 is the port where the bios checkpoint codes
		 * are written during the power on self test.  It should
		 * be safe to read on any machine.
		 */
		inb(0x80);
#endif
	}

	mcd->start_time = time.tv_sec;
}

void
mcdtimeout(cookie)
	void *cookie;
{
	int s;
	int i;
	struct mcd_softc *mcd;

	s = splbio();
	for (i = 0; i < mcdcd.cd_ndevs; i++)
		if ((mcd = mcdcd.cd_devs[i]) != NULL)
			mcdintr(mcd);
	splx(s);

	if (mcd_timeout_needed)
		timeout(mcdtimeout, NULL, 1);
	else
		mcd_timeout_running = 0;
}

int
mcd_read_status(mcd)
	struct mcd_softc *mcd;
{
	int status;

	status = inb(mcd->iobase + MCD_DATA);

	if (status & MCD_STATUS_DISK_CHANGED)
		mcd->disk_changed = 1;

	return (status);
}

int
mcdintr(mcd)
	struct mcd_softc *mcd;
{
	int i, j;
	struct buf *bp;
	int flags;
	int status = -1;
	int offset_blks;
	int nbytes;
	int minutes, secs, block;
	char cmd[7];
	int expected = 1;
	int dma_resid;
	struct timeval tv;
	int delta;

top:
	if ((bp = mcd->requests.b_actf) == NULL)
		return (expected);

 reswitch:
	switch (mcd->state) {
	case MCD_IDLE:
		if (bp == &mcd->cmdbuf) {
			mcd_output_command(mcd, mcd->args, mcd->nargs);
			mcd->state = MCD_AWAIT_STATUS_AFTER_CMD;
			break;
		}

		mcd->addr = bp->b_un.b_addr;
		mcd->resid = bp->b_bcount;
		mcd->blkno = bp->b_blkno;

	read_more:
		if (mcd->disk_changed || mcd->blkno < 0 ||
		    mcd->blkno >= mcd->nblks) {
			if (mcd->blkno == mcd->nblks)
				/* return EOF */
				goto done;
			goto bad;
		}

		if (mcd->blkbuf_valid && mcd->blkno >= mcd->blkbuf_blkno &&
		    mcd->blkno < mcd->blkbuf_blkno + mcd->blkbuf_valid) {
			offset_blks = mcd->blkno - mcd->blkbuf_blkno;
			nbytes = (mcd->blkbuf_valid - offset_blks) * DEV_BSIZE;

			if (nbytes > mcd->resid)
				nbytes = mcd->resid;

			bcopy(mcd->blkbuf + offset_blks * DEV_BSIZE,
			       mcd->addr, nbytes);

			mcd->resid -= nbytes;
			mcd->addr += nbytes;
			mcd->blkno += nbytes / DEV_BSIZE;
			if (mcd->resid <= 0)
				goto done;

			goto read_more;
		}

		if (mcd->use_dma && mcd->drive_type == 1) {
			/* 
			 * This is another piece of magic extracted from
			 * the dos driver.
			 */
			outb(mcd->iobase + MCD_CHN, mcd->chn & 0xf0);
			DELAY(1000);
			outb(mcd->iobase + MCD_CHN, mcd->chn);
		}

		if (mcd->use_dma == 0 &&
		    (mcd->blkno % (MCD_BSIZE / DEV_BSIZE)) == 0 &&
		    mcd->resid >= mcd->blkbuf_size) {
			mcd->use_blkbuf = 0;
			mcd->chunk_ncdblks = mcd->resid / MCD_BSIZE;
			mcd->chunk_ptr = mcd->addr;
		} else {
			mcd->use_blkbuf = 1;
			mcd->blkbuf_valid = 0;
			mcd->chunk_ncdblks = mcd->blkbuf_size / MCD_BSIZE;
			mcd->chunk_ptr = mcd->blkbuf;

		}

		mcd->cdblk = mcd->blkno / (MCD_BSIZE / DEV_BSIZE);
		mcd->chunk_blkno = mcd->cdblk * (MCD_BSIZE / DEV_BSIZE);

		mcd->cdblk += mcd->start_cdblk;
		if (mcd->cdblk + mcd->chunk_ncdblks > mcd->end_cdblk)
			mcd->chunk_ncdblks = mcd->end_cdblk - mcd->cdblk;
		mcd->chunk_done = 0;

		mcd->command_retry_count = 0;
retry_read_command:
		if (mcddebug > 1)
			printf("mcd: read %d\n", mcd->blkno);

		minutes = mcd->cdblk / BLOCKS_PER_MINUTE;
		secs = (mcd->cdblk / BLOCKS_PER_SECOND) % SECONDS_PER_MINUTE;
		block = mcd->cdblk % BLOCKS_PER_SECOND;
	
		cmd[0] = MCD_SEEK_AND_READ;
		/* block address */
		cmd[1] = encode_bcd(minutes);
		cmd[2] = encode_bcd(secs);
		cmd[3] = encode_bcd(block);
		/* number of blocks, most significant byte first */
		if (mcd->use_dma) {
			/* 
			 * I don't know that this means, but it is the
			 * primary piece of magic I discovered from the
			 * assembly driver.
			 */
			cmd[4] = 0xf0;
			cmd[5] = 0;
			cmd[6] = 0;
		} else {
			cmd[4] = mcd->chunk_ncdblks >> 16;
			cmd[5] = mcd->chunk_ncdblks >> 8;
			cmd[6] = mcd->chunk_ncdblks;
		}

		mcd_output_command(mcd, cmd, 7);
		
		if (mcd->use_dma == 0) {
			mcd->state = MCD_AWAIT_DATA;
			break;
		}

		if (mcd->drive_type == 0)
			mcd->state = MCD_CHECK_FOR_COMMAND_SENT;
		else
			mcd->state = MCD_CHECK_FOR_COMMAND_SENT_NEW;

		microtime(&tv);
		mcd->timer = tv.tv_sec * 1000 + tv.tv_usec / 1000;
		goto reswitch;

	case MCD_CHECK_FOR_COMMAND_SENT:
		/*
		 * Sometimes the controller ignores commands, so
		 * we have to look to see if it respsonds to the
		 * read command by dropping it's DMA request.
		 * If not, we will have to resend the command.
		 * We can't so anything similar for non-dma transfers,
		 * and that is one reason they are less reliable.
		 *
		 * Typically, the dma request will drop about 20-40ms
		 * after the command is sent.
		 */
		microtime(&tv);
		delta = (tv.tv_sec * 1000 + tv.tv_usec / 1000) - mcd->timer;

		if (mcd_get_flags(mcd->iobase) & MCD_STATUS_AVAIL)
			/* flush the preliminary status byte, if it arrives */
			mcd_read_status(mcd);

		if (MCD_DMA_PEND(mcd) == 0) {
			static int maxdelta;
			if (mcddebug && delta > maxdelta) {
				maxdelta = delta;
				printf("mcd: command sent delta %d %d\n",
					delta, mcd->blkno);
			}
			/* 
			 * Now that the disk doesn't have a pending
			 * request, we can safely enable the dma channel.
			 */
			at_dma(1, mcd->blkbuf, MCD_BSIZE, mcd->dmachan);
			mcd->state = MCD_AWAIT_TERMINAL_COUNT;
			break;
		}

		if (delta < 500)
			break;

		/* we've waited too long, try sending the command again */
		if (++mcd->command_retry_count < 5) {
			if (mcddebug)
				printf("mcd: retry read command %d %d\n",
					mcd->command_retry_count, mcd->blkno);
			goto retry_read_command;
		}

		printf("mcd%d: dma req stuck %d\n",
			mcd->sc_dev.dv_unit, mcd->blkno);
		goto bad;

	case MCD_CHECK_FOR_COMMAND_SENT_NEW:
		microtime(&tv);
		delta = (tv.tv_sec * 1000 + tv.tv_usec / 1000) - mcd->timer;

		if (mcd_get_flags(mcd->iobase) & MCD_STATUS_AVAIL) {
			/* flush the preliminary status byte */
			mcd_read_status(mcd);

			/*
			 * I hate having such a long delay here, but the
			 * only alternative would be to add a state and
			 * stay in it until the system clock advances
			 * by 10 milliseconds.  But since it takes 10
			 * milliseconds per tick, we'd have to wait for
			 * two ticks.  I'm not sure if the drive will
			 * be happy waiting up to 20 milliseconds,
			 * and I know it will silently give bad data 
			 * if we don't wait long enough.  Luckily, this
			 * delay doesn't happen very often:  The disk
			 * transfer rate is 150 kbytes/sec, and we do
			 * this once per blkbuf = 32kbytes.  So, we are
			 * here at most 5 times per second.  In practice,
			 * rotational delays and other drive overhead
			 * reduce this to 2 or 3 times per second.
			 */
			DELAY(10000);

			at_dma(1, mcd->blkbuf, MCD_BSIZE, mcd->dmachan);
			mcd->state = MCD_AWAIT_TERMINAL_COUNT;
			break;
		}

		if (delta < 1200)
			break;

		/* we've waited too long, try sending the command again */
		if (++mcd->command_retry_count < 5) {
			if (mcddebug)
				printf("mcd: retry read command %d %d\n",
					mcd->command_retry_count, mcd->blkno);
			goto retry_read_command;
		}

		printf("mcd%d: never got status after read command\n",
			mcd->sc_dev.dv_unit);
		goto bad;

	case MCD_AWAIT_DATA:
		flags = mcd_get_flags(mcd->iobase);

		if (flags & MCD_STATUS_AVAIL) {
			expected = 1;
			/*
			 * Premature status means an error happened.
			 * Pick up possible disk change bit.
			 */
			status = mcd_read_status(mcd); 
			printf("mcd%d: premature status %x %x %d\n",
				mcd->sc_dev.dv_unit, flags, status,
			        mcd->blkno);
			goto retry_block;
		}

		if ((flags & MCD_DATA_AVAIL) == 0)
			break;

		expected = 1;

		insb(mcd->iobase + MCD_DATA, mcd->chunk_ptr, MCD_BSIZE - 1);

		if ((mcd_get_flags(mcd->iobase) & MCD_DATA_AVAIL) == 0) {
			printf("mcd%d: not enough data %d\n",
			    mcd->sc_dev.dv_unit, mcd->blkno);
			goto retry_block;
		}

		mcd->chunk_ptr[MCD_BSIZE - 1] = inb(mcd->iobase + MCD_DATA);

		mcd->chunk_ptr += MCD_BSIZE;

		mcd->chunk_done++;
		if (mcd->chunk_done < mcd->chunk_ncdblks)
			/* wait for more data */
			break;

		mcd->state = MCD_AWAIT_STATUS_AFTER_DATA;
		break;

	case MCD_AWAIT_STATUS_AFTER_DATA:
		flags = mcd_get_flags(mcd->iobase);

		if ((flags & MCD_STATUS_AVAIL) == 0)
			break;

		expected = 1;

		status = mcd_read_status(mcd);

		if (status & (MCD_STATUS_READ_ERROR|MCD_STATUS_COMMAND_CHECK))
			goto bad;

		if (mcd->use_blkbuf) {
			/* 
			 * Mark the blkbuf valid.  Will copy the new data
			 * to the buffer and update the transfer variables
			 * when we loop around and discover the current
			 * block is in the buffer.
			 */
			mcd->blkbuf_blkno = mcd->chunk_blkno;
			mcd->blkbuf_valid = mcd->chunk_ncdblks *
			    (MCD_BSIZE / DEV_BSIZE);
		} else {
			/*
			 * Not using blkbuf, so the data was read
			 * to the correct place already.
			 */
			mcd->addr += mcd->chunk_ncdblks * MCD_BSIZE;
			mcd->resid -= mcd->chunk_ncdblks * MCD_BSIZE;
			mcd->blkno += mcd->chunk_ncdblks*(MCD_BSIZE/DEV_BSIZE);
			if (mcd->resid <= 0)
				goto done;
		}
		
		goto read_more;

	case MCD_AWAIT_TERMINAL_COUNT:
		if (MCD_DMA_TERM(mcd) == 0)
			break;
		expected = 1;

		at_dma_terminate(mcd->dmachan);

		mcd->chunk_done++;
		if (mcd->chunk_done < mcd->chunk_ncdblks) {
			at_dma(1, mcd->blkbuf + mcd->chunk_done * MCD_BSIZE,
				MCD_BSIZE, mcd->dmachan);
			break;
		}

		outb(mcd->iobase + MCD_DATA, MCD_HOLD);

		mcd->state = MCD_AWAIT_STATUS_AFTER_TERMINAL_COUNT;
		microtime(&tv);
		mcd->timer = tv.tv_sec * 1000 + tv.tv_usec / 1000;
		/* fall in */

	case MCD_AWAIT_STATUS_AFTER_TERMINAL_COUNT:
		microtime(&tv);
		delta = (tv.tv_sec * 1000 + tv.tv_usec / 1000) - mcd->timer;

		/*
		 * Typically, the status arrives about 90 to 100 ms after
		 * the dma is finished.
		 */
		flags = mcd_get_flags(mcd->iobase);
		if (flags & MCD_STATUS_AVAIL) {
			static int maxdelta;
			status = mcd_read_status(mcd);
			if (status & (MCD_STATUS_READ_ERROR
				      | MCD_STATUS_COMMAND_CHECK)) {
				printf("mcd%d: bad status after read %x %d\n",
					status, mcd->blkno);
				goto retry_block;
			}

			if (mcddebug && delta > maxdelta) {
				maxdelta = delta;
				printf("mcd: status after tc %d %d\n",
					delta, mcd->blkno);
			}
		} else {
			if (delta < 200)
				break;
			if (mcddebug)
				printf("mcd: no status %d\n", mcd->blkno);
		}

		/* 
		 * Mark the blkbuf valid.  Will copy the new data
		 * to the buffer and update the transfer variables
		 * when we loop around and discover the current
		 * block is in the buffer.
		 */
		mcd->blkbuf_blkno = mcd->chunk_blkno;
		mcd->blkbuf_valid = mcd->chunk_ncdblks *
			(MCD_BSIZE / DEV_BSIZE);

		goto read_more;

	case MCD_AWAIT_STATUS_AFTER_CMD:
		flags = mcd_get_flags(mcd->iobase);

		if (flags & MCD_DATA_AVAIL) {
			printf("mcd%d: unexpected data %x\n",
				mcd->sc_dev.dv_unit, flags);
			goto retry_block;
		}

		if ((flags & MCD_STATUS_AVAIL) == 0)
			break;

		expected = 1;
		status = mcd_read_status(mcd);
		if (status & MCD_STATUS_COMMAND_CHECK) {
			printf("mcd%d: command error %x\n",
				mcd->sc_dev.dv_unit, status);
			goto retry_block;
		}
		
		mcd->results[0] = status;
		for (i = 1; i < mcd->nresults; i++) {
			for (j = 0; j < 5; j++) {
				DELAY(10);
				if (mcd_get_flags(mcd->iobase)&MCD_STATUS_AVAIL)
					break;
			}
			if (j == 5) {
				printf("mcd%d: not enough status bytes: %d\n",
				       mcd->sc_dev.dv_unit, i);
				goto retry_block;
			}
			mcd->results[i] = inb(mcd->iobase + MCD_DATA);
		}
		goto done;

	default:
		printf("mcd%d: illegal state\n", mcd->sc_dev.dv_unit);
		goto bad;
	}

	if (time.tv_sec - mcd->start_time > MCD_TIMEOUT) {
		printf("mcd%d: timeout: state %d blkno %d\n",
		       mcd->sc_dev.dv_unit, mcd->state, mcd->blkno);
		goto retry_block;
	}
	return (expected);

retry_block:
	if (mcd->block_retry_count++ < 5) {
		printf("mcd: retry %d\n", mcd->block_retry_count);
		mcd->state = MCD_IDLE;
		goto top;
	}

bad:
	mcd->blkbuf_valid = 0;
	bp->b_flags |= B_ERROR;
	bp->b_error = EIO;

	if (bp != &mcd->cmdbuf) {
		printf("mcd%d: block %d: read error",
			mcd->sc_dev.dv_unit, mcd->blkno);
		if (status != -1)
			printf(" %x", status);
		printf("\n");
	}

done:
	bp->b_resid = mcd->resid;
	mcd->requests.b_actf = bp->av_forw;
	biodone(bp);

	mcd->state = MCD_IDLE;
	mcd->block_retry_count = 0;
	goto top;
}

/* ARGSUSED */
mcddump(dev)
	dev_t dev;
{

	return (EIO);
}

int
mcdsize(dev)
	dev_t dev;
{

	return (-1);
}
