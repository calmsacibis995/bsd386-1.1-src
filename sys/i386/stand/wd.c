/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: wd.c,v 1.10 1994/01/05 20:13:32 karels Exp $
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
 *	@(#)wd.c	7.3 (Berkeley) 5/4/91
 */

/*  device driver for winchester disk  */

#include "param.h"
#include "dkbad.h"
#include "disklabel.h"
#include "reboot.h"
#include "i386/isa/isa.h"
#include "i386/isa/wdreg.h"
#include "i386/include/bootblock.h"
#include "saio.h"

#define	NWD		4	/* number of hard disk units supported, max 2 */
#define	RETRIES		5	/* number of retries before giving up */

#define	WDUNIT(ctlr, unit)	((ctlr) * 2 + (unit))
int noretries, wdquiet;

/* #define WDDEBUG */
#ifdef WDDEBUG
#define	dprintf(x)	printf x
#else
#define	dprintf(x)	/* nothing */
#endif

#ifdef	SMALL
extern struct disklabel disklabel;
#else
struct disklabel wdsizes[NWD];
#endif
#ifdef BSDGEOM
extern struct bsdgeom *bsdgeom;
#endif

/*
 * Record for the bad block forwarding code.
 * This is initialized to be empty until the bad-sector table
 * is read from the disk.
 */
#define TRKSEC(trk,sec)	((trk << 8) + sec)

#define NDKBAD	(sizeof(((struct dkbad *)0)->bt_bad)/sizeof(struct bt_bad))
struct	dkbad	dkbad[NWD];

wdopen(io)
	register struct iob *io;
{
	register struct disklabel *dd;

	dprintf(("wdopen(%d, %d, %d) ", io->i_ctlr, io->i_unit, io->i_part));
#ifdef SMALL
	dd = &disklabel;
#ifdef BSDGEOM
	bsdgeom->unit = io->i_unit;
	bsdgeom->ncylinders = dd->d_ncylinders;
	bsdgeom->ntracks = dd->d_ntracks;
	bsdgeom->nsectors = dd->d_nsectors;
#endif
#else /* SMALL */
	if (io->i_part > 8)
		return (EPART);
	if (io->i_ctlr > 1)
		return (ECTLR);
	if (io->i_unit > 1)
		return (EUNIT);
	dd = &wdsizes[WDUNIT(io->i_ctlr, io->i_unit)];
#endif /* SMALL */
	if (wdinit(io))
		return (EIO);
	io->i_boff = dd->d_partitions[io->i_part].p_offset;
	dprintf(("boff %d\n", io->i_boff));
	return (0);
}

/* ARGSUSED */
void
wdclose(io)
	struct iob *io;
{

}

wdstrategy(io, func)
	register struct iob *io;
{
	register int iosize;    /* number of sectors to do IO for this loop */
	register daddr_t sector;
	int nblocks;
	int unit, partition;
	char *address;
	register struct disklabel *dd;

	unit = io->i_unit;
	partition = io->i_part;
	dprintf(("wdstrat %d %d ", unit, partition));
#ifdef	SMALL
	dd = &disklabel;
#else
	dd = &wdsizes[WDUNIT(io->i_ctlr, io->i_unit)];
#endif
	iosize = io->i_cc / dd->d_secsize;
	/*
	 * Convert PGSIZE "blocks" to sectors.
	 * Note: doing the conversions this way limits the partition size
	 * to about 8 million sectors (1-8 Gb).
	 */
	sector = (unsigned long) io->i_bn * DEV_BSIZE / dd->d_secsize;
#ifndef SMALL
	nblocks = dd->d_partitions[partition].p_size;
	if (iosize < 0 || sector + iosize > io->i_boff + nblocks ||
	    sector < io->i_boff) {
		dprintf(("bn = %d; sectors = %d; partition = %d; boff = %d; fssize = %d\n",
			io->i_bn, iosize, partition, io->i_boff, nblocks));
		printf("wdstrategy - I/O out of filesystem boundaries\n");
		return (-1);
	}
	if (io->i_bn * DEV_BSIZE % dd->d_secsize) {
		printf("wdstrategy - transfer starts in midsector\n");
		return (-1);
	}
	if (io->i_cc % dd->d_secsize) {
		printf("wd: transfer of partial sector\n");
		return (-1);
	}
#endif

	address = io->i_ma;
	while (iosize > 0) {
		if ((nblocks = wdio(func, io, sector, address, iosize)) == -1)
			return (-1);
		iosize -= nblocks;
		sector += nblocks;
		address += nblocks * dd->d_secsize;
	}
	return (io->i_cc);
}

/* 
 * Routine to do an I/O operation, and wait for it
 * to complete.
 */
wdio(func, io, blknm, addr, nblocks)
	int func;
	register struct iob *io;
	daddr_t blknm;
	short *addr;
	int nblocks;
{
	struct disklabel *dd;
	register wdc;
#ifndef SMALL
	struct bt_bad *bt_ptr, *bt_start;
	daddr_t eblk, bblk;
#endif
	int unit = WDUNIT(io->i_ctlr, io->i_unit);
	int i;
	int retries = 0;
	long cylin, head, sector;
	u_char opcode, erro;

#ifdef	SMALL
	dd = &disklabel;
#else
	dd = &wdsizes[unit];
#endif
	if (func == F_WRITE)
		opcode = WDCC_WRITE;
	else
		opcode = WDCC_READ;

	if (nblocks > 64)
		nblocks = 64;

#ifndef SMALL
	/* 
	 * See if the current block is in the bad block list.
	 */
	if (blknm > BBSIZE/DEV_BSIZE) {	/* should be BBSIZE */
	    eblk = blknm + nblocks;
	    bt_start = dkbad[unit].bt_bad;
	    for (bt_ptr = bt_start; bt_ptr < &bt_start[NDKBAD]; bt_ptr++) {
		bblk = (bt_ptr->bt_cyl * dd->d_ntracks +
			(bt_ptr->bt_trksec >> 8)) * dd->d_nsectors +
			(bt_ptr->bt_trksec & 0xff);
		if (bblk >= eblk)
			/* Sorted list, and we passed ending block. quit. */
			break;
		if (bblk < blknm)
			continue;
		if (bblk > blknm)
			nblocks = bblk - blknm;
		else  {
			/*
			 * Found bad block.  Calculate new block addr.
			 * and works backwards to the front of the disk.
			 */
			dprintf(("--- badblock code -> Old = %d; ", blknm));
			blknm = dd->d_secperunit - dd->d_nsectors -
			    (bt_ptr - bt_start) - 1;
			nblocks = 1;
			dprintf(("new = %d\n", blknm));
			break;
		}
	    }
	}
#endif

	/* Calculate data for output. */
	cylin = blknm / dd->d_secpercyl;
	head = (blknm % dd->d_secpercyl) / dd->d_nsectors;
	sector = blknm % dd->d_nsectors;
	sector += 1;
	wdc = io->i_ctlr ? IO_WD2 : IO_WD1;
retry:
	dprintf(("sec %d sdh %x cylin %d ", sector,
		WDSD_IBM | (io->i_unit << 4) | (head & 0xf), cylin));
	outb(wdc+wd_precomp, 0xff);
	outb(wdc+wd_seccnt, nblocks);
	outb(wdc+wd_sector, sector);
	outb(wdc+wd_cyl_lo, cylin);
	outb(wdc+wd_cyl_hi, cylin >> 8);

	/* Set up the SDH register (select drive).     */
	outb(wdc+wd_sdh, WDSD_IBM | (io->i_unit << 4) | (head & 0xf));
	while ((inb(wdc+wd_status) & WDCS_READY) == 0)
		;

	wait(1000);	/* hack for certain machines */
	outb(wdc+wd_command, opcode);
	for (i = 0; i < nblocks; i++) {
		while (opcode == WDCC_READ && (inb(wdc+wd_status) & WDCS_BUSY))
			;
		/* Did we get an error? */
		if (opcode == WDCC_READ && (inb(wdc+wd_status) & WDCS_ERR))
			goto error;

		/* Ready to remove data? */
		while ((inb(wdc+wd_status) & WDCS_DRQ) == 0)
			;

		if (opcode == WDCC_READ)
			insw(wdc+wd_data, addr, 256);
		else
			outsw(wdc+wd_data, addr, 256);

		/* Check data request (should be done). */
		if (inb(wdc+wd_status) & WDCS_DRQ)
			goto error;

		while (opcode == WDCC_WRITE && (inb(wdc+wd_status) & WDCS_BUSY))
			;
		wait(1000);	/* hack for certain machines */

		if (inb(wdc+wd_status) & WDCS_ERR)
			goto error;
		addr += 512 / sizeof(*addr);
	}

	dprintf(("+"));
	return (nblocks);
error:
	erro = inb(wdc+wd_error);
	if (++retries < RETRIES)
		goto retry;
	if (!wdquiet)
#ifdef	SMALL
	    printf("wd%d: hard error: sector %d status %x error %x\n", unit,
		blknm, inb(wdc+wd_status), erro);
#else
	    printf("wd%d: hard %s error: sector %d status %b error %b\n", unit,
		opcode == WDCC_READ? "read" : "write", blknm, 
		inb(wdc+wd_status), WDCS_BITS, erro, WDERR_BITS);
#endif
	return (-1);
}

wdinit(io)
	struct iob *io;
{
	register wdc;
	struct disklabel *dd;
	unsigned int unit;
#ifndef SMALL
	struct biosgeom *bgp;
	struct dkbad *db;
	extern struct biosgeom *biosgeom[2];
	static open[NWD];
#endif
	int i, errcnt = 0;
	char buf[512];

	unit = WDUNIT(io->i_ctlr, io->i_unit);
#ifndef SMALL
	if (open[unit])
		return (0);
#endif

	dprintf(("wdinit "));
	wdc = io->i_ctlr ? IO_WD2 : IO_WD1;

#ifdef	SMALL
	dd = &disklabel;
#else
	dd = &wdsizes[unit];
#endif
	/* reset controller */
	outb(wdc+wd_ctlr, 12); wait(10); outb(wdc+wd_ctlr, 8);
	wdwait(wdc);

tryagainrecal:
	/* set SDH, step rate, do restore to recalibrate drive */
	outb(wdc+wd_sdh, WDSD_IBM | (io->i_unit << 4));
	wdwait(wdc);
	outb(wdc+wd_command, WDCC_RESTORE | WD_STEP);
	wdwait(wdc);
	if ((i = inb(wdc+wd_status)) & WDCS_ERR) {
#ifdef SMALL
		printf("wd%d: recal status %x error %x\n",
			unit, i, inb(wdc+wd_error));
#else
		printf("wd%d: recal status %b error %b\n",
			unit, i, WDCS_BITS, inb(wdc+wd_error), WDERR_BITS);
#endif
		if (++errcnt < 10)
			goto tryagainrecal;
		return (-1);
	}

#ifndef SMALL
	/*
	 * If we have been passed the BSD geometry, use that (ifdef BSDGEOM).
	 * Otherwise, if we have geometry from the BIOS/CMOS setup,
	 * use it as the default geometry, otherwise use a "safe"
	 * geometry that lets us get to the beginning of each DOS-accesible
	 * cylinder.
	 */
	i = 0;
#ifdef BSDGEOM
	if (io->i_ctlr == 0 && bsdgeom->unit == io->i_unit &&
	    bsdgeom->ncylinders) {
		dd->d_ncylinders = bsdgeom->ncylinders;
		dd->d_ntracks = bsdgeom->ntracks;
		dd->d_nsectors = bsdgeom->nsectors;
		i = bsdgeom->bsd_startsec + LABELSECTOR;
	} else
#endif
	if (io->i_ctlr == 0 && (bgp = biosgeom[io->i_unit])) {
		dd->d_ncylinders = bgp->ncylinders;
		dd->d_ntracks = bgp->ntracks;
		dd->d_nsectors = bgp->nsectors;
	} else {
		dd->d_ncylinders = 1023;
		dd->d_ntracks = 5;
		dd->d_nsectors = 17;
	}
	outb(wdc+wd_sdh, WDSD_IBM | (io->i_unit << 4) + dd->d_ntracks - 1);
	outb(wdc+wd_seccnt, dd->d_nsectors);
	outb(wdc+wd_cyl_lo, dd->d_ncylinders);
	outb(wdc+wd_cyl_hi, dd->d_ncylinders >> 8);
	outb(wdc+wd_command, WDCC_SETGEOM);
	while (inb(wdc+wd_status) & WDCS_BUSY)
		;

	errcnt = 0;
	/*
	 * Initialize defaults, enough that we can call wdio.
	 */
	dkbad[unit].bt_bad[0].bt_cyl = 0xffff;
	dd->d_secpercyl = dd->d_nsectors * dd->d_ntracks;
	dd->d_secsize = 512;

	if (i == 0) {
		/*
		 * Check for DOS partition table in sector 0
		 * and determine BSD label location.
		 */
		i = LABELSECTOR;
		dprintf(("read dos bb1: "));
		if (wdio(F_READ, io, 0, buf, 1) == 1) {
			struct mbpart *mp, *getbsdpartition();
			if (mp = getbsdpartition(buf)) {
				i += mbpssec(mp) +
				    mbpstrk(mp) * dd->d_nsectors +
				    mbpscyl(mp) * dd->d_secpercyl;
				dprintf(("label at %d/%d/%d => %d\n",
				    mbpscyl(mp), mbpstrk(mp), mbpssec(mp), i));
			}
		}
	}
	
	/*
	 * Read in disklabel sector to get the pack label and geometry.
	 */
	if (wdio(F_READ, io, i, buf, 1) != 1) {
		if (!wdquiet)
		    printf("wd%d: can't read label\n", unit);
		return (-1);
	}
	if (((struct disklabel *) (buf + LABELOFFSET))->d_magic == DISKMAGIC) {
		*dd = * (struct disklabel *) (buf + LABELOFFSET);
		open[unit] = 1;
	} else {
		printf("wd%d: bad disk label\n", unit);
		if (io->i_flgs & F_FILE)
			return (-1);
		outb(wdc+wd_precomp, 0xff);	/* force head 3 bit off */
		return (0);
	}
#else /* SMALL */
	/* label should have been loaded with us ... */
	dprintf(("magic %x sect %d cyl %d trk %d unit %d\n",
	   dd->d_magic, dd->d_nsectors, dd->d_ncylinders, dd->d_ntracks, unit));
#endif /* SMALL */

	/* now that we know the disk geometry, tell the controller */
	dprintf(("set geom\n"));
	outb(wdc+wd_cyl_lo, dd->d_ncylinders);
	outb(wdc+wd_cyl_hi, dd->d_ncylinders >> 8);
	outb(wdc+wd_sdh, WDSD_IBM | (unit << 4) + dd->d_ntracks - 1);
	outb(wdc+wd_seccnt, dd->d_nsectors);
	outb(wdc+wd_command, WDCC_SETGEOM);
	while (inb(wdc+wd_status) & WDCS_BUSY)
		;

	outb(wdc+wd_precomp, dd->d_precompcyl / 4);

#ifndef SMALL
	/*
	 * Read bad sector table into memory.
	 */
	i = 0;
	do {
		int blknm = dd->d_secperunit - dd->d_nsectors + i;
		
		dprintf(("read badsect %d\n", blknm));
		errcnt = wdio(F_READ, io, blknm, buf, 1);
	} while (errcnt == -1 && (i += 2) < 10 && i < dd->d_nsectors);
	
	db = (struct dkbad *) (buf);
#define DKBAD_MAGIC 0x4321
	if (errcnt != -1 && db->bt_mbz == 0 && db->bt_flag == DKBAD_MAGIC)
		dkbad[unit] = *db;
	else if (!wdquiet)
		printf("wd%d: error in bad-sector file\n", unit);
#endif
	return (0);
}

wdwait(wdc)
	register wdc;
{
	register i = 0;
	
	while (inb(wdc+wd_status) & WDCS_BUSY)
		;
	while ((inb(wdc+wd_status) & WDCS_READY) == 0)
		if (i++ > 100000)
			return(-1);
	return(0);
}
