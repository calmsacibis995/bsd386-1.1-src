/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: sd.c,v 1.5 1994/01/07 01:16:31 karels Exp $
 */

/*
 * Standalone SCSI driver, modelled on LBL machine-independent SCSI driver.
 * This is probably more complicated than it needs to be.
 */

#include "saio.h"
#include "disklabel.h"
#include "scsi/scsi.h"
#include "scsi/disk.h"
#include "i386/include/bootblock.h"

#include "sascsi.h"

int scsi_test_unit_ready __P((struct iob *io));
int scsi_request_sense __P((struct iob *io, struct scsi_sense *sn));
void sderror __P((struct iob *io, int stat));
int sdopen __P((struct iob *io));
int sdgo __P((struct iob *io, int flag, struct scsi_cdb *c));
int sdstrategy __P((struct iob *io, int flag));

#define	NADAPT		2	/* number of recognized adapter types */

#ifndef SMALL
/* XXX only one host adapter type at a time */
/* XXX use aha as the default */
extern struct scsi_driver ahadriver;
extern struct scsi_driver eahadriver;
struct scsi_driver *scsi_driver[] = {
	&ahadriver,		/* adapter 0 = aha */
	&eahadriver,		/* adapter 1 = eaha */
};
#endif

struct scsi_driver *scd;

struct scsi_cdb cdb;

/* table of lengths of scsi commands */
const char scsicmdlen[8] = { 6, 10, 0, 0, 0, 12, 0, 0 };

#ifndef SMALL
/*
 * Sometimes we need to repeat a TUR to get past
 * the initial trauma of UNIT ATTENTION after a reset.
 */
int
scsi_test_unit_ready(io)
	struct iob *io;
{
	int stat;
	int retries = 0;

	do {
		bzero(&cdb, sizeof cdb);
		io->i_ma = 0;
		io->i_cc = 0;
		cdb.cdb_bytes[0] = CMD_TEST_UNIT_READY;
#ifdef notyet /* i_unit is not currently a LUN */
		cdb.cdb_bytes[1] = io->i_unit << 5;
#endif
		stat = (*scd->s_start)(io, 0, sdgo, &cdb);
		if (++retries > 1) {
			sderror(io, stat);
			return (stat);
		}
	} while ((stat & STS_MASK) != STS_GOOD);

	return (stat);
}

int
scsi_request_sense(io, sn)
	struct iob *io;
	struct scsi_sense *sn;
{

	bzero(&cdb, sizeof cdb);
	io->i_ma = (char *)sn;
	io->i_cc = sizeof *sn;
	cdb.cdb_bytes[0] = CMD_REQUEST_SENSE;
#ifdef notyet /* i_unit is not currently a LUN */
	cdb.cdb_bytes[1] = io->i_unit << 5;
#endif
	cdb.cdb_bytes[4] = sizeof *sn;
	return ((*scd->s_start)(io, F_READ, sdgo, &cdb));
}

const char * scsisensekeymsg[16] = {
    "no sense",
    "recovered error",
    "not ready",
    "medium error",
    "hardware error",
    "illegal request",
    "unit attention",
    "data protect",
    "blank check",
    "vendor specific error",
    "copy aborted",
    "aborted command",
    "equal comparison",
    "volume overflow",
    "miscompare",
    "reserved error code"
};

