/*-
 * Copyright (c) 1992, 1993, 1994 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: fd.c,v 1.8 1994/01/05 20:18:04 karels Exp $
 */

/*
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
 *	@(#)fd.c	7.3 (Berkeley) 5/25/91
 */

/****************************************************************************/
/*                        standalone fd driver                               */
/****************************************************************************/
#include "param.h"
#include "dkbad.h"
#include "i386/isa/fdreg.h"
#include "i386/isa/isa.h"
#include "saio.h"

#define NUMRETRY 10

#define NFD 2
#define FDBLK 512
#define NUMTYPES 4

struct fd_type {
	int	sectrac;		/* sectors per track         */
	int	secsize;		/* size code for sectors     */
	int	datalen;		/* data len when secsize = 0 */
	int	gap;			/* gap len between sectors   */
	int	tracks;			/* total num of tracks       */
	int	size;			/* size of disk in sectors   */
	int	steptrac;		/* steps per cylinder        */
	int	trans;			/* transfer speed code       */
};

struct fd_type fd_types[NUMTYPES] = {
 	{ 18,2,0xFF,0x1B,80,2880,1,0 },	/* 1.44 meg HD 3.5in floppy    */
	{ 15,2,0xFF,0x1B,80,2400,1,0 },	/* 1.2 meg HD floppy           */
#ifndef SMALL
	{ 9,2,0xFF,0x23,40,720,2,1 },	/* 360k floppy in 1.2meg drive */
	{ 9,2,0xFF,0x2A,40,720,1,1 },	/* 360k floppy in DD drive     */
#endif
};


/* state needed for current transfer */
static int fd_type;
static int fd_status[7];

static int fdc = IO_FD1;	/* floppy disk base */
static int fd_curtrack[NFD];	/* current cylinder/track */

/* Make sure DMA buffer doesn't cross 64k boundary */
char bounce[FDBLK];

#ifndef SMALL
#define FDDEBUG
extern int bootdebug;
#define	dprintf(x)	if (bootdebug) printf x
#define	dprintf1(x)	if (bootdebug > 1) printf x
#else
#define	dprintf(x)
#define	dprintf1(x)
#endif

/****************************************************************************/
/*                               fdstrategy                                 */
/****************************************************************************/
int
fdstrategy(io,func)
register struct iob *io;
int func;
{
	char *address;
	long nblocks,blknum;
 	int unit, iosize;

#ifdef SMALL
	printf(".");			/* print some warm fuzzies/debugging */
#else
	dprintf1(("fdstrat "));
#endif
	unit = io->i_unit;
	fd_type = io->i_adapt;		/* XXX */

	/*
	 * Set up block calculations.
	 */
        iosize = io->i_cc / FDBLK;
	blknum = (unsigned long) io->i_bn * DEV_BSIZE / FDBLK;
#ifndef SMALL
 	nblocks = fd_types[fd_type].size;
/*printf("fdstrat %d@%d ", nblocks, blknum);*/
	if ((blknum + iosize > nblocks) || blknum < 0) {
		printf("bn = %d; sectors = %d; type = %d; fssize = %d ",
			blknum, iosize, fd_type, nblocks);
                printf("fdstrategy - I/O out of filesystem boundaries\n");
		return(-1);
	}
#endif

	address = io->i_ma;
        while (iosize > 0) {
                if (fdio(func, unit, blknum, address, io))
                        return(-1);
		iosize--;
		blknum++;
                address += FDBLK;
        }
        return (io->i_cc);
}

int
fdio(func, unit, blknum, address, io)
	int func, unit, blknum;
	char *address;
	struct iob *io;
{
	int i, j, cyl, sectrac, sec, head, numretry;
	struct fd_type *ft;

 	ft = &fd_types[fd_type];
	dprintf1(("| %d ", blknum));

 	sectrac = ft->sectrac;
	cyl = blknum / (2 * sectrac);
	numretry = NUMRETRY;

	if (func == F_WRITE)
		bcopy(address, bounce, FDBLK);

retry:
	if (numretry == NUMRETRY / 2) {
		dprintf(("Reset "));
		fdopen(io);	/* reset and ready to retry */
	}
	dprintf1(("s"));
	if (((cyl << 1) | head) != fd_curtrack[unit]) {
		out_fdc(NE7CMD_SEEK);
		out_fdc(unit | (head << 2));	/* Drive number */
		out_fdc(cyl);

		waitio();

		out_fdc(NE7CMD_SENSEI);
		i = in_fdc();
		j = in_fdc();
		if (!(i&NE7_ST0_SEEK_COMPLETE) || (cyl != j)) {
			dprintf(("seek err %x %d %d ", i, j, cyl));
			if (--numretry > 0)
				goto retry;
			printf("Seek error: status 0x%x, cyl %d should be %d, blk %d\n",
			    i, j, cyl, blknum);
			fd_curtrack[unit] = -1;
			return (-1);
		}
		fd_curtrack[unit] = (cyl << 1) | head;
	}

	dprintf1(("r"));
	/* set up transfer */
	fd_dma(func == F_READ, bounce, FDBLK);
	sec = blknum %  (sectrac * 2);
	head = sec / sectrac;
	sec = sec % sectrac + 1;	/* origin 1 */

	if (func == F_READ)
		out_fdc(NE7_READ);
	else
		out_fdc(NE7_WRITE);
	out_fdc(head << 2 | unit);	/* head & unit */
	out_fdc(cyl);			/* track */
	out_fdc(head);
	out_fdc(sec);			/* sector */
	out_fdc(ft->secsize);		/* sector size */
	out_fdc(sectrac);		/* sectors/track */
	out_fdc(ft->gap);		/* gap size */
	out_fdc(ft->datalen);		/* data length */

