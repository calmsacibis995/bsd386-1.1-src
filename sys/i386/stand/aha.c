/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: aha.c,v 1.6 1993/08/23 03:14:10 karels Exp $
 */

/*
 * Standalone Adaptec 1542[BC] SCSI host adapter driver.
 */

#include "saio.h"
#include "scsi/scsi.h"
#include "sascsi.h"
#include "i386/isa/ahareg.h"
#include "i386/isa/isa.h"

#define	NO_DRQ	-1

#define	DMA_MASK_REG_1	0x0a
#define	DMA_MASK_REG_2	0xd4
#define	DMA_MASK_REG(drq)	((drq) & 0x4 ? DMA_MASK_REG_2 : DMA_MASK_REG_1)
#define	DMA_MODE_REG_1	0x0b
#define	DMA_MODE_REG_2	0xd6
#define	DMA_MODE_REG(drq)	((drq) & 0x4 ? DMA_MODE_REG_2 : DMA_MODE_REG_1)

#define	DMA_MODE_CASCADE	0xc0
#define	DMA_MODE(drq)		(DMA_MODE_CASCADE|((drq) & 0x3))
#define	DMA_MASK(drq)		((drq) & 0x3)

#define	NTARG			8
#define	NUNIT			8

int ahaattach __P((struct iob *io));
int ahastart __P((struct iob *io, int,
    int (*dgo) (struct iob *io, int, struct scsi_cdb *cdb),
    struct scsi_cdb *cdb));
int ahago __P((struct iob *io));

struct scsi_driver ahadriver = {
	ahaattach,
	ahastart,
	ahago
};

#ifdef SMALL
struct scsi_driver *scsi_driver[] = {
	&ahadriver,		/* 0 = ahadriver */
};
#endif

#define	AHA_PRIMARY_PORT	0x330
#define	AHA_SECONDARY_PORT	0x230
#define	AHA_TERTIARY_PORT	0x130

static const char cmd_out_size[] =
    { 0, 4, 0, 0, 0, 1, 4, 1, 1, 1, 0, 0, 2, 1, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 0, 1,
      0, 2, 35,3, 1, 0, 0, 0, 0, 2, 0 };

struct aha_softc {
	int	sc_port;
	int	sc_targ;
	int	sc_lun;
} aha_softc[] = {
	{ AHA_PRIMARY_PORT },
#if 0
	{ AHA_SECONDARY_PORT },
	{ AHA_TERTIARY_PORT },
#endif
};

struct mbox mbox[2];

#define	outbox	mbox[0]
#define	inbox	mbox[1]

struct ccb ccb;

#ifndef SMALL
/*
 * Allow users to type 'aha()bsd'.
 */
int
ahaopen(io)
	struct iob *io;
{

	io->i_dev = SD_MAJORDEV;
	io->i_adapt = 0;
	return (sdopen(io));
}
#endif


static int
ahafastcmd(aha, cmd, out, in)
	int aha, cmd;
	unsigned char *out, *in;
{
	int cnt;

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
		return (EIO);
	}

	outb(AHA_STAT(aha), AHA_C_IRST);

	return (0);
}

int
ahaattach(io)
	struct iob *io;
{
	struct aha_softc *sc;
	u_char in[4], out[4];
	int drq;
	int aha;

#if 0
	if ((unsigned)io->i_adapt >=
	    sizeof aha_softc / sizeof (struct aha_softc))
		return (EADAPT);
	sc = &aha_softc[io->i_adapt];
#else
	/* we currently steal i_adapt for other purposes */
	sc = &aha_softc[0];
#endif
	aha = sc->sc_port;
	if (io->i_ctlr & ~(NTARG - 1))
		return (ECTLR);
	sc->sc_targ = io->i_ctlr;
	if (io->i_unit & ~(NUNIT - 1))
		return (EUNIT);

#ifndef SMALL
	/*
	 * Set the 8237 up for bus master DMA.
	 * The BIOS may not be enabled, hence we must do this ourselves.
	 */
	ahafastcmd(aha, AHA_CONFIG_DATA, (u_char *)0, in);
	switch (in[0]) {
	case 0x80:	drq = 7; break;
	case 0x40:	drq = 6; break;
	case 0x20:	drq = 5; break;
	case 0x01:	drq = 0; break;
	case 0x00:	drq = NO_DRQ; break;	/* Local bus? */
	default:
		printf("aha%d: can't get DRQ\n", io->i_adapt);
		return (EIO);
	}
	if (drq != NO_DRQ) {
		outb(DMA_MODE_REG(drq), DMA_MODE(drq));
		outb(DMA_MASK_REG(drq), DMA_MASK(drq));
	}