void
sderror(io, stat)
	struct iob *io;
	int stat;
{
	struct scsi_sense sn;
	int key;

	if ((stat & STS_MASK) == STS_CHECKCOND) {
		stat = scsi_request_sense(io, &sn);
		if ((stat & STS_MASK) != STS_GOOD) {
			printf("sd%d: sense failed, status %x\n",
			    io->i_adapt, stat);
			return;
		}
		printf("sd%d: scsi sense", io->i_adapt);
		if (SENSE_ISXSENSE(&sn) && XSENSE_ISSTD(&sn)) {
			key = XSENSE_KEY(&sn);
			printf(": %s", scsisensekeymsg[key]);
			if (XSENSE_HASASC(&sn))
				printf(": asc 0x%x", XSENSE_ASC(&sn));
			if (XSENSE_HASASCQ(&sn))
				printf(", ascq 0x%x", XSENSE_ASCQ(&sn));
			if (XSENSE_IVALID(&sn))
				printf(", blk %d", XSENSE_INFO(&sn));
		} else
			printf(" class %d, code %d",
			    SENSE_ECLASS(&sn), SENSE_ECODE(&sn));
		printf("\n");
	} else
		printf("sd%d: command failed, status %x\n",
		    io->i_adapt, stat);
}
#endif /* SMALL */

int
sdopen(io)
	struct iob *io;
{
	static u_char buf[DEV_BSIZE];
	struct disklabel *dl;
	struct mbpart *mp, *getbsdpartition();
	int error;
#ifdef SMALL
	extern struct disklabel disklabel;
#endif

#ifndef SMALL
	if ((unsigned)io->i_adapt >= NADAPT)
		return (EADAPT);
#endif

	scd = scsi_driver[io->i_adapt];
	if (error = (*scd->s_attach)(io))
		return (error);
#ifndef SMALL
	if ((scsi_test_unit_ready(io) & STS_MASK) != STS_GOOD) {
		printf("sd%d: not ready\n", io->i_adapt);
		return (EIO);
	}
#endif

#ifdef SMALL
	dl = &disklabel;
#else
	/* read the dos partition table to see where our label might be */
	io->i_boff = 0;
	io->i_bn = 0;
	io->i_ma = (char *)buf;
	io->i_cc = DEV_BSIZE;
	if (devread(io) == -1)
		return (EIO);
	io->i_bn = LABELSECTOR;
	if (mp = getbsdpartition(buf))
		io->i_bn += mp->start;

	/* read the disk label */
	if (devread(io) == -1)
		return (ERDLAB);
	dl = (struct disklabel *)&buf[LABELOFFSET];
	if (dl->d_magic != DISKMAGIC)
		return (ERDLAB);
	if (io->i_part >= dl->d_npartitions)
		return (EPART);
#endif
	io->i_boff = dl->d_partitions[io->i_part].p_offset;
	return (0);
}

/* ARGSUSED */
void
sdclose(io)
	struct iob *io;
{

}

int
sdgo(io, flag, c)
	struct iob *io;
	int flag;
	struct scsi_cdb *c;
{
	unsigned int u;

	if (c) {
		/* XXX this code assumes DEV_BSIZE blocks! */
		CDB10(c)->cdb_cmd = flag & F_READ ? CMD_READ10 : CMD_WRITE10;
#ifdef notyet /* i_unit is not currently a LUN */
		CDB10(c)->cdb_lun_rel = io->i_unit << 5;
#endif
		u = io->i_bn;
		CDB10(c)->cdb_lbah = u >> 24;
		CDB10(c)->cdb_lbahm = u >> 16;
		CDB10(c)->cdb_lbalm = u >> 8;
		CDB10(c)->cdb_lbal = u;
		CDB10(c)->cdb_xxx = 0;
		u = (io->i_cc + (DEV_BSIZE - 1)) >> DEV_BSHIFT;
		CDB10(c)->cdb_lenh = u >> 8;
		CDB10(c)->cdb_lenl = u;
		CDB10(c)->cdb_ctrl = 0;
	}
	return ((*scd->s_go)(io));
}

int
sdstrategy(io, flag)
	struct iob *io;
	int flag;
{
	int stat;

	/* XXX check transfer parameters for reasonableness? */
	stat = (*scd->s_start)(io, flag, sdgo, (struct scsi_cdb *)0);
	if ((stat & STS_MASK) != STS_GOOD) {
#ifndef SMALL
		sderror(io, stat);
#endif
		return (-1);
	}
	return (io->i_cc);
}