	dprintf1(("W"));
#ifdef sometime_fails
	/*
	 * On one machine, waitio hangs here; however,
	 * the replacement code doesn't work after seeks.
	 */
	waitio();
#else
	/* wait for both RQM and DIO to set, and the unit busy bit to clear */
	while ((inb(fdc + fdsts) & (NE7_RQM|NE7_DIO|(1 << unit))) !=
	    (NE7_RQM|NE7_DIO))
		delay(50);
	delay(50);
	outb(0x20,0x20);		/* EOI, just in case */
#endif

	for(i=0;i<7;i++) {
		fd_status[i] = in_fdc();
	}
	if (fd_status[0]&NE7_ST0_NOT_GOOD) {
		dprintf(("rd err %x %x %x ",
		    fd_status[0], fd_status[1], fd_status[2]));
		if (--numretry > 0)
			goto retry;
/* #ifndef SMALL */
		printf("FD err %x %x %x %x %x %x %x\n",
		fd_status[0], fd_status[1], fd_status[2], fd_status[3],
		fd_status[4], fd_status[5], fd_status[6] );
/* #endif */
		return (-1);
	}
	if (func == F_READ)
		bcopy(bounce, address, FDBLK);
	return (0);
}

/****************************************************************************/
/*                             fdc in/out                                   */
/****************************************************************************/
int
in_fdc()
{
	int i;

	while ((i = inb(fdc + fdsts) & (NE7_RQM|NE7_DIO)) != (NE7_RQM|NE7_DIO))
		if (i == NE7_RQM)
			return (-1);
	return (inb(fdc + fddata));
}

dump_stat()
{
	int i;
	for(i=0;i<7;i++) {
		fd_status[i] = in_fdc();
		if (fd_status[i] < 0) break;
	}
	printf("FD unexpected status: %x %x %x %x %x %x %x\n",
		fd_status[0], fd_status[1], fd_status[2], fd_status[3],
		fd_status[4], fd_status[5], fd_status[6] );
}

set_intr()
{
	/* initialize 8259's */
	outb(0x20,0x11);
	outb(0x21,32);
	outb(0x21,4);
	outb(0x21,1);
	outb(0x21,0xbf); /* turn on int 6 */

/*
	outb(0xa0,0x11);
	outb(0xa1,40);
	outb(0xa1,2);
	outb(0xa1,1);
	outb(0xa1,0xff); */

}



waitio()
{
	char c;

	dprintf1(("w"));
	do {
		delay(10);
		outb(0x20,0xc); /* read polled interrupt */
		delay(1);
	} while ((c=inb(0x20))&0x7f != 6); /* wait for int */
	delay(1);
	outb(0x20,0x20);
}

/* delay, roughly in microseconds; may be up to 3x that on slow machines. */
delay(n)
	int n;
{
	while (n-- > 0)
		asm("outb %al,$0x80");
}

out_fdc(x)
	int x;
{
	int r;

	do {
		r = (inb(fdc + fdsts) & (NE7_RQM|NE7_DIO));
		if (r == NE7_RQM)
			break;
		if (r == (NE7_RQM|NE7_DIO)) {
			dump_stat(); /* error: direction. eat up output */
		}
	} while (1);
	outb(fdc + fddata, x);
}


/****************************************************************************/
/*                           fdopen/fdclose                                 */
/****************************************************************************/
fdopen(io)
	register struct iob *io;
{
	int unit, type, i, out;
	struct fd_type *ft;

	unit = io->i_unit;
	type = io->i_adapt;		/* XXX */
	io->i_boff = 0;		/* no disklabels -- tar/dump wont work */
	dprintf1(("fdopen %d %d ", unit, type));
 	ft = &fd_types[type];

	set_intr(); /* init intr cont */

	/* Try a reset, keep motor on */
	outb(fdc + fdout, 0);
	delay(2000);
	out = unit | (unit ? FDO_MOEN1 : FDO_MOEN0);
	outb(fdc + fdout, out);
	delay(2000);
	outb(fdc + fdout, out | FDO_FRST | FDO_FDMAEN);
	outb(fdc + fdctl, ft->trans);

	waitio();

	out_fdc(NE7CMD_SPECIFY);
	out_fdc(0xDF);	/* step rate 0xd; head unload 0xf(*0x10) */
	out_fdc(2);	/* head load - 2msec; DMA */

	out_fdc(NE7CMD_RECAL);
	out_fdc(unit);

	waitio();

	/*
	 * For some reason, we get "No Data" errors on the next read
	 * on some machines, but if we repeat the recal, everything
	 * is fine.
	 */
	out_fdc(NE7CMD_RECAL);
	out_fdc(unit);
	waitio();

	fd_curtrack[unit] = -1;
	return (0);
}

/* ARGSUSED */
void
fdclose(io)
	struct iob *io;
{

	outb(fdc + fdout, FDO_FRST|FDO_FDMAEN);		/* motor off */
}

/****************************************************************************/
/*                                 fd_dma                                   */
/* set up DMA read/write operation and virtual address addr for nbytes      */
/****************************************************************************/
fd_dma(read, addr, nbytes)
	int read;
	unsigned long addr;
	int nbytes;
{

	/* Set read/write bytes */
	if (read) {
		outb(0xC, 0x46);
		outb(0xB, 0x46);
	} else {
		outb(0xC, 0x4A);
		outb(0xB, 0x4A);
	}
	/* Send start address */
	outb(0x4, addr & 0xFF);
	outb(0x4, (addr>>8) & 0xFF);
	outb(0x81, (addr>>16) & 0xFF);
	/* Send count */
	nbytes--;
	outb(0x5, nbytes & 0xFF);
	outb(0x5, (nbytes>>8) & 0xFF);
	/* set channel 2 */
	outb(0x0A, 2);
}