	/*
	 * Do a soft reset.
	 */
	outb(AHA_STAT(aha), AHA_C_SRST);
	wait(100000);

	/*
	 * Program the mailbox lock on the 1542C and late-model 1542Bs.
	 */
	if (ahafastcmd(aha, AHA_GET_LOCK, (u_char *)0, in) == 0 && in[1]) {
		out[0] = 0;
		out[1] = in[1];
		ahafastcmd(aha, AHA_SET_LOCK, out, (u_char *)0);
	}
#endif

	/*
	 * Install new mailbox address.
	 */
	out[0] = 1;	/* one mailbox */
	out[1] = (unsigned)&mbox >> 16;
	out[2] = (unsigned)&mbox >> 8;
	out[3] = (unsigned)&mbox;
	if (ahafastcmd(aha, AHA_MBOX_INIT, out, (u_char *)0))
		printf("aha mbox init failed\n");

	return (0);
}

int
ahastart(io, flag, dgo, cdb)
	struct iob *io;
	int flag;
	int (*dgo) __P((struct iob *io, int, struct scsi_cdb *cdb));
	struct scsi_cdb *cdb;
{
	struct aha_softc *sc = &aha_softc[io->i_adapt];
	unsigned int u;

	bzero(&ccb, sizeof ccb);
	ccb.ccb_opcode = CCB_CMD_RDL;
	ccb.ccb_control = CCB_CONTROL(sc->sc_targ, 0, 0, sc->sc_lun);
	ccb.ccb_rqslen = 1;
	u = (unsigned)io->i_ma;
	ccb.ccb_data[0] = u >> 16;
	ccb.ccb_data[1] = u >> 8;
	ccb.ccb_data[2] = u;
	u = io->i_cc;
	ccb.ccb_datalen[0] = u >> 16;
	ccb.ccb_datalen[1] = u >> 8;
	ccb.ccb_datalen[2] = u;
	if (cdb)
		bcopy(cdb, &ccb.ccb_cdb, sizeof ccb.ccb_cdb);

	return ((*dgo)(io, flag,
	    cdb ? (struct scsi_cdb *)0 : (struct scsi_cdb *)&ccb.ccb_cdb));
}

int
ahago(io)
	struct iob *io;
{
	struct aha_softc *sc = &aha_softc[io->i_adapt];
	int istat, resid;
	int aha = sc->sc_port;

	ccb.ccb_cdblen = scsicmdlen[ccb.ccb_cdbbytes[0] >> 5];

	outbox.mb_ccb[0] = (unsigned)&ccb >> 16;
	outbox.mb_ccb[1] = (unsigned)&ccb >> 8;
	outbox.mb_ccb[2] = (unsigned)&ccb;
	outbox.mb_cmd = MBOX_O_START;

	outb(AHA_STAT(aha), AHA_C_IRST);
	while (inb(AHA_STAT(aha)) & AHA_S_CDF)
		;
	outb(AHA_DATA(aha), AHA_START_SCSI_CMD);

	while ((inb(AHA_INTR(aha)) & AHA_I_ANY) == 0)
		;
	istat = inb(AHA_INTR(aha));
	outb(AHA_STAT(aha), AHA_C_IRST);
	if ((istat & AHA_I_MBIF) == 0)
		_stop("ahago missing mbif");
	inbox.mb_status = MBOX_I_FREE;

	resid = ccb.ccb_datalen[0] << 16 | ccb.ccb_datalen[1] << 8 |
	    ccb.ccb_datalen[2];
	if (ccb.ccb_hastat != CCB_H_NORMAL) {
		printf("aha%d: host adapter error 0x%x\n",
		    io->i_adapt, ccb.ccb_hastat);
		return (-1);
	}

	return (ccb.ccb_tarstat);
}
